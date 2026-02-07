#include "tsdb.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>

namespace tsdb {
namespace {

constexpr int64_t kDefaultPartitionSeconds = 3600;
constexpr const char* kMetaFile = "meta.txt";
constexpr const char* kPartitionsFile = "partitions.txt";
constexpr const char* kPointsTextFile = "points.txt";
constexpr const char* kPointsBinaryFile = "points.bin";

struct ParsedLine {
  std::string metric;
  std::vector<std::pair<std::string, std::string>> tags;
  int64_t timestamp = 0;
  std::array<double, 5> values{};
};

uint64_t HashToken(const std::string& s) {
  uint64_t hash = 1469598103934665603ULL;
  for (unsigned char c : s) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  return hash;
}

std::vector<std::string> SplitByChar(const std::string& s, char delim) {
  std::vector<std::string> out;
  std::string item;
  std::istringstream iss(s);
  while (std::getline(iss, item, delim)) {
    out.push_back(item);
  }
  return out;
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

bool ParseTagToken(const std::string& token, std::string* key, std::string* value) {
  auto pos = token.find('=');
  if (pos == std::string::npos || pos == 0 || pos + 1 >= token.size()) {
    return false;
  }
  *key = token.substr(0, pos);
  *value = token.substr(pos + 1);
  return true;
}

bool ParseLine(const std::string& line, ParsedLine* out, std::string* err) {
  auto tokens = SplitWhitespace(line);
  if (tokens.size() < 1 + 1 + 5) {
    if (err) {
      *err = "line has too few tokens";
    }
    return false;
  }
  out->metric = tokens[0];
  out->tags.clear();
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

  if (tokens.size() - i != out->values.size()) {
    if (err) {
      *err = "expected 5 numeric fields after timestamp";
    }
    return false;
  }

  for (size_t idx = 0; idx < out->values.size(); ++idx) {
    try {
      out->values[idx] = std::stod(tokens[i + idx]);
    } catch (...) {
      if (err) {
        *err = "invalid numeric field";
      }
      return false;
    }
  }

  return true;
}

std::string MakeSeriesKey(const std::string& metric,
                          const std::vector<std::pair<std::string, std::string>>& tags) {
  std::string key = metric;
  for (const auto& kv : tags) {
    key.append("|");
    key.append(kv.first);
    key.append("=");
    key.append(kv.second);
  }
  return key;
}

bool ParseSeriesKey(const std::string& series_key,
                    std::string* metric,
                    std::unordered_map<std::string, std::string>* tags) {
  auto parts = SplitByChar(series_key, '|');
  if (parts.empty()) {
    return false;
  }
  *metric = parts[0];
  tags->clear();
  for (size_t i = 1; i < parts.size(); ++i) {
    std::string key;
    std::string value;
    if (!ParseTagToken(parts[i], &key, &value)) {
      return false;
    }
    (*tags)[key] = value;
  }
  return true;
}

bool ParsePointLine(const std::string& line, int* series_id, Point* point) {
  std::istringstream iss(line);
  if (!(iss >> *series_id)) {
    return false;
  }
  if (!(iss >> point->timestamp)) {
    return false;
  }
  for (double& v : point->values) {
    if (!(iss >> v)) {
      return false;
    }
  }
  return true;
}

int64_t BucketFor(int64_t timestamp, int64_t partition_seconds) {
  if (partition_seconds <= 0) {
    return 0;
  }
  return timestamp / partition_seconds;
}

std::string PointsFilePath(const std::string& dir, bool binary_points) {
  return dir + "/" + (binary_points ? kPointsBinaryFile : kPointsTextFile);
}

std::string PartitionDir(const std::string& out_dir, int64_t bucket) {
  return out_dir + "/part_" + std::to_string(bucket);
}

void WriteVarint(std::ostream& out, uint64_t value) {
  while (value >= 0x80) {
    uint8_t byte = static_cast<uint8_t>(value) | 0x80;
    out.put(static_cast<char>(byte));
    value >>= 7;
  }
  out.put(static_cast<char>(value));
}

bool ReadVarint(std::istream& in, uint64_t* value) {
  *value = 0;
  int shift = 0;
  for (int i = 0; i < 10; ++i) {
    int byte = in.get();
    if (byte == EOF) {
      return false;
    }
    *value |= static_cast<uint64_t>(byte & 0x7F) << shift;
    if ((byte & 0x80) == 0) {
      return true;
    }
    shift += 7;
  }
  return false;
}

bool WriteBinaryPoint(std::ostream& out, int series_id, const Point& point) {
  WriteVarint(out, static_cast<uint64_t>(series_id));
  WriteVarint(out, static_cast<uint64_t>(point.timestamp));
  for (double v : point.values) {
    uint64_t bits = 0;
    std::memcpy(&bits, &v, sizeof(bits));
    out.write(reinterpret_cast<const char*>(&bits), sizeof(bits));
  }
  return static_cast<bool>(out);
}

bool ReadBinaryPoint(std::istream& in, int* series_id, Point* point) {
  uint64_t id = 0;
  uint64_t ts = 0;
  if (!ReadVarint(in, &id)) {
    return false;
  }
  if (!ReadVarint(in, &ts)) {
    return false;
  }
  point->timestamp = static_cast<int64_t>(ts);
  for (double& v : point->values) {
    uint64_t bits = 0;
    in.read(reinterpret_cast<char*>(&bits), sizeof(bits));
    if (!in) {
      return false;
    }
    std::memcpy(&v, &bits, sizeof(bits));
  }
  *series_id = static_cast<int>(id);
  return true;
}

bool EnsureDir(const std::string& dir, std::string* err) {
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    if (err) {
      *err = "failed to create directory: " + dir;
    }
    return false;
  }
  return true;
}

bool WriteSeriesKeys(const std::string& path,
                     const std::vector<std::string>& series_keys,
                     std::string* err) {
  std::ofstream out(path);
  if (!out) {
    if (err) {
      *err = "failed to open serieskey file: " + path;
    }
    return false;
  }
  for (size_t i = 0; i < series_keys.size(); ++i) {
    out << i << "|" << series_keys[i] << "\n";
  }
  return true;
}

bool LoadSeriesKeys(const std::string& path,
                    std::vector<std::string>* series_keys,
                    std::unordered_map<std::string, int>* series_id_by_key,
                    std::string* err) {
  std::ifstream in(path);
  if (!in) {
    if (err) {
      *err = "failed to open serieskey file: " + path;
    }
    return false;
  }
  series_keys->clear();
  series_id_by_key->clear();
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    auto pos = line.find('|');
    if (pos == std::string::npos) {
      continue;
    }
    int id = std::stoi(line.substr(0, pos));
    std::string key = line.substr(pos + 1);
    if (id >= static_cast<int>(series_keys->size())) {
      series_keys->resize(id + 1);
    }
    (*series_keys)[id] = key;
    (*series_id_by_key)[key] = id;
  }
  return true;
}

bool LoadForwardIndex(const std::string& path, std::vector<Range>* ranges, std::string* err) {
  std::ifstream in(path);
  if (!in) {
    if (err) {
      *err = "failed to open forward index file: " + path;
    }
    return false;
  }
  ranges->clear();
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    std::istringstream iss(line);
    int id = 0;
    int64_t start = -1;
    int64_t end = -1;
    if (!(iss >> id >> start >> end)) {
      continue;
    }
    if (id >= static_cast<int>(ranges->size())) {
      ranges->resize(id + 1);
    }
    (*ranges)[id] = Range{start, end};
  }
  return true;
}

