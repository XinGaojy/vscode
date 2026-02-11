#include "kronos.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace kronos {
namespace {

struct Sample {
  std::string metric;
  std::vector<std::pair<std::string, std::string>> tags;
  int64_t timestamp = 0;
  std::vector<tsdb::FieldValue> fields;
};

uint64_t HashToken(const std::string& s) {
  uint64_t hash = 1469598103934665603ULL;
  for (unsigned char c : s) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  return hash;
}

std::vector<std::string> SplitWhitespace(const std::string& s) {
  std::vector<std::string> out;
  std::istringstream iss(s);
  std::string token;
  while (iss >> token) {
    out.push_back(token);
  }
  return out;
}

std::vector<std::string> SplitByChar(const std::string& s, char delim) {
  std::vector<std::string> out;
  std::string item;
  std::istringstream iss(s);
  while (std::getline(iss, item, delim)) {
    if (!item.empty()) {
      out.push_back(item);
    }
  }
  return out;
}

bool ParseTagToken(const std::string& token, std::string* key, std::string* value) {
  auto pos = token.find('=');
  if (pos == std::string::npos || pos == 0 || pos + 1 >= token.size()) {
    return false;
  }
  *key = token.substr(0, pos);
  *value = token.substr(pos + 1);
  return true;
}

bool ParseFieldToken(const std::string& token, tsdb::FieldValue* out, std::string* err) {
  std::string key;
  std::string value;
  if (!ParseTagToken(token, &key, &value)) {
    if (err) {
      *err = "invalid field token";
    }
    return false;
  }
  try {
    out->name = std::move(key);
    out->value = std::stod(value);
  } catch (...) {
    if (err) {
      *err = "invalid field value";
    }
    return false;
  }
  return true;
}

bool IsNumericToken(const std::string& token, double* out) {
  try {
    size_t idx = 0;
    double value = std::stod(token, &idx);
    if (idx != token.size()) {
      return false;
    }
    *out = value;
    return true;
  } catch (...) {
    return false;
  }
}

bool ParseLine(const std::string& line, Sample* out, std::string* err) {
  auto tokens = SplitWhitespace(line);
  if (tokens.size() < 2) {
    if (err) {
      *err = "line has too few tokens";
    }
    return false;
  }
  out->metric = tokens[0];
  out->tags.clear();
  out->fields.clear();
  size_t i = 1;
  bool has_timestamp = false;
  for (; i < tokens.size(); ++i) {
    const std::string& tok = tokens[i];
    if (tok.rfind("timestamp=", 0) == 0) {
      std::string ts_str = tok.substr(std::string("timestamp=").size());
      try {
        out->timestamp = std::stoll(ts_str);
      } catch (...) {
        if (err) {
          *err = "invalid timestamp";
        }
        return false;
      }
      has_timestamp = true;
      ++i;
      break;
    }
    std::string key;
    std::string value;
    if (!ParseTagToken(tok, &key, &value)) {
      if (err) {
        *err = "invalid tag token";
      }
      return false;
    }
    out->tags.emplace_back(key, value);
  }

  if (!has_timestamp) {
    if (err) {
      *err = "missing timestamp";
    }
    return false;
  }

  if (i >= tokens.size()) {
    if (err) {
      *err = "missing fields after timestamp";
    }
    return false;
  }

  std::vector<double> legacy_values;
  bool legacy = true;
  for (size_t idx = i; idx < tokens.size(); ++idx) {
    double value = 0.0;
    if (!IsNumericToken(tokens[idx], &value)) {
      legacy = false;
      break;
    }
    legacy_values.push_back(value);
  }

  if (legacy && legacy_values.size() == 5) {
    static const std::array<std::string, 5> kLegacyNames = {"min", "max", "avg", "sum", "count"};
    out->fields.reserve(kLegacyNames.size());
    for (size_t idx = 0; idx < kLegacyNames.size(); ++idx) {
      out->fields.push_back(tsdb::FieldValue{kLegacyNames[idx], legacy_values[idx]});
    }
    return true;
  }

  for (size_t idx = i; idx < tokens.size(); ++idx) {
    tsdb::FieldValue field;
    if (!ParseFieldToken(tokens[idx], &field, err)) {
      return false;
    }
    out->fields.push_back(std::move(field));
  }
  return true;
}

std::string MakeSeriesKey(const std::string& metric,
                          const std::vector<std::pair<std::string, std::string>>& tags) {
  std::vector<std::pair<std::string, std::string>> sorted_tags = tags;
  std::sort(sorted_tags.begin(), sorted_tags.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
  std::string key = metric;
  for (const auto& kv : sorted_tags) {
    key.append("|");
    key.append(kv.first);
    key.append("=");
    key.append(kv.second);
  }
  return key;
}

bool HasField(const std::vector<tsdb::FieldValue>& fields, const std::string& name, double* value) {
  for (const auto& field : fields) {
    if (field.name == name) {
      if (value) {
        *value = field.value;
      }
      return true;
    }
  }
  return false;
}

struct RollupBucket {
  std::string metric;
  std::vector<std::pair<std::string, std::string>> tags;
  int64_t timestamp = 0;
  bool has = false;
  bool standard = false;
  double min_v = 0.0;
  double max_v = 0.0;
  double sum_v = 0.0;
  double count_v = 0.0;
  std::unordered_map<std::string, std::pair<double, double>> generic;
};

void AddToBucket(RollupBucket* bucket, const Sample& sample, int64_t window_start) {
  if (!bucket->has) {
    bucket->metric = sample.metric;
    bucket->tags = sample.tags;
    bucket->timestamp = window_start;
    bucket->has = true;
    double tmp = 0.0;
    bucket->standard = HasField(sample.fields, "min", &tmp) &&
                       HasField(sample.fields, "max", &tmp) &&
                       HasField(sample.fields, "sum", &tmp) &&
                       HasField(sample.fields, "count", &tmp);
  }

  if (bucket->standard) {
    double cur_min = 0.0;
    double cur_max = 0.0;
    double cur_sum = 0.0;
    double cur_count = 0.0;
    HasField(sample.fields, "min", &cur_min);
    HasField(sample.fields, "max", &cur_max);
    HasField(sample.fields, "sum", &cur_sum);
    HasField(sample.fields, "count", &cur_count);
    if (bucket->count_v == 0.0) {
      bucket->min_v = cur_min;
      bucket->max_v = cur_max;
    } else {
      bucket->min_v = std::min(bucket->min_v, cur_min);
      bucket->max_v = std::max(bucket->max_v, cur_max);
    }
    bucket->sum_v += cur_sum;
    bucket->count_v += cur_count;
  } else {
    for (const auto& field : sample.fields) {
      auto& stat = bucket->generic[field.name];
      stat.first += field.value;
      stat.second += 1.0;
    }
  }
}

std::string SerializeLine(const std::string& metric,
                          const std::vector<std::pair<std::string, std::string>>& tags,
                          int64_t timestamp,
                          const std::vector<tsdb::FieldValue>& fields) {
  std::ostringstream oss;
  oss << metric;
  std::vector<std::pair<std::string, std::string>> sorted_tags = tags;
  std::sort(sorted_tags.begin(), sorted_tags.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
  for (const auto& kv : sorted_tags) {
    oss << " " << kv.first << "=" << kv.second;
  }
  oss << " timestamp=" << timestamp;
  for (const auto& field : fields) {
    oss << " " << field.name << "=" << field.value;
  }
  return oss.str();
}

std::vector<std::string> DownsampleLines(const std::vector<std::string>& lines,
                                         int64_t resolution,
                                         std::string* err) {
  std::unordered_map<std::string, RollupBucket> buckets;
  buckets.reserve(lines.size());

  for (const auto& line : lines) {
    if (line.empty()) {
      continue;
    }
    Sample sample;
    if (!ParseLine(line, &sample, err)) {
      return {};
    }
    int64_t window = resolution > 0 ? (sample.timestamp / resolution) : 0;
    int64_t window_start = resolution > 0 ? window * resolution : sample.timestamp;
    std::string series_key = MakeSeriesKey(sample.metric, sample.tags);
    std::string key = series_key + "|" + std::to_string(window);
    auto& bucket = buckets[key];
    AddToBucket(&bucket, sample, window_start);
  }

  std::vector<std::string> output;
  output.reserve(buckets.size());
  for (auto& entry : buckets) {
    auto& bucket = entry.second;
    if (!bucket.has) {
      continue;
    }
    std::vector<tsdb::FieldValue> fields;
    if (bucket.standard) {
      double avg = bucket.count_v > 0.0 ? bucket.sum_v / bucket.count_v : 0.0;
      fields.push_back(tsdb::FieldValue{"min", bucket.min_v});
      fields.push_back(tsdb::FieldValue{"max", bucket.max_v});
      fields.push_back(tsdb::FieldValue{"avg", avg});
      fields.push_back(tsdb::FieldValue{"sum", bucket.sum_v});
      fields.push_back(tsdb::FieldValue{"count", bucket.count_v});
    } else {
      fields.reserve(bucket.generic.size());
      for (const auto& kv : bucket.generic) {
        double avg = kv.second.second > 0.0 ? kv.second.first / kv.second.second : 0.0;
        fields.push_back(tsdb::FieldValue{kv.first, avg});
      }
    }
    output.push_back(SerializeLine(bucket.metric, bucket.tags, bucket.timestamp, fields));
  }
  return output;
}

bool ReadLines(const std::string& path, std::vector<std::string>* lines, std::string* err) {
  std::ifstream in(path);
  if (!in) {
    if (err) {
      *err = "failed to open input file: " + path;
    }
    return false;
  }
  lines->clear();
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty()) {
      lines->push_back(line);
    }
  }
  return true;
}

class FileQueue {
 public:
  explicit FileQueue(std::string dir) : dir_(std::move(dir)) {}

