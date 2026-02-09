#include "tsdb.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>

#ifdef TSDB_ENABLE_ORC
#include <orc/OrcFile.hh>
#endif

namespace tsdb {
namespace {

constexpr int64_t kDefaultPartitionSeconds = 3600;
constexpr const char* kMetaFile = "meta.txt";
constexpr const char* kPartitionsFile = "partitions.txt";
constexpr const char* kPointsTextFile = "points.txt";
constexpr const char* kPointsBinaryFile = "points.bin";
constexpr const char* kPointsOrcFile = "points.orc";
constexpr const char* kDeltasFile = "deltas.txt";

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

const char* PointsFormatName(tsdb::Builder::PointsFormat format) {
  switch (format) {
    case tsdb::Builder::PointsFormat::kOrc:
      return "orc";
    case tsdb::Builder::PointsFormat::kBinary:
      return "binary";
    case tsdb::Builder::PointsFormat::kText:
      return "text";
  }
  return "binary";
}

bool ParsePointsFormat(const std::string& value, tsdb::Builder::PointsFormat* out) {
  if (value == "orc") {
    *out = tsdb::Builder::PointsFormat::kOrc;
    return true;
  }
  if (value == "binary") {
    *out = tsdb::Builder::PointsFormat::kBinary;
    return true;
  }
  if (value == "text") {
    *out = tsdb::Builder::PointsFormat::kText;
    return true;
  }
  return false;
}

std::string PointsFilePath(const std::string& dir, tsdb::Builder::PointsFormat format) {
  const char* name = kPointsBinaryFile;
  switch (format) {
    case tsdb::Builder::PointsFormat::kOrc:
      name = kPointsOrcFile;
      break;
    case tsdb::Builder::PointsFormat::kBinary:
      name = kPointsBinaryFile;
      break;
    case tsdb::Builder::PointsFormat::kText:
      name = kPointsTextFile;
      break;
  }
  return dir + "/" + name;
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

bool EnsureForwardIndexSize(const std::string& path, size_t series_count, std::string* err) {
  std::vector<Range> ranges;
  if (std::filesystem::exists(path)) {
    if (!LoadForwardIndex(path, &ranges, err)) {
      return false;
    }
  }
  if (ranges.size() < series_count) {
    ranges.resize(series_count, Range{-1, -1});
    return WriteForwardIndex(path, ranges, err);
  }
  if (!std::filesystem::exists(path)) {
    ranges.assign(series_count, Range{-1, -1});
    return WriteForwardIndex(path, ranges, err);
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
                     tsdb::Builder::PointsFormat format,
                     std::vector<int64_t>* offsets,
                     std::string* err) {
  switch (format) {
    case tsdb::Builder::PointsFormat::kBinary:
      return BuildRowOffsetsBinary(path, offsets, err);
    case tsdb::Builder::PointsFormat::kText:
      return BuildRowOffsetsText(path, offsets, err);
    case tsdb::Builder::PointsFormat::kOrc:
      offsets->clear();
      return true;
  }
  return false;
}

int64_t CountRows(const std::string& path, tsdb::Builder::PointsFormat format) {
  if (format == tsdb::Builder::PointsFormat::kText) {
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
  if (format == tsdb::Builder::PointsFormat::kBinary) {
    std::vector<int64_t> offsets;
    if (!BuildRowOffsetsBinary(path, &offsets, nullptr)) {
      return 0;
    }
    return static_cast<int64_t>(offsets.size());
  }
#ifdef TSDB_ENABLE_ORC
  if (format == tsdb::Builder::PointsFormat::kOrc) {
    try {
      std::unique_ptr<orc::InputStream> inStream = orc::readLocalFile(path);
      orc::ReaderOptions options;
      std::unique_ptr<orc::Reader> reader = orc::createReader(std::move(inStream), options);
      return static_cast<int64_t>(reader->getNumberOfRows());
    } catch (...) {
      return 0;
    }
  }
#endif
  return 0;
}

bool WriteMeta(const std::string& dir,
               int64_t partition_seconds,
               tsdb::Builder::PointsFormat format,
               std::string* err) {
  std::ofstream out(dir + "/" + kMetaFile);
  if (!out) {
    if (err) {
      *err = "failed to open meta file";
    }
    return false;
  }
  out << "partition_seconds " << partition_seconds << "\n";
  out << "points_format " << PointsFormatName(format) << "\n";
  return true;
}

bool LoadMeta(const std::string& dir,
              int64_t* partition_seconds,
              tsdb::Builder::PointsFormat* format) {
  std::ifstream in(dir + "/" + kMetaFile);
  if (!in) {
    return false;
  }
  std::string key;
  std::string value;
  bool found_partition = false;
  bool found_format = false;
  while (in >> key >> value) {
    if (key == "partition_seconds") {
      *partition_seconds = std::stoll(value);
      found_partition = true;
    } else if (key == "points_format") {
      if (ParsePointsFormat(value, format)) {
        found_format = true;
      }
    }
  }
  return found_partition && found_format;
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

std::string DeltasPath(const std::string& dir) {
  return dir + "/" + kDeltasFile;
}

bool LoadDeltas(const std::string& dir, std::vector<std::string>* deltas) {
  std::ifstream in(DeltasPath(dir));
  deltas->clear();
  if (!in) {
    return true;
  }
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    deltas->push_back(line);
  }
  return true;
}

bool WriteDeltas(const std::string& dir,
                 const std::vector<std::string>& deltas,
                 std::string* err) {
  std::ofstream out(DeltasPath(dir));
  if (!out) {
    if (err) {
      *err = "failed to open deltas file: " + DeltasPath(dir);
    }
    return false;
  }
  for (const auto& name : deltas) {
    out << name << "\n";
  }
  return true;
}

bool AppendDelta(const std::string& dir, const std::string& name, std::string* err) {
  std::ofstream out(DeltasPath(dir), std::ios::app);
  if (!out) {
    if (err) {
      *err = "failed to append deltas file: " + DeltasPath(dir);
    }
    return false;
  }
  out << name << "\n";
  return true;
}

std::string MakeDeltaFileName(int64_t bucket) {
  static std::atomic<uint64_t> counter{0};
  const auto now = std::chrono::system_clock::now().time_since_epoch();
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  const uint64_t seq = counter.fetch_add(1);
  return "points_delta_" + std::to_string(bucket) + "_" + std::to_string(ms) + "_" +
         std::to_string(seq) + ".orc";
}

#ifdef TSDB_ENABLE_ORC
bool WriteOrcFile(const std::string& points_path,
                  const std::vector<std::vector<Point>>& series_points,
                  std::vector<Range>* ranges,
                  std::string* err) {
  try {
    std::unique_ptr<orc::Type> schema = orc::Type::buildTypeFromString(
        "struct<series_id:int,timestamp:bigint,min:double,max:double,avg:double,sum:double,"
        "count:double>");
    orc::WriterOptions options;
    std::unique_ptr<orc::OutputStream> outStream = orc::writeLocalFile(points_path);
    std::unique_ptr<orc::Writer> writer = orc::createWriter(*schema, outStream.get(), options);

    constexpr uint64_t kBatchSize = 1024;
    std::unique_ptr<orc::ColumnVectorBatch> batch = writer->createRowBatch(kBatchSize);
    auto* root = dynamic_cast<orc::StructVectorBatch*>(batch.get());
    if (!root || root->fields.size() != 7) {
      if (err) {
        *err = "orc schema mismatch for points file";
      }
      return false;
    }

    auto* series_col = dynamic_cast<orc::LongVectorBatch*>(root->fields[0]);
    auto* ts_col = dynamic_cast<orc::LongVectorBatch*>(root->fields[1]);
    auto* min_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[2]);
    auto* max_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[3]);
    auto* avg_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[4]);
    auto* sum_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[5]);
    auto* count_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[6]);
    if (!series_col || !ts_col || !min_col || !max_col || !avg_col || !sum_col || !count_col) {
      if (err) {
        *err = "orc column batch cast failed";
      }
      return false;
    }