bool WriteForwardIndex(const std::string& path, const std::vector<Range>& ranges, std::string* err) {
  std::ofstream out(path);
  if (!out) {
    if (err) {
      *err = "failed to open forward index file: " + path;
    }
    return false;
  }
  for (size_t i = 0; i < ranges.size(); ++i) {
    out << i << " " << ranges[i].start_row << " " << ranges[i].end_row << "\n";
  }
  return true;
}

bool WritePostings(const std::string& postings_path,
                   const std::string& dict_path,
                   const std::unordered_map<std::string, std::unordered_set<int>>& postings,
                   std::string* err) {
  std::ofstream postings_out(postings_path, std::ios::binary);
  std::ofstream dict_out(dict_path);
  if (!postings_out || !dict_out) {
    if (err) {
      *err = "failed to open postings or dict file";
    }
    return false;
  }

  for (const auto& entry : postings) {
    std::vector<int> ids(entry.second.begin(), entry.second.end());
    std::sort(ids.begin(), ids.end());

    uint64_t hash = HashToken(entry.first);
    auto offset = postings_out.tellp();
    postings_out << hash << " ";
    for (size_t i = 0; i < ids.size(); ++i) {
      if (i > 0) {
        postings_out << ",";
      }
      postings_out << ids[i];
    }
    postings_out << "\n";
    auto endpos = postings_out.tellp();
    int64_t length = static_cast<int64_t>(endpos - offset);
    dict_out << hash << " " << static_cast<int64_t>(offset) << " " << length << "\n";
  }
  return true;
}