  bool Append(const std::string& line, std::string* err) {
    std::error_code ec;
    std::filesystem::create_directories(dir_, ec);
    if (ec) {
      if (err) {
        *err = "failed to create queue dir: " + dir_;
      }
      return false;
    }
    std::ofstream out(LogPath(), std::ios::app);
    if (!out) {
      if (err) {
        *err = "failed to open queue log: " + LogPath();
      }
      return false;
    }
    out << line << "\n";
    return true;
  }

  bool ReadBatch(uint64_t offset,
                 size_t max_messages,
                 std::vector<std::string>* out,
                 uint64_t* next_offset,
                 std::string* err) const {
    out->clear();
    const std::string path = LogPath();
    if (!std::filesystem::exists(path)) {
      *next_offset = 0;
      return true;
    }
    std::ifstream in(path);
    if (!in) {
      if (err) {
        *err = "failed to open queue log: " + path;
      }
      return false;
    }
    const uint64_t file_size = static_cast<uint64_t>(std::filesystem::file_size(path));
    if (offset > file_size) {
      offset = file_size;
    }
    in.seekg(static_cast<std::streamoff>(offset));
    std::string line;
    uint64_t current_offset = offset;
    while (out->size() < max_messages && std::getline(in, line)) {
      if (!line.empty()) {
        out->push_back(line);
      }
      auto pos = in.tellg();
      if (pos >= 0) {
        current_offset = static_cast<uint64_t>(pos);
      }
    }
    if (!in && current_offset == offset) {
      current_offset = file_size;
    }
    if (in.eof()) {
      current_offset = file_size;
    }
    *next_offset = current_offset;
    return true;
  }