    auto set_batch_size = [&](uint64_t n) {
      root->numElements = n;
      series_col->numElements = n;
      ts_col->numElements = n;
      min_col->numElements = n;
      max_col->numElements = n;
      avg_col->numElements = n;
      sum_col->numElements = n;
      count_col->numElements = n;
    };

    if (ranges) {
      ranges->assign(series_points.size(), Range{-1, -1});
    }

    int64_t row = 0;
    uint64_t idx = 0;
    for (size_t id = 0; id < series_points.size(); ++id) {
      const auto& vec = series_points[id];
      int64_t start = vec.empty() ? -1 : row;
      for (const auto& pt : vec) {
        series_col->data[idx] = static_cast<int64_t>(id);
        ts_col->data[idx] = static_cast<int64_t>(pt.timestamp);
        min_col->data[idx] = pt.values[0];
        max_col->data[idx] = pt.values[1];
        avg_col->data[idx] = pt.values[2];
        sum_col->data[idx] = pt.values[3];
        count_col->data[idx] = pt.values[4];
        ++idx;
        ++row;
        if (idx == kBatchSize) {
          set_batch_size(idx);
          writer->add(*batch);
          idx = 0;
        }
      }
      int64_t end = vec.empty() ? -1 : row - 1;
      if (ranges) {
        (*ranges)[id] = Range{start, end};
      }
    }

    if (idx > 0) {
      set_batch_size(idx);
      writer->add(*batch);
    }
    writer->close();
    return true;
  } catch (const std::exception& ex) {
    if (err) {
      *err = std::string("orc write failed: ") + ex.what();
    }
    return false;
  } catch (...) {
    if (err) {
      *err = "orc write failed";
    }
    return false;
  }
}

bool WritePartitionDataOrc(const std::string& points_path,
                           std::ofstream& forward_out,
                           const std::vector<std::vector<Point>>& series_points,
                           std::string* err) {
  std::vector<Range> ranges;
  if (!WriteOrcFile(points_path, series_points, &ranges, err)) {
    return false;
  }
  for (size_t id = 0; id < ranges.size(); ++id) {
    forward_out << id << " " << ranges[id].start_row << " " << ranges[id].end_row << "\n";
  }
  return true;
}