bool RebuildPostings(const std::string& out_dir,
                     const std::vector<std::string>& series_keys,
                     std::string* err) {
  std::unordered_map<std::string, std::unordered_set<int>> postings;
  std::unordered_map<std::string, std::string> tags;
  std::string metric;
  for (size_t id = 0; id < series_keys.size(); ++id) {
    if (!ParseSeriesKey(series_keys[id], &metric, &tags)) {
      continue;
    }
    for (const auto& kv : tags) {
      std::string token = kv.first + "=" + kv.second;
      postings[token].insert(static_cast<int>(id));
    }
  }
  return WritePostings(out_dir + "/postings.txt", out_dir + "/dict.txt", postings, err);
}

bool BuildRowOffsetsText(const std::string& path, std::vector<int64_t>* offsets, std::string* err) {
  std::ifstream in(path);
  if (!in) {
    if (err) {
      *err = "failed to open points file: " + path;
    }
    return false;
  }
  offsets->clear();
  std::string line;
  while (true) {
    auto pos = in.tellg();
    if (!std::getline(in, line)) {
      break;
    }
    offsets->push_back(static_cast<int64_t>(pos));
  }
  return true;
}

bool BuildRowOffsetsBinary(const std::string& path, std::vector<int64_t>* offsets, std::string* err) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (err) {
      *err = "failed to open points file: " + path;
    }
    return false;
  }
  offsets->clear();
  while (true) {
    auto pos = in.tellg();
    uint64_t id = 0;
    if (!ReadVarint(in, &id)) {
      break;
    }
    uint64_t ts = 0;
    if (!ReadVarint(in, &ts)) {
      if (err) {
        *err = "corrupt binary points file: " + path;
      }
      return false;
    }
    for (int i = 0; i < 5; ++i) {
      uint64_t bits = 0;
      in.read(reinterpret_cast<char*>(&bits), sizeof(bits));
      if (!in) {
        if (err) {
          *err = "corrupt binary points file: " + path;
        }
        return false;
      }
    }
    offsets->push_back(static_cast<int64_t>(pos));
  }
  return true;
}

bool BuildRowOffsets(const std::string& path,
                     bool binary_points,
                     std::vector<int64_t>* offsets,
                     std::string* err) {
  if (binary_points) {
    return BuildRowOffsetsBinary(path, offsets, err);
  }
  return BuildRowOffsetsText(path, offsets, err);
}

int64_t CountRows(const std::string& path, bool binary_points) {
  if (!binary_points) {
    std::ifstream in(path);
    if (!in) {
      return 0;
    }
    int64_t count = 0;
    std::string line;
    while (std::getline(in, line)) {
      ++count;
    }
    return count;
  }
  std::vector<int64_t> offsets;
  if (!BuildRowOffsetsBinary(path, &offsets, nullptr)) {
    return 0;
  }
  return static_cast<int64_t>(offsets.size());
}

bool WriteMeta(const std::string& dir,
               int64_t partition_seconds,
               bool binary_points,
               std::string* err) {
  std::ofstream out(dir + "/" + kMetaFile);
  if (!out) {
    if (err) {
      *err = "failed to open meta file";
    }
    return false;
  }
  out << "partition_seconds " << partition_seconds << "\n";
  out << "points_format " << (binary_points ? "binary" : "text") << "\n";
  return true;
}

bool LoadMeta(const std::string& dir, int64_t* partition_seconds, bool* binary_points) {
  std::ifstream in(dir + "/" + kMetaFile);
  if (!in) {
    return false;
  }
  std::string key;
  std::string value;
  bool found = false;
  while (in >> key >> value) {
    if (key == "partition_seconds") {
      *partition_seconds = std::stoll(value);
      found = true;
    } else if (key == "points_format") {
      *binary_points = (value == "binary");
      found = true;
    }
  }
  return found;
}

bool WritePartitions(const std::string& dir, const std::vector<int64_t>& buckets, std::string* err) {
  std::ofstream out(dir + "/" + kPartitionsFile);
  if (!out) {
    if (err) {
      *err = "failed to open partitions file";
    }
    return false;
  }
  for (int64_t bucket : buckets) {
    out << bucket << " part_" << bucket << "\n";
  }
  return true;
}