  bool LoadOffset(const std::string& consumer, uint64_t* offset) const {
    std::ifstream in(OffsetPath(consumer));
    if (!in) {
      *offset = 0;
      return true;
    }
    uint64_t value = 0;
    if (in >> value) {
      *offset = value;
      return true;
    }
    *offset = 0;
    return true;
  }

  bool SaveOffset(const std::string& consumer, uint64_t offset, std::string* err) const {
    std::ofstream out(OffsetPath(consumer));
    if (!out) {
      if (err) {
        *err = "failed to write queue offset";
      }
      return false;
    }
    out << offset << "\n";
    return true;
  }

 private:
  std::string LogPath() const { return dir_ + "/queue.log"; }
  std::string OffsetPath(const std::string& consumer) const {
    return dir_ + "/" + consumer + ".offset";
  }

  std::string dir_;
};

std::string ClusterMetaPath(const std::string& root_dir) {
  return root_dir + "/cluster.meta";
}

bool CopyDirectory(const std::string& from, const std::string& to, std::string* err) {
  std::error_code ec;
  std::filesystem::create_directories(to, ec);
  if (ec) {
    if (err) {
      *err = "failed to create directory: " + to;
    }
    return false;
  }
  std::filesystem::copy(from, to,
                        std::filesystem::copy_options::recursive |
                            std::filesystem::copy_options::overwrite_existing,
                        ec);
  if (ec) {
    if (err) {
      *err = "failed to copy directory";
    }
    return false;
  }
  return true;
}

std::string ShardDir(const ClusterConfig& config, int shard_id) {
  return config.online_dir + "/shard_" + std::to_string(shard_id);
}

std::string PanguShardDir(const ClusterConfig& config,
                          const std::string& version,
                          int shard_id) {
  return config.pangu_dir + "/index_" + version + "/shard_" + std::to_string(shard_id);
}

std::string CurrentVersion() {
  auto now = std::chrono::system_clock::now().time_since_epoch();
  auto sec = std::chrono::duration_cast<std::chrono::seconds>(now).count();
  return std::to_string(sec);
}

int ShardForSeries(const std::string& series_key, int shards) {
  if (shards <= 0) {
    return 0;
  }
  uint64_t hash = HashToken(series_key);
  return static_cast<int>(hash % static_cast<uint64_t>(shards));
}

std::vector<int> ResolveShardIds(const ClusterConfig& config,
                                 const tsdb::Query& query,
                                 bool exact_series) {
  if (exact_series && !query.metric.empty() && !query.tags.empty()) {
    std::string key = MakeSeriesKey(query.metric, query.tags);
    return {ShardForSeries(key, config.shards)};
  }
  std::vector<int> ids;
  ids.reserve(static_cast<size_t>(config.shards));
  for (int i = 0; i < config.shards; ++i) {
    ids.push_back(i);
  }
  return ids;
}

}  // namespace