bool ReadOrcFilePoints(const std::string& path,
                       std::vector<std::vector<Point>>* series_points,
                       std::string* err) {
  try {
    std::unique_ptr<orc::InputStream> inStream = orc::readLocalFile(path);
    orc::ReaderOptions options;
    std::unique_ptr<orc::Reader> reader = orc::createReader(std::move(inStream), options);
    orc::RowReaderOptions row_options;
    std::unique_ptr<orc::RowReader> row_reader = reader->createRowReader(row_options);
    std::unique_ptr<orc::ColumnVectorBatch> batch = row_reader->createRowBatch(1024);

    auto* root = dynamic_cast<orc::StructVectorBatch*>(batch.get());
    if (!root || root->fields.size() != 7) {
      if (err) {
        *err = "orc schema mismatch for points file";
      }
      return false;
    }
    auto* series_col = dynamic_cast<orc::LongVectorBatch*>(root->fields[0]);
    auto* ts_col = dynamic_cast<orc::LongVectorBatch*>(root->fields[1]);
    auto* min_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[2]);
    auto* max_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[3]);
    auto* avg_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[4]);
    auto* sum_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[5]);
    auto* count_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[6]);
    if (!series_col || !ts_col || !min_col || !max_col || !avg_col || !sum_col || !count_col) {
      if (err) {
        *err = "orc column batch cast failed";
      }
      return false;
    }

    while (row_reader->next(*batch)) {
      const uint64_t n = batch->numElements;
      for (uint64_t i = 0; i < n; ++i) {
        int series_id = static_cast<int>(series_col->data[i]);
        if (series_id < 0) {
          continue;
        }
        if (series_id >= static_cast<int>(series_points->size())) {
          series_points->resize(static_cast<size_t>(series_id) + 1);
        }
        Point point;
        point.timestamp = static_cast<int64_t>(ts_col->data[i]);
        point.values[0] = min_col->data[i];
        point.values[1] = max_col->data[i];
        point.values[2] = avg_col->data[i];
        point.values[3] = sum_col->data[i];
        point.values[4] = count_col->data[i];
        (*series_points)[static_cast<size_t>(series_id)].push_back(point);
      }
    }
    return true;
  } catch (const std::exception& ex) {
    if (err) {
      *err = std::string("orc read failed: ") + ex.what();
    }
    return false;
  } catch (...) {
    if (err) {
      *err = "orc read failed";
    }
    return false;
  }
}

bool ScanOrcFile(const std::string& path,
                 const tsdb::Query& q,
                 const std::vector<int>& index_by_series,
                 const std::array<bool, 5>& field_mask,
                 std::unordered_map<int, std::vector<Point>>* out,
                 int64_t* rows_scanned,
                 std::string* err) {
  try {
    std::unique_ptr<orc::InputStream> inStream = orc::readLocalFile(path);
    orc::ReaderOptions options;
    std::unique_ptr<orc::Reader> reader = orc::createReader(std::move(inStream), options);
    orc::RowReaderOptions row_options;
    std::list<std::string> include_cols;
    include_cols.push_back("series_id");
    include_cols.push_back("timestamp");
    if (field_mask[0]) {
      include_cols.push_back("min");
    }
    if (field_mask[1]) {
      include_cols.push_back("max");
    }
    if (field_mask[2]) {
      include_cols.push_back("avg");
    }
    if (field_mask[3]) {
      include_cols.push_back("sum");
    }
    if (field_mask[4]) {
      include_cols.push_back("count");
    }
    row_options.include(include_cols);
    std::unique_ptr<orc::RowReader> row_reader = reader->createRowReader(row_options);
    std::unique_ptr<orc::ColumnVectorBatch> batch = row_reader->createRowBatch(1024);

    auto* root = dynamic_cast<orc::StructVectorBatch*>(batch.get());
    if (!root || root->fields.size() != 7) {
      if (err) {
        *err = "orc schema mismatch for points file";
      }
      return false;
    }
    auto* series_col = dynamic_cast<orc::LongVectorBatch*>(root->fields[0]);
    auto* ts_col = dynamic_cast<orc::LongVectorBatch*>(root->fields[1]);
    auto* min_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[2]);
    auto* max_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[3]);
    auto* avg_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[4]);
    auto* sum_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[5]);
    auto* count_col = dynamic_cast<orc::DoubleVectorBatch*>(root->fields[6]);
    if (!series_col || !ts_col || !min_col || !max_col || !avg_col || !sum_col || !count_col) {
      if (err) {
        *err = "orc column batch cast failed";
      }
      return false;
    }

    while (row_reader->next(*batch)) {
      const uint64_t n = batch->numElements;
      if (rows_scanned) {
        *rows_scanned += static_cast<int64_t>(n);
      }
      for (uint64_t i = 0; i < n; ++i) {
        int series_id = static_cast<int>(series_col->data[i]);
        if (series_id < 0 || series_id >= static_cast<int>(index_by_series.size())) {
          continue;
        }
        if (index_by_series[series_id] < 0) {
          continue;
        }
        int64_t ts = static_cast<int64_t>(ts_col->data[i]);
        if (q.start_time && ts < q.start_time) {
          continue;
        }
        if (q.end_time && ts > q.end_time) {
          continue;
        }
        Point point;
        point.timestamp = ts;
        if (field_mask[0]) {
          point.values[0] = min_col->data[i];
        }
        if (field_mask[1]) {
          point.values[1] = max_col->data[i];
        }
        if (field_mask[2]) {
          point.values[2] = avg_col->data[i];
        }
        if (field_mask[3]) {
          point.values[3] = sum_col->data[i];
        }
        if (field_mask[4]) {
          point.values[4] = count_col->data[i];
        }
        (*out)[series_id].push_back(point);
      }
    }
    return true;
  } catch (const std::exception& ex) {
    if (err) {
      *err = std::string("orc read failed: ") + ex.what();
    }
    return false;
  } catch (...) {
    if (err) {
      *err = "orc read failed";
    }
    return false;
  }
}
#endif