bool LoadPartitions(const std::string& dir, std::vector<int64_t>* buckets) {
  std::ifstream in(dir + "/" + kPartitionsFile);
  if (!in) {
    return false;
  }
  buckets->clear();
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    std::istringstream iss(line);
    int64_t bucket = 0;
    std::string name;
    if (iss >> bucket >> name) {
      buckets->push_back(bucket);
    }
  }
  return true;
}

bool WritePartitionData(const std::string& dir,
                        bool binary_points,
                        const std::vector<std::vector<Point>>& series_points,
                        std::string* err) {
  if (!EnsureDir(dir, err)) {
    return false;
  }
  const std::string points_path = PointsFilePath(dir, binary_points);
  const std::string forward_path = dir + "/forward_index.txt";

  std::ofstream points_out(points_path, binary_points ? std::ios::binary : std::ios::out);
  if (!points_out) {
    if (err) {
      *err = "failed to open points file: " + points_path;
    }
    return false;
  }
  std::ofstream forward_out(forward_path);
  if (!forward_out) {
    if (err) {
      *err = "failed to open forward index file: " + forward_path;
    }
    return false;
  }

  int64_t row = 0;
  for (size_t id = 0; id < series_points.size(); ++id) {
    const auto& vec = series_points[id];
    int64_t start = vec.empty() ? -1 : row;
    for (const auto& pt : vec) {
      if (binary_points) {
        if (!WriteBinaryPoint(points_out, static_cast<int>(id), pt)) {
          if (err) {
            *err = "failed to write binary point";
          }
          return false;
        }
      } else {
        points_out << id << " " << pt.timestamp;
        for (double v : pt.values) {
          points_out << " " << v;
        }
        points_out << "\n";
      }
      ++row;
    }
    int64_t end = vec.empty() ? -1 : row - 1;
    forward_out << id << " " << start << " " << end << "\n";
  }
  return true;
}

}  // namespace

bool Builder::Build(const std::string& input_path,
                    const std::string& out_dir,
                    const BuildOptions& options,
                    std::string* err) {
  std::ifstream input(input_path);
  if (!input) {
    if (err) {
      *err = "failed to open input file: " + input_path;
    }
    return false;
  }

  const int64_t partition_seconds =
      options.partition_seconds > 0 ? options.partition_seconds : kDefaultPartitionSeconds;
  const bool binary_points = options.binary_points;

  std::unordered_map<std::string, int> series_id_by_key;
  std::vector<std::string> series_keys;
  std::unordered_map<std::string, std::unordered_set<int>> postings;
  std::unordered_map<int64_t, std::vector<std::vector<Point>>> bucket_points;

  std::string line;
  int line_no = 0;
  while (std::getline(input, line)) {
    ++line_no;
    if (line.empty()) {
      continue;
    }
    ParsedLine parsed;
    if (!ParseLine(line, &parsed, err)) {
      if (err) {
        *err = "line " + std::to_string(line_no) + ": " + *err;
      }
      return false;
    }
    std::sort(parsed.tags.begin(), parsed.tags.end());
    std::string series_key = MakeSeriesKey(parsed.metric, parsed.tags);

    int series_id = 0;
    auto it = series_id_by_key.find(series_key);
    if (it == series_id_by_key.end()) {
      series_id = static_cast<int>(series_keys.size());
      series_id_by_key[series_key] = series_id;
      series_keys.push_back(series_key);
      for (auto& entry : bucket_points) {
        entry.second.resize(series_keys.size());
      }
    } else {
      series_id = it->second;
    }

    int64_t bucket = BucketFor(parsed.timestamp, partition_seconds);
    auto& series_vec = bucket_points[bucket];
    if (series_vec.size() < series_keys.size()) {
      series_vec.resize(series_keys.size());
    }
    series_vec[series_id].push_back(Point{parsed.timestamp, parsed.values});

    for (const auto& kv : parsed.tags) {
      std::string token = kv.first + "=" + kv.second;
      postings[token].insert(series_id);
    }
  }

  for (auto& bucket_entry : bucket_points) {
    for (auto& vec : bucket_entry.second) {
      std::sort(vec.begin(), vec.end(), [](const Point& a, const Point& b) {
        return a.timestamp < b.timestamp;
      });
    }
  }

  if (!EnsureDir(out_dir, err)) {
    return false;
  }

  if (!WriteMeta(out_dir, partition_seconds, binary_points, err)) {
    return false;
  }

  if (!WriteSeriesKeys(out_dir + "/serieskey.txt", series_keys, err)) {
    return false;
  }

  if (!WritePostings(out_dir + "/postings.txt", out_dir + "/dict.txt", postings, err)) {
    return false;
  }

  std::vector<int64_t> buckets;
  buckets.reserve(bucket_points.size());
  for (const auto& entry : bucket_points) {
    buckets.push_back(entry.first);
  }
  std::sort(buckets.begin(), buckets.end());
  if (!WritePartitions(out_dir, buckets, err)) {
    return false;
  }

  for (int64_t bucket : buckets) {
    const auto& series_vec = bucket_points[bucket];
    std::string dir = PartitionDir(out_dir, bucket);
    if (!WritePartitionData(dir, binary_points, series_vec, err)) {
      if (err) {
        *err = "bucket " + std::to_string(bucket) + ": " + *err;
      }
      return false;
    }
  }

  return true;
}