bool InitCluster(ClusterConfig* config, std::string* err) {
  if (!config || config->root_dir.empty()) {
    if (err) {
      *err = "root_dir is required";
    }
    return false;
  }
  if (config->shards <= 0) {
    config->shards = 1;
  }
  if (config->online_dir.empty()) {
    config->online_dir = config->root_dir + "/online";
  }
  if (config->mq_dir.empty()) {
    config->mq_dir = config->root_dir + "/mq";
  }
  if (config->pangu_dir.empty()) {
    config->pangu_dir = config->root_dir + "/pangu";
  }
  std::error_code ec;
  std::filesystem::create_directories(config->root_dir, ec);
  if (ec) {
    if (err) {
      *err = "failed to create root dir";
    }
    return false;
  }
  for (int i = 0; i < config->shards; ++i) {
    std::filesystem::create_directories(ShardDir(*config, i), ec);
  }
  std::filesystem::create_directories(config->mq_dir + "/raw", ec);
  std::filesystem::create_directories(config->pangu_dir, ec);
  if (!SaveClusterConfig(*config, err)) {
    return false;
  }
  return true;
}

bool LoadClusterConfig(const std::string& root_dir, ClusterConfig* config, std::string* err) {
  std::ifstream in(ClusterMetaPath(root_dir));
  if (!in) {
    if (err) {
      *err = "failed to open cluster meta";
    }
    return false;
  }
  config->root_dir = root_dir;
  std::string key;
  std::string value;
  while (in >> key >> value) {
    if (key == "shards") {
      config->shards = std::stoi(value);
    } else if (key == "online_dir") {
      config->online_dir = value;
    } else if (key == "mq_dir") {
      config->mq_dir = value;
    } else if (key == "pangu_dir") {
      config->pangu_dir = value;
    } else if (key == "latest_version") {
      config->latest_version = value;
    }
  }
  return true;
}

bool SaveClusterConfig(const ClusterConfig& config, std::string* err) {
  std::ofstream out(ClusterMetaPath(config.root_dir));
  if (!out) {
    if (err) {
      *err = "failed to write cluster meta";
    }
    return false;
  }
  out << "shards " << config.shards << "\n";
  out << "online_dir " << config.online_dir << "\n";
  out << "mq_dir " << config.mq_dir << "\n";
  out << "pangu_dir " << config.pangu_dir << "\n";
  out << "latest_version " << config.latest_version << "\n";
  return true;
}