bool WritePartitionData(const std::string& dir,
                        tsdb::Builder::PointsFormat format,
                        const std::vector<std::vector<Point>>& series_points,
                        std::string* err) {
  if (!EnsureDir(dir, err)) {
    return false;
  }
  const std::string points_path = PointsFilePath(dir, format);
  const std::string forward_path = dir + "/forward_index.txt";

  std::ofstream forward_out(forward_path);
  if (!forward_out) {
    if (err) {
      *err = "failed to open forward index file: " + forward_path;
    }
    return false;
  }

  if (format == tsdb::Builder::PointsFormat::kOrc) {
#ifdef TSDB_ENABLE_ORC
    return WritePartitionDataOrc(points_path, forward_out, series_points, err);
#else
    if (err) {
      *err = "orc format requested but ORC support is not enabled";
    }
    return false;
#endif
  }

  const bool binary_points = (format == tsdb::Builder::PointsFormat::kBinary);
  std::ofstream points_out(points_path, binary_points ? std::ios::binary : std::ios::out);
  if (!points_out) {
    if (err) {
      *err = "failed to open points file: " + points_path;
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

#ifdef TSDB_ENABLE_ORC
bool IngestOrc(const std::string& input_path,
               const std::string& out_dir,
               int64_t partition_seconds,
               std::vector<std::string>* series_keys,
               std::unordered_map<std::string, int>* series_id_by_key,
               const std::vector<int64_t>& existing_buckets,
               std::string* err) {
  std::ifstream input(input_path);
  if (!input) {
    if (err) {
      *err = "failed to open input file: " + input_path;
    }
    return false;
  }

  std::unordered_map<int64_t, std::vector<std::vector<Point>>> bucket_points;
  bool added_series = false;

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
    auto it = series_id_by_key->find(series_key);
    if (it == series_id_by_key->end()) {
      series_id = static_cast<int>(series_keys->size());
      (*series_id_by_key)[series_key] = series_id;
      series_keys->push_back(series_key);
      added_series = true;
      for (auto& entry : bucket_points) {
        entry.second.resize(series_keys->size());
      }
    } else {
      series_id = it->second;
    }

    int64_t bucket = BucketFor(parsed.timestamp, partition_seconds);
    auto& series_vec = bucket_points[bucket];
    if (series_vec.size() < series_keys->size()) {
      series_vec.resize(series_keys->size());
    }
    series_vec[series_id].push_back(Point{parsed.timestamp, parsed.values});
  }

  for (auto& bucket_entry : bucket_points) {
    for (auto& vec : bucket_entry.second) {
      std::sort(vec.begin(), vec.end(), [](const Point& a, const Point& b) {
        return a.timestamp < b.timestamp;
      });
    }
  }

  if (added_series) {
    if (!WriteSeriesKeys(out_dir + "/serieskey.txt", *series_keys, err)) {
      return false;
    }
    if (!RebuildPostings(out_dir, *series_keys, err)) {
      return false;
    }
  }

  std::vector<int64_t> buckets;
  buckets.reserve(existing_buckets.size() + bucket_points.size());
  for (int64_t bucket : existing_buckets) {
    buckets.push_back(bucket);
  }
  for (const auto& entry : bucket_points) {
    buckets.push_back(entry.first);
  }
  if (!buckets.empty()) {
    std::sort(buckets.begin(), buckets.end());
    buckets.erase(std::unique(buckets.begin(), buckets.end()), buckets.end());
    WritePartitions(out_dir, buckets, nullptr);
  }

  for (auto& entry : bucket_points) {
    const int64_t bucket = entry.first;
    auto& series_vec = entry.second;

    std::string dir;
    if (bucket == 0 && !std::filesystem::exists(PartitionDir(out_dir, bucket)) &&
        std::filesystem::exists(PointsFilePath(out_dir, tsdb::Builder::PointsFormat::kOrc))) {
      dir = out_dir;
    } else {
      dir = PartitionDir(out_dir, bucket);
    }

    if (!EnsureDir(dir, err)) {
      return false;
    }
    if (!EnsureForwardIndexSize(dir + "/forward_index.txt", series_keys->size(), err)) {
      return false;
    }

    // ORC is append-unfriendly, so ingest writes per-bucket delta ORC files for merge compaction.
    const std::string delta_name = MakeDeltaFileName(bucket);
    const std::string delta_path = dir + "/" + delta_name;
    if (!WriteOrcFile(delta_path, series_vec, nullptr, err)) {
      return false;
    }
    if (!AppendDelta(dir, delta_name, err)) {
      return false;
    }
  }

  return true;
}
#endif

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
  const auto format = options.format;
#ifndef TSDB_ENABLE_ORC
  if (format == tsdb::Builder::PointsFormat::kOrc) {
    if (err) {
      *err = "orc format requested but ORC support is not enabled";
    }
    return false;
  }
#endif

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

  if (!WriteMeta(out_dir, partition_seconds, format, err)) {
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
    if (!WritePartitionData(dir, format, series_vec, err)) {
      if (err) {
        *err = "bucket " + std::to_string(bucket) + ": " + *err;
      }
      return false;
    }
    if (format == tsdb::Builder::PointsFormat::kOrc) {
      if (!WriteDeltas(dir, {}, err)) {
        if (err) {
          *err = "bucket " + std::to_string(bucket) + ": " + *err;
        }
        return false;
      }
    }
  }

  return true;
}

bool Ingest(const std::string& input_path, const std::string& out_dir, std::string* err) {
  int64_t partition_seconds = kDefaultPartitionSeconds;
  auto format = tsdb::Builder::PointsFormat::kBinary;
  bool meta_loaded = LoadMeta(out_dir, &partition_seconds, &format);

  std::vector<std::string> series_keys;
  std::unordered_map<std::string, int> series_id_by_key;
  if (!LoadSeriesKeys(out_dir + "/serieskey.txt", &series_keys, &series_id_by_key, err)) {
    return false;
  }

  std::vector<int64_t> existing_buckets;
  LoadPartitions(out_dir, &existing_buckets);
  if (existing_buckets.empty()) {
    if (std::filesystem::exists(out_dir + "/points.txt") ||
        std::filesystem::exists(out_dir + "/points.bin") ||
        std::filesystem::exists(out_dir + "/points.orc")) {
      existing_buckets.push_back(0);
    }
  }
  if (!meta_loaded) {
    for (int64_t bucket : existing_buckets) {
      std::string dir = (bucket == 0 && !std::filesystem::exists(PartitionDir(out_dir, bucket)) &&
                         (std::filesystem::exists(out_dir + "/points.txt") ||
                          std::filesystem::exists(out_dir + "/points.bin") ||
                          std::filesystem::exists(out_dir + "/points.orc")))
                            ? out_dir
                            : PartitionDir(out_dir, bucket);
      if (std::filesystem::exists(PointsFilePath(dir, tsdb::Builder::PointsFormat::kOrc))) {
        format = tsdb::Builder::PointsFormat::kOrc;
        break;
      }
      if (std::filesystem::exists(PointsFilePath(dir, tsdb::Builder::PointsFormat::kBinary))) {
        format = tsdb::Builder::PointsFormat::kBinary;
        break;
      }
      if (std::filesystem::exists(PointsFilePath(dir, tsdb::Builder::PointsFormat::kText))) {
        format = tsdb::Builder::PointsFormat::kText;
        break;
      }
    }
  }
  if (!meta_loaded) {
    WriteMeta(out_dir, partition_seconds, format, nullptr);
  }

  if (format == tsdb::Builder::PointsFormat::kOrc) {
#ifndef TSDB_ENABLE_ORC
    if (err) {
      *err = "orc ingest requested but ORC support is not enabled";
    }
    return false;
#else
    return IngestOrc(input_path, out_dir, partition_seconds, &series_keys, &series_id_by_key,
                     existing_buckets, err);
#endif
  }

  std::ifstream input(input_path);
  if (!input) {
    if (err) {
      *err = "failed to open input file: " + input_path;
    }
    return false;
  }
  const bool binary_points = (format == tsdb::Builder::PointsFormat::kBinary);

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
    state.row_count = CountRows(PointsFilePath(state.dir, format), format);
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
    std::string points_path = PointsFilePath(part.dir, format);
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

bool Merge(const std::string& out_dir, std::string* err) {
  // Merge compacts ORC base + deltas into a new base file and clears deltas.
  int64_t partition_seconds = kDefaultPartitionSeconds;
  auto format = tsdb::Builder::PointsFormat::kBinary;
  bool meta_loaded = LoadMeta(out_dir, &partition_seconds, &format);

  std::vector<int64_t> buckets;
  if (!LoadPartitions(out_dir, &buckets)) {
    buckets.clear();
    buckets.push_back(0);
  }

  if (!meta_loaded) {
    for (int64_t bucket : buckets) {
      std::string dir = (bucket == 0 && !std::filesystem::exists(PartitionDir(out_dir, bucket)))
                            ? out_dir
                            : PartitionDir(out_dir, bucket);
      if (std::filesystem::exists(PointsFilePath(dir, tsdb::Builder::PointsFormat::kOrc))) {
        format = tsdb::Builder::PointsFormat::kOrc;
        break;
      }
    }
  }

  if (format != tsdb::Builder::PointsFormat::kOrc) {
    if (err) {
      *err = "merge is only supported for orc format";
    }
    return false;
  }

#ifndef TSDB_ENABLE_ORC
  if (err) {
    *err = "orc merge requested but ORC support is not enabled";
  }
  return false;
#else
  std::vector<std::string> series_keys;
  std::unordered_map<std::string, int> ignore_map;
  if (!LoadSeriesKeys(out_dir + "/serieskey.txt", &series_keys, &ignore_map, err)) {
    return false;
  }

  bool merged_any = false;
  for (int64_t bucket : buckets) {
    std::string dir;
    if (bucket == 0 && !std::filesystem::exists(PartitionDir(out_dir, bucket)) &&
        std::filesystem::exists(PointsFilePath(out_dir, tsdb::Builder::PointsFormat::kOrc))) {
      dir = out_dir;
    } else {
      dir = PartitionDir(out_dir, bucket);
    }

    std::vector<std::string> deltas;
    LoadDeltas(dir, &deltas);

    const std::string base_path = PointsFilePath(dir, tsdb::Builder::PointsFormat::kOrc);
    bool has_base = std::filesystem::exists(base_path);
    bool has_deltas = !deltas.empty();
    if (!has_base && !has_deltas) {
      continue;
    }

    std::vector<std::vector<Point>> series_points(series_keys.size());
    if (has_base) {
      if (!ReadOrcFilePoints(base_path, &series_points, err)) {
        return false;
      }
    }
    for (const auto& delta : deltas) {
      const std::string delta_path = dir + "/" + delta;
      if (!std::filesystem::exists(delta_path)) {
        continue;
      }
      if (!ReadOrcFilePoints(delta_path, &series_points, err)) {
        return false;
      }
    }

    for (auto& vec : series_points) {
      std::sort(vec.begin(), vec.end(), [](const Point& a, const Point& b) {
        return a.timestamp < b.timestamp;
      });
    }

    const std::string tmp_path = base_path + ".tmp";
    std::vector<Range> ranges;
    if (!WriteOrcFile(tmp_path, series_points, &ranges, err)) {
      return false;
    }
    if (!WriteForwardIndex(dir + "/forward_index.txt", ranges, err)) {
      return false;
    }

    std::error_code ec;
    std::filesystem::rename(tmp_path, base_path, ec);
    if (ec) {
      std::filesystem::remove(base_path, ec);
      std::filesystem::rename(tmp_path, base_path, ec);
      if (ec) {
        if (err) {
          *err = "failed to replace base orc file: " + base_path;
        }
        return false;
      }
    }

    for (const auto& delta : deltas) {
      std::filesystem::remove(dir + "/" + delta, ec);
    }
    if (!WriteDeltas(dir, {}, err)) {
      return false;
    }
    merged_any = true;
  }

  if (!merged_any) {
    return true;
  }
  return true;
#endif
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
  format_ = tsdb::Builder::PointsFormat::kBinary;
  bool meta_loaded = LoadMeta(dir_, &partition_seconds_, &format_);

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
      if (std::filesystem::exists(PointsFilePath(dir, tsdb::Builder::PointsFormat::kOrc))) {
        format_ = tsdb::Builder::PointsFormat::kOrc;
        break;
      }
      if (std::filesystem::exists(PointsFilePath(dir, tsdb::Builder::PointsFormat::kBinary))) {
        format_ = tsdb::Builder::PointsFormat::kBinary;
        break;
      }
      if (std::filesystem::exists(PointsFilePath(dir, tsdb::Builder::PointsFormat::kText))) {
        format_ = tsdb::Builder::PointsFormat::kText;
        break;
      }
    }
  }
#ifndef TSDB_ENABLE_ORC
  if (format_ == tsdb::Builder::PointsFormat::kOrc) {
    if (err) {
      *err = "orc format detected but ORC support is not enabled";
    }
    return false;
  }
#endif

  for (int64_t bucket : buckets) {
    Partition part;
    part.bucket = bucket;
    if (bucket == 0 && !std::filesystem::exists(PartitionDir(dir_, bucket))) {
      part.dir = dir_;
    } else {
      part.dir = PartitionDir(dir_, bucket);
    }
    part.points_path = PointsFilePath(part.dir, format_);
    std::string forward_path = part.dir + "/forward_index.txt";

    if (!LoadForwardIndex(forward_path, &part.ranges, err)) {
      return false;
    }
    if (!BuildRowOffsets(part.points_path, format_, &part.row_offsets, err)) {
      return false;
    }
    LoadDeltas(part.dir, &part.delta_paths);
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
  metrics_.queries.fetch_add(1, std::memory_order_relaxed);
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
    struct Posting {
      std::vector<int> ids;
    };
    std::vector<Posting> postings;
    postings.reserve(q.tags.size());
    for (const auto& kv : q.tags) {
      std::string token = kv.first + "=" + kv.second;
      std::vector<int> posting = GetPosting(token, err);
      if (posting.empty()) {
        return results;
      }
      postings.push_back(Posting{std::move(posting)});
    }
    std::sort(postings.begin(), postings.end(),
              [](const Posting& a, const Posting& b) { return a.ids.size() < b.ids.size(); });
    candidates = std::move(postings[0].ids);
    for (size_t i = 1; i < postings.size(); ++i) {
      std::vector<int> intersection;
      std::set_intersection(candidates.begin(), candidates.end(),
                            postings[i].ids.begin(), postings[i].ids.end(),
                            std::back_inserter(intersection));
      candidates.swap(intersection);
      if (candidates.empty()) {
        return results;
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

#ifndef TSDB_ENABLE_ORC
  if (format_ == tsdb::Builder::PointsFormat::kOrc) {
    if (err) {
      *err = "orc format requested but ORC support is not enabled";
    }
    return results;
  }
#endif

  std::array<bool, 5> field_mask{};
  if (q.fields.empty()) {
    field_mask.fill(true);
  } else {
    field_mask.fill(false);
    for (const auto& field : q.fields) {
      if (field == "min") {
        field_mask[0] = true;
      } else if (field == "max") {
        field_mask[1] = true;
      } else if (field == "avg") {
        field_mask[2] = true;
      } else if (field == "sum") {
        field_mask[3] = true;
      } else if (field == "count") {
        field_mask[4] = true;
      }
    }
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

  metrics_.series_scanned.fetch_add(static_cast<int64_t>(filtered.size()),
                                    std::memory_order_relaxed);

  if (format_ == tsdb::Builder::PointsFormat::kOrc) {
#ifdef TSDB_ENABLE_ORC
    // ORC path scans each bucket once (base + deltas), then merges series results.
    std::vector<int> index_by_series(series_keys_.size(), -1);
    for (size_t i = 0; i < filtered.size(); ++i) {
      int series_id = filtered[i];
      if (series_id >= 0 && series_id < static_cast<int>(series_keys_.size())) {
        index_by_series[series_id] = static_cast<int>(i);
      }
    }

    struct LocalOrcResult {
      std::unordered_map<int, std::vector<Point>> points_by_series;
      int64_t rows_scanned = 0;
    };

    int thread_count = q.threads > 0 ? q.threads : 1;
    if (thread_count > static_cast<int>(buckets_to_scan.size())) {
      thread_count = static_cast<int>(buckets_to_scan.size());
    }
    if (thread_count <= 1) {
      thread_count = 1;
    }

    std::vector<LocalOrcResult> locals(static_cast<size_t>(thread_count));
    std::atomic<bool> scan_failed{false};
    std::mutex err_mu;
    std::string scan_err;

    auto record_error = [&](const std::string& msg) {
      bool expected = false;
      if (scan_failed.compare_exchange_strong(expected, true)) {
        std::lock_guard<std::mutex> lock(err_mu);
        scan_err = msg;
      }
    };

    auto bucket_worker = [&](int worker_id, size_t begin, size_t end) {
      auto& local = locals[static_cast<size_t>(worker_id)];
      for (size_t i = begin; i < end; ++i) {
        if (scan_failed.load()) {
          return;
        }
        int64_t bucket = buckets_to_scan[i];
        auto pit = partitions_.find(bucket);
        if (pit == partitions_.end()) {
          continue;
        }
        const Partition& part = pit->second;
        std::vector<std::string> files;
        if (std::filesystem::exists(part.points_path)) {
          files.push_back(part.points_path);
        }
        for (const auto& delta : part.delta_paths) {
          files.push_back(part.dir + "/" + delta);
        }
        for (const auto& path : files) {
          if (!std::filesystem::exists(path)) {
            continue;
          }
          int64_t scanned = 0;
          std::string local_err;
          if (!ScanOrcFile(path, q, index_by_series, field_mask, &local.points_by_series,
                           &scanned, &local_err)) {
            record_error(local_err);
            return;
          }
          local.rows_scanned += scanned;
        }
      }
    };

    if (thread_count == 1) {
      bucket_worker(0, 0, buckets_to_scan.size());
    } else {
      std::vector<std::thread> threads;
      size_t chunk = (buckets_to_scan.size() + thread_count - 1) / thread_count;
      size_t begin = 0;
      for (int t = 0; t < thread_count; ++t) {
        size_t end = std::min(buckets_to_scan.size(), begin + chunk);
        if (begin >= end) {
          break;
        }
        threads.emplace_back(bucket_worker, t, begin, end);
        begin = end;
      }
      for (auto& th : threads) {
        th.join();
      }
    }

    if (scan_failed.load()) {
      if (err) {
        std::lock_guard<std::mutex> lock(err_mu);
        *err = scan_err;
      }
      return results;
    }

    std::vector<QueryResult> merged(filtered.size());
    for (size_t i = 0; i < filtered.size(); ++i) {
      merged[i].series_key = series_keys_[filtered[i]];
    }

    int64_t total_rows_scanned = 0;
    for (auto& local : locals) {
      total_rows_scanned += local.rows_scanned;
      for (auto& entry : local.points_by_series) {
        int series_id = entry.first;
        if (series_id < 0 || series_id >= static_cast<int>(index_by_series.size())) {
          continue;
        }
        int idx = index_by_series[series_id];
        if (idx < 0) {
          continue;
        }
        auto& dest = merged[static_cast<size_t>(idx)].points;
        dest.insert(dest.end(),
                    std::make_move_iterator(entry.second.begin()),
                    std::make_move_iterator(entry.second.end()));
      }
    }
    metrics_.rows_scanned.fetch_add(total_rows_scanned, std::memory_order_relaxed);

    for (auto& result : merged) {
      if (!result.points.empty()) {
        std::sort(result.points.begin(), result.points.end(),
                  [](const Point& a, const Point& b) { return a.timestamp < b.timestamp; });
        results.push_back(std::move(result));
      }
    }
    return results;
#else
    if (err) {
      *err = "orc format requested but ORC support is not enabled";
    }
    return results;
#endif
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
    std::vector<int> series_subset;
    series_subset.reserve(end - begin);
    std::vector<int> index_by_series(series_keys_.size(), -1);
    for (size_t idx = begin; idx < end; ++idx) {
      int series_id = filtered[idx];
      if (series_id < 0 || series_id >= static_cast<int>(series_keys_.size())) {
        continue;
      }
      index_by_series[series_id] = static_cast<int>(series_subset.size());
      series_subset.push_back(series_id);
    }
    if (series_subset.empty()) {
      return;
    }

    std::vector<QueryResult> series_results(series_subset.size());
    for (size_t i = 0; i < series_subset.size(); ++i) {
      series_results[i].series_key = series_keys_[series_subset[i]];
    }

    auto apply_mask = [&](Point* point) {
      if (!field_mask[0]) {
        point->values[0] = 0.0;
      }
      if (!field_mask[1]) {
        point->values[1] = 0.0;
      }
      if (!field_mask[2]) {
        point->values[2] = 0.0;
      }
      if (!field_mask[3]) {
        point->values[3] = 0.0;
      }
      if (!field_mask[4]) {
        point->values[4] = 0.0;
      }
    };

    int64_t local_rows_scanned = 0;
    const bool binary_points = (format_ == tsdb::Builder::PointsFormat::kBinary);
    for (int64_t bucket : buckets_to_scan) {
      auto pit = partitions_.find(bucket);
      if (pit == partitions_.end()) {
        continue;
      }
      const Partition& part = pit->second;
      std::ifstream points_in(part.points_path,
                              binary_points ? std::ios::binary : std::ios::in);
      if (!points_in) {
        continue;
      }
      for (size_t i = 0; i < series_subset.size(); ++i) {
        int series_id = series_subset[i];
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
        points_in.clear();
        points_in.seekg(part.row_offsets[range.start_row]);
        int64_t current = range.start_row;
        if (binary_points) {
          while (current <= range.end_row) {
            int row_series_id = 0;
            Point point;
            if (!ReadBinaryPoint(points_in, &row_series_id, &point)) {
              break;
            }
            ++local_rows_scanned;
            if (row_series_id == series_id) {
              if (q.start_time && point.timestamp < q.start_time) {
                ++current;
                continue;
              }
              if (q.end_time && point.timestamp > q.end_time) {
                ++current;
                continue;
              }
              apply_mask(&point);
              series_results[i].points.push_back(point);
            }
            ++current;
          }
        } else {
          std::string line;
          while (current <= range.end_row && std::getline(points_in, line)) {
            int row_series_id = 0;
            Point point;
            ++local_rows_scanned;
            if (ParsePointLine(line, &row_series_id, &point) && row_series_id == series_id) {
              if (q.start_time && point.timestamp < q.start_time) {
                ++current;
                continue;
              }
              if (q.end_time && point.timestamp > q.end_time) {
                ++current;
                continue;
              }
              apply_mask(&point);
              series_results[i].points.push_back(point);
            }
            ++current;
          }
        }
      }
    }

    metrics_.rows_scanned.fetch_add(local_rows_scanned, std::memory_order_relaxed);

    std::vector<QueryResult> local_results;
    for (auto& result : series_results) {
      if (!result.points.empty()) {
        std::sort(result.points.begin(), result.points.end(),
                  [](const Point& a, const Point& b) { return a.timestamp < b.timestamp; });
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

void DB::ResetMetrics() {
  metrics_.queries.store(0, std::memory_order_relaxed);
  metrics_.series_scanned.store(0, std::memory_order_relaxed);
  metrics_.rows_scanned.store(0, std::memory_order_relaxed);
}

void DB::PrintMetrics(std::ostream& out) const {
  out << "queries=" << metrics_.queries.load(std::memory_order_relaxed)
      << " series_scanned=" << metrics_.series_scanned.load(std::memory_order_relaxed)
      << " rows_scanned=" << metrics_.rows_scanned.load(std::memory_order_relaxed) << "\n";
}

}  // namespace tsdb