bool Ingest(const std::string& input_path, const std::string& out_dir, std::string* err) {
  std::ifstream input(input_path);
  if (!input) {
    if (err) {
      *err = "failed to open input file: " + input_path;
    }
    return false;
  }

  int64_t partition_seconds = kDefaultPartitionSeconds;
  bool binary_points = false;
  bool meta_loaded = LoadMeta(out_dir, &partition_seconds, &binary_points);

  std::vector<std::string> series_keys;
  std::unordered_map<std::string, int> series_id_by_key;
  if (!LoadSeriesKeys(out_dir + "/serieskey.txt", &series_keys, &series_id_by_key, err)) {
    return false;
  }

  std::vector<int64_t> existing_buckets;
  LoadPartitions(out_dir, &existing_buckets);
  if (existing_buckets.empty()) {
    if (std::filesystem::exists(out_dir + "/points.txt")) {
      existing_buckets.push_back(0);
    }
  }
  if (!meta_loaded) {
    if (std::filesystem::exists(out_dir + "/points.bin")) {
      binary_points = true;
    } else {
      for (int64_t bucket : existing_buckets) {
        std::string dir = (bucket == 0 && !std::filesystem::exists(PartitionDir(out_dir, bucket)) &&
                           std::filesystem::exists(out_dir + "/points.txt"))
                              ? out_dir
                              : PartitionDir(out_dir, bucket);
        if (std::filesystem::exists(PointsFilePath(dir, true))) {
          binary_points = true;
          break;
        }
      }
    }
  }
  if (!meta_loaded) {
    WriteMeta(out_dir, partition_seconds, binary_points, nullptr);
  }

  struct PartitionState {
    int64_t bucket = 0;
    std::string dir;
    std::vector<Range> ranges;
    int64_t row_count = 0;
    std::vector<std::vector<Point>> new_points;
  };

  std::unordered_map<int64_t, PartitionState> partitions;

  auto ensure_partition = [&](int64_t bucket) -> PartitionState& {
    auto it = partitions.find(bucket);
    if (it != partitions.end()) {
      return it->second;
    }
    PartitionState state;
    state.bucket = bucket;
    if (bucket == 0 && !std::filesystem::exists(PartitionDir(out_dir, bucket)) &&
        std::filesystem::exists(out_dir + "/points.txt")) {
      state.dir = out_dir;
    } else {
      state.dir = PartitionDir(out_dir, bucket);
    }
    std::string forward_path = state.dir + "/forward_index.txt";
    LoadForwardIndex(forward_path, &state.ranges, nullptr);
    state.row_count = CountRows(PointsFilePath(state.dir, binary_points), binary_points);
    state.new_points.resize(series_keys.size());
    auto result = partitions.emplace(bucket, std::move(state));
    return result.first->second;
  };

  std::string line;
  int line_no = 0;
  bool added_series = false;
  while (std::getline(input, line)) {
    ++line_no;
    if (line.empty()) {
      continue;
    }
    ParsedLine parsed;
    if (!ParseLine(line, &parsed, err)) {
      if (err) {
        *err = "line " + std::to_string(line_no) + ": " + *err;
      }
      return false;
    }
    std::sort(parsed.tags.begin(), parsed.tags.end());
    std::string series_key = MakeSeriesKey(parsed.metric, parsed.tags);

    int series_id = 0;
    auto it = series_id_by_key.find(series_key);
    if (it == series_id_by_key.end()) {
      series_id = static_cast<int>(series_keys.size());
      series_keys.push_back(series_key);
      series_id_by_key[series_key] = series_id;
      added_series = true;
      for (auto& entry : partitions) {
        entry.second.new_points.resize(series_keys.size());
        if (entry.second.ranges.size() < series_keys.size()) {
          entry.second.ranges.resize(series_keys.size());
        }
      }
    } else {
      series_id = it->second;
    }

    int64_t bucket = BucketFor(parsed.timestamp, partition_seconds);
    auto& partition = ensure_partition(bucket);
    if (partition.ranges.size() < series_keys.size()) {
      partition.ranges.resize(series_keys.size());
    }
    if (partition.new_points.size() < series_keys.size()) {
      partition.new_points.resize(series_keys.size());
    }
    partition.new_points[series_id].push_back(Point{parsed.timestamp, parsed.values});
  }

  if (added_series) {
    if (!WriteSeriesKeys(out_dir + "/serieskey.txt", series_keys, err)) {
      return false;
    }
    if (!RebuildPostings(out_dir, series_keys, err)) {
      return false;
    }
  }

  std::vector<int64_t> buckets;
  for (int64_t bucket : existing_buckets) {
    buckets.push_back(bucket);
  }
  for (const auto& entry : partitions) {
    buckets.push_back(entry.first);
  }
  if (!buckets.empty()) {
    std::sort(buckets.begin(), buckets.end());
    buckets.erase(std::unique(buckets.begin(), buckets.end()), buckets.end());
    WritePartitions(out_dir, buckets, nullptr);
  }

  for (auto& entry : partitions) {
    auto& part = entry.second;
    if (!EnsureDir(part.dir, err)) {
      return false;
    }
    std::string points_path = PointsFilePath(part.dir, binary_points);
    std::string forward_path = part.dir + "/forward_index.txt";

    std::ofstream points_out(points_path,
                             (binary_points ? std::ios::binary : std::ios::out) | std::ios::app);
    if (!points_out) {
      if (err) {
        *err = "failed to open points file: " + points_path;
      }
      return false;
    }

    for (auto& vec : part.new_points) {
      std::sort(vec.begin(), vec.end(), [](const Point& a, const Point& b) {
        return a.timestamp < b.timestamp;
      });
    }

    for (size_t id = 0; id < part.new_points.size(); ++id) {
      auto& vec = part.new_points[id];
      if (vec.empty()) {
        continue;
      }
      if (part.ranges[id].start_row < 0) {
        part.ranges[id].start_row = part.row_count;
      }
      for (const auto& pt : vec) {
        if (binary_points) {
          if (!WriteBinaryPoint(points_out, static_cast<int>(id), pt)) {
            if (err) {
              *err = "failed to write binary point";
            }
            return false;
          }
        } else {
          points_out << id << " " << pt.timestamp;
          for (double v : pt.values) {
            points_out << " " << v;
          }
          points_out << "\n";
        }
        ++part.row_count;
      }
      part.ranges[id].end_row = part.row_count - 1;
    }

    if (!WriteForwardIndex(forward_path, part.ranges, err)) {
      return false;
    }
  }

  return true;
}