bool Publish(const ClusterConfig& config,
             const std::string& input_path,
             const PublishOptions& options,
             std::string* err) {
  std::vector<std::string> lines;
  if (!ReadLines(input_path, &lines, err)) {
    return false;
  }

  FileQueue raw_queue(config.mq_dir + "/raw");
  for (const auto& line : lines) {
    if (!raw_queue.Append(line, err)) {
      return false;
    }
  }

  for (int64_t rollup : options.rollup_seconds) {
    if (rollup <= 0) {
      continue;
    }
    std::vector<std::string> downsampled = DownsampleLines(lines, rollup, err);
    if (err && !err->empty()) {
      return false;
    }
    FileQueue rollup_queue(config.mq_dir + "/rollup_" + std::to_string(rollup));
    for (const auto& line : downsampled) {
      if (!rollup_queue.Append(line, err)) {
        return false;
      }
    }
  }

  return true;
}

bool OnlineIngest(const ClusterConfig& config, const IngestOptions& options, std::string* err) {
  FileQueue queue(config.mq_dir + "/raw");
  uint64_t offset = 0;
  if (!queue.LoadOffset("online", &offset)) {
    if (err) {
      *err = "failed to load queue offset";
    }
    return false;
  }

  std::vector<std::string> batch;
  uint64_t next_offset = offset;
  if (!queue.ReadBatch(offset, options.max_messages, &batch, &next_offset, err)) {
    return false;
  }
  if (batch.empty()) {
    return true;
  }

  std::unordered_map<int, std::vector<std::string>> per_shard;
  for (const auto& line : batch) {
    Sample sample;
    if (!ParseLine(line, &sample, err)) {
      return false;
    }
    std::string series_key = MakeSeriesKey(sample.metric, sample.tags);
    int shard_id = ShardForSeries(series_key, config.shards);
    per_shard[shard_id].push_back(line);
  }

  for (const auto& entry : per_shard) {
    int shard_id = entry.first;
    const auto& lines = entry.second;
    const std::string shard_dir = ShardDir(config, shard_id);
    std::error_code ec;
    std::filesystem::create_directories(shard_dir, ec);
    if (ec) {
      if (err) {
        *err = "failed to create shard dir";
      }
      return false;
    }
    const std::string tmp_path = shard_dir + "/ingest_" + CurrentVersion() + ".log";
    std::ofstream out(tmp_path);
    if (!out) {
      if (err) {
        *err = "failed to create ingest batch";
      }
      return false;
    }
    for (const auto& line : lines) {
      out << line << "\n";
    }
    out.close();
    if (!tsdb::Ingest(tmp_path, shard_dir, err)) {
      return false;
    }
    std::filesystem::remove(tmp_path, ec);
  }

  if (!queue.SaveOffset("online", next_offset, err)) {
    return false;
  }
  return true;
}

bool OfflineBuild(const ClusterConfig& config, const BuildOptions& options, std::string* err) {
  FileQueue queue(config.mq_dir + "/raw");
  uint64_t offset = 0;
  if (!queue.LoadOffset("offline", &offset)) {
    if (err) {
      *err = "failed to load queue offset";
    }
    return false;
  }

  std::vector<std::string> batch;
  uint64_t next_offset = offset;
  if (!queue.ReadBatch(offset, options.max_messages, &batch, &next_offset, err)) {
    return false;
  }
  if (batch.empty()) {
    return true;
  }

  std::unordered_map<int, std::vector<std::string>> per_shard;
  for (const auto& line : batch) {
    Sample sample;
    if (!ParseLine(line, &sample, err)) {
      return false;
    }
    std::string series_key = MakeSeriesKey(sample.metric, sample.tags);
    int shard_id = ShardForSeries(series_key, config.shards);
    per_shard[shard_id].push_back(line);
  }

  const std::string version = CurrentVersion();
  for (const auto& entry : per_shard) {
    int shard_id = entry.first;
    const auto& lines = entry.second;
    const std::string shard_dir = config.root_dir + "/offline_staging";
    std::error_code ec;
    std::filesystem::create_directories(shard_dir, ec);
    if (ec) {
      if (err) {
        *err = "failed to create staging dir";
      }
      return false;
    }
    const std::string tmp_path =
        shard_dir + "/shard_" + std::to_string(shard_id) + "_" + version + ".log";
    std::ofstream out(tmp_path);
    if (!out) {
      if (err) {
        *err = "failed to create staging file";
      }
      return false;
    }
    for (const auto& line : lines) {
      out << line << "\n";
    }
    out.close();

    tsdb::Builder builder;
    tsdb::Builder::BuildOptions build_options;
    build_options.partition_seconds = options.partition_seconds;
    build_options.format = options.format;
    build_options.rollup_seconds = options.rollup_seconds;
    const std::string build_dir =
        config.root_dir + "/offline_build/index_" + version + "/shard_" + std::to_string(shard_id);
    if (!builder.Build(tmp_path, build_dir, build_options, err)) {
      return false;
    }

    if (!CopyDirectory(build_dir, PanguShardDir(config, version, shard_id), err)) {
      return false;
    }
    std::filesystem::remove(tmp_path, ec);
  }

  ClusterConfig updated = config;
  updated.latest_version = version;
  if (!SaveClusterConfig(updated, err)) {
    return false;
  }
  if (!queue.SaveOffset("offline", next_offset, err)) {
    return false;
  }
  return true;
}