bool DB::Open(const std::string& dir, std::string* err) {
  dir_ = dir;
  series_keys_.clear();
  dict_.clear();
  partitions_.clear();
  partition_buckets_.clear();
  posting_cache_.clear();

  const std::string series_path = dir_ + "/serieskey.txt";
  const std::string dict_path = dir_ + "/dict.txt";
  postings_path_ = dir_ + "/postings.txt";

  std::unordered_map<std::string, int> ignore_map;
  if (!LoadSeriesKeys(series_path, &series_keys_, &ignore_map, err)) {
    return false;
  }

  std::ifstream dict_in(dict_path);
  if (!dict_in) {
    if (err) {
      *err = "failed to open dict file: " + dict_path;
    }
    return false;
  }
  std::string line;
  while (std::getline(dict_in, line)) {
    if (line.empty()) {
      continue;
    }
    std::istringstream iss(line);
    uint64_t hash = 0;
    int64_t offset = 0;
    int64_t length = 0;
    if (!(iss >> hash >> offset >> length)) {
      continue;
    }
    dict_[hash] = DictEntry{offset, length};
  }

  partition_seconds_ = kDefaultPartitionSeconds;
  binary_points_ = false;
  bool meta_loaded = LoadMeta(dir_, &partition_seconds_, &binary_points_);

  std::vector<int64_t> buckets;
  if (!LoadPartitions(dir_, &buckets)) {
    buckets.clear();
    buckets.push_back(0);
  }
  std::sort(buckets.begin(), buckets.end());
  partition_buckets_ = buckets;

  if (!meta_loaded) {
    for (int64_t bucket : buckets) {
      std::string dir = (bucket == 0 && !std::filesystem::exists(PartitionDir(dir_, bucket)))
                            ? dir_
                            : PartitionDir(dir_, bucket);
      if (std::filesystem::exists(PointsFilePath(dir, true))) {
        binary_points_ = true;
        break;
      }
    }
  }

  for (int64_t bucket : buckets) {
    Partition part;
    part.bucket = bucket;
    if (bucket == 0 && !std::filesystem::exists(PartitionDir(dir_, bucket))) {
      part.dir = dir_;
    } else {
      part.dir = PartitionDir(dir_, bucket);
    }
    part.points_path = PointsFilePath(part.dir, binary_points_);
    std::string forward_path = part.dir + "/forward_index.txt";

    if (!LoadForwardIndex(forward_path, &part.ranges, err)) {
      return false;
    }
    if (!BuildRowOffsets(part.points_path, binary_points_, &part.row_offsets, err)) {
      return false;
    }
    partitions_[bucket] = std::move(part);
  }

  return true;
}

std::vector<int> DB::GetPosting(const std::string& token, std::string* err) const {
  {
    std::lock_guard<std::mutex> lock(cache_mu_);
    auto it = posting_cache_.find(token);
    if (it != posting_cache_.end()) {
      return it->second;
    }
  }

  uint64_t hash = HashToken(token);
  auto it = dict_.find(hash);
  if (it == dict_.end()) {
    return {};
  }

  std::ifstream postings_in(postings_path_, std::ios::binary);
  if (!postings_in) {
    if (err) {
      *err = "failed to open postings file: " + postings_path_;
    }
    return {};
  }

  postings_in.seekg(it->second.offset);
  std::string buffer(static_cast<size_t>(it->second.length), '\0');
  postings_in.read(&buffer[0], static_cast<std::streamsize>(buffer.size()));
  if (!postings_in) {
    if (err) {
      *err = "failed to read postings entry";
    }
    return {};
  }

  while (!buffer.empty() && (buffer.back() == '\n' || buffer.back() == '\r')) {
    buffer.pop_back();
  }

  auto space = buffer.find(' ');
  if (space == std::string::npos) {
    return {};
  }
  std::string ids_str = buffer.substr(space + 1);
  if (ids_str.empty()) {
    return {};
  }

  std::vector<int> ids;
  auto parts = SplitByChar(ids_str, ',');
  for (const auto& part : parts) {
    if (part.empty()) {
      continue;
    }
    ids.push_back(std::stoi(part));
  }
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

  {
    std::lock_guard<std::mutex> lock(cache_mu_);
    posting_cache_[token] = ids;
  }

  return ids;
}