std::vector<tsdb::QueryResult> QueryCluster(const ClusterConfig& config,
                                            const QueryOptions& options,
                                            std::string* err) {
  std::vector<tsdb::QueryResult> results;
  auto shard_ids = ResolveShardIds(config, options.query, options.exact_series);
  std::unordered_set<std::string> tiers(options.query.tiers.begin(), options.query.tiers.end());
  const bool query_online =
      tiers.empty() || tiers.count("hot") > 0 || tiers.count("warm") > 0;
  const bool query_pangu = tiers.empty() || tiers.count("cold") > 0;

  struct SeriesAccumulator {
    std::map<int64_t, tsdb::Point> points;
  };
  std::unordered_map<std::string, SeriesAccumulator> acc;

  auto add_results = [&](const std::vector<tsdb::QueryResult>& chunk, bool overwrite) {
    for (const auto& res : chunk) {
      auto& series = acc[res.series_key];
      for (const auto& pt : res.points) {
        auto it = series.points.find(pt.timestamp);
        if (it == series.points.end() || overwrite) {
          series.points[pt.timestamp] = pt;
        }
      }
    }
  };

  if (query_pangu && !config.latest_version.empty()) {
    for (int shard_id : shard_ids) {
      std::string dir = PanguShardDir(config, config.latest_version, shard_id);
      if (!std::filesystem::exists(dir)) {
        continue;
      }
      if (options.query.resolution > 0) {
        std::string rollup_dir =
            dir + "/rollup_" + std::to_string(options.query.resolution);
        if (std::filesystem::exists(rollup_dir)) {
          dir = rollup_dir;
        } else {
          if (err && err->empty()) {
            *err = "rollup directory not found: " + rollup_dir;
          }
          continue;
        }
      }
      std::string local_err;
      tsdb::DB db;
      if (!db.Open(dir, &local_err)) {
        if (err && err->empty()) {
          *err = local_err;
        }
        continue;
      }
      auto chunk = db.QueryData(options.query, &local_err);
      if (!local_err.empty() && err && err->empty()) {
        *err = local_err;
      }
      add_results(chunk, false);
    }
  }

  if (query_online) {
    for (int shard_id : shard_ids) {
      std::string dir = ShardDir(config, shard_id);
      if (!std::filesystem::exists(dir)) {
        continue;
      }
      if (options.query.resolution > 0) {
        std::string rollup_dir =
            dir + "/rollup_" + std::to_string(options.query.resolution);
        if (std::filesystem::exists(rollup_dir)) {
          dir = rollup_dir;
        } else {
          if (err && err->empty()) {
            *err = "rollup directory not found: " + rollup_dir;
          }
          continue;
        }
      }
      std::string local_err;
      tsdb::DB db;
      if (!db.Open(dir, &local_err)) {
        if (err && err->empty()) {
          *err = local_err;
        }
        continue;
      }
      auto chunk = db.QueryData(options.query, &local_err);
      if (!local_err.empty() && err && err->empty()) {
        *err = local_err;
      }
      add_results(chunk, true);
    }
  }

  results.reserve(acc.size());
  for (auto& entry : acc) {
    tsdb::QueryResult res;
    res.series_key = entry.first;
    res.points.reserve(entry.second.points.size());
    for (const auto& kv : entry.second.points) {
      res.points.push_back(kv.second);
    }
    results.push_back(std::move(res));
  }
  return results;
}

}  // namespace kronos