std::vector<QueryResult> DB::QueryData(const Query& q, std::string* err) const {
  std::vector<QueryResult> results;
  if (q.start_time && q.end_time && q.start_time > q.end_time) {
    if (err) {
      *err = "start_time is greater than end_time";
    }
    return results;
  }

  std::vector<int> candidates;
  if (q.tags.empty()) {
    candidates.reserve(series_keys_.size());
    for (size_t i = 0; i < series_keys_.size(); ++i) {
      candidates.push_back(static_cast<int>(i));
    }
  } else {
    bool first = true;
    for (const auto& kv : q.tags) {
      std::string token = kv.first + "=" + kv.second;
      std::vector<int> posting = GetPosting(token, err);
      if (posting.empty()) {
        return results;
      }
      if (first) {
        candidates = std::move(posting);
        first = false;
      } else {
        std::vector<int> intersection;
        std::set_intersection(candidates.begin(), candidates.end(),
                              posting.begin(), posting.end(),
                              std::back_inserter(intersection));
        candidates.swap(intersection);
        if (candidates.empty()) {
          return results;
        }
      }
    }
  }

  std::vector<int> filtered;
  filtered.reserve(candidates.size());
  std::unordered_map<std::string, std::string> series_tags;
  std::string metric;
  for (int series_id : candidates) {
    if (series_id < 0 || series_id >= static_cast<int>(series_keys_.size())) {
      continue;
    }
    if (!ParseSeriesKey(series_keys_[series_id], &metric, &series_tags)) {
      continue;
    }
    if (!q.metric.empty() && metric != q.metric) {
      continue;
    }
    bool match = true;
    for (const auto& kv : q.tags) {
      auto it = series_tags.find(kv.first);
      if (it == series_tags.end() || it->second != kv.second) {
        match = false;
        break;
      }
    }
    if (match) {
      filtered.push_back(series_id);
    }
  }

  if (filtered.empty()) {
    return results;
  }

  std::vector<int64_t> buckets_to_scan;
  if (partition_buckets_.empty()) {
    return results;
  }
  if (q.start_time == 0 && q.end_time == 0) {
    buckets_to_scan = partition_buckets_;
  } else {
    int64_t start_bucket = q.start_time ? BucketFor(q.start_time, partition_seconds_)
                                        : partition_buckets_.front();
    int64_t end_bucket = q.end_time ? BucketFor(q.end_time, partition_seconds_)
                                    : partition_buckets_.back();
    if (start_bucket > end_bucket) {
      std::swap(start_bucket, end_bucket);
    }
    for (int64_t bucket = start_bucket; bucket <= end_bucket; ++bucket) {
      if (partitions_.find(bucket) != partitions_.end()) {
        buckets_to_scan.push_back(bucket);
      }
    }
  }

  if (buckets_to_scan.empty()) {
    return results;
  }

  int thread_count = q.threads > 0 ? q.threads : 1;
  if (thread_count > static_cast<int>(filtered.size())) {
    thread_count = static_cast<int>(filtered.size());
  }
  if (thread_count <= 1) {
    thread_count = 1;
  }

  std::mutex results_mu;
  auto worker = [&](size_t begin, size_t end) {
    std::vector<QueryResult> local_results;
    for (size_t idx = begin; idx < end; ++idx) {
      int series_id = filtered[idx];
      QueryResult result;
      result.series_key = series_keys_[series_id];

      for (int64_t bucket : buckets_to_scan) {
        auto pit = partitions_.find(bucket);
        if (pit == partitions_.end()) {
          continue;
        }
        const Partition& part = pit->second;
        if (series_id >= static_cast<int>(part.ranges.size())) {
          continue;
        }
        Range range = part.ranges[series_id];
        if (range.start_row < 0 || range.end_row < range.start_row) {
          continue;
        }
        if (range.start_row >= static_cast<int64_t>(part.row_offsets.size())) {
          continue;
        }

        std::ifstream points_in(part.points_path,
                                binary_points_ ? std::ios::binary : std::ios::in);
        if (!points_in) {
          continue;
        }
        points_in.seekg(part.row_offsets[range.start_row]);
        int64_t current = range.start_row;
        if (binary_points_) {
          while (current <= range.end_row) {
            int row_series_id = 0;
            Point point;
            if (!ReadBinaryPoint(points_in, &row_series_id, &point)) {
              break;
            }
            if (row_series_id == series_id) {
              bool within = true;
              if (q.start_time && point.timestamp < q.start_time) {
                within = false;
              }
              if (q.end_time && point.timestamp > q.end_time) {
                within = false;
              }
              if (within) {
                result.points.push_back(point);
              }
            }
            ++current;
          }
        } else {
          std::string line;
          while (current <= range.end_row && std::getline(points_in, line)) {
            int row_series_id = 0;
            Point point;
            if (ParsePointLine(line, &row_series_id, &point) && row_series_id == series_id) {
              bool within = true;
              if (q.start_time && point.timestamp < q.start_time) {
                within = false;
              }
              if (q.end_time && point.timestamp > q.end_time) {
                within = false;
              }
              if (within) {
                result.points.push_back(point);
              }
            }
            ++current;
          }
        }
      }

      if (!result.points.empty()) {
        std::sort(result.points.begin(), result.points.end(), [](const Point& a, const Point& b) {
          return a.timestamp < b.timestamp;
        });
        local_results.push_back(std::move(result));
      }
    }

    if (!local_results.empty()) {
      std::lock_guard<std::mutex> lock(results_mu);
      results.insert(results.end(),
                     std::make_move_iterator(local_results.begin()),
                     std::make_move_iterator(local_results.end()));
    }
  };

  if (thread_count == 1) {
    worker(0, filtered.size());
  } else {
    std::vector<std::thread> threads;
    size_t chunk = (filtered.size() + thread_count - 1) / thread_count;
    size_t begin = 0;
    for (int t = 0; t < thread_count; ++t) {
      size_t end = std::min(filtered.size(), begin + chunk);
      if (begin >= end) {
        break;
      }
      threads.emplace_back(worker, begin, end);
      begin = end;
    }
    for (auto& th : threads) {
      th.join();
    }
  }

  return results;
}

}  // namespace tsdb
