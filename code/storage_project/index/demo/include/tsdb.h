#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tsdb {

struct FieldValue {
  std::string name;
  double value = 0.0;
};

struct Point {
  int64_t timestamp = 0;
  std::vector<FieldValue> fields;
};

struct Range {
  int64_t start_row = -1;
  int64_t end_row = -1;
};

struct Query {
  std::string metric;
  std::vector<std::pair<std::string, std::string>> tags;
  int64_t start_time = 0;
  int64_t end_time = 0;
  std::vector<std::string> fields;
  int threads = 1;
  std::vector<std::string> tiers;
  int64_t resolution = 0;
};

struct QueryResult {
  std::string series_key;
  std::vector<Point> points;
};

class Builder {
 public:
  enum class PointsFormat { kOrc, kBinary, kText };

  struct BuildOptions {
    int64_t partition_seconds = 3600;
    PointsFormat format = PointsFormat::kBinary;
    std::vector<int64_t> rollup_seconds;
    int64_t tier_hot_seconds = 43200;
    int64_t tier_warm_seconds = 259200;
  };

  bool Build(const std::string& input_path,
             const std::string& out_dir,
             const BuildOptions& options,
             std::string* err);
};

bool Ingest(const std::string& input_path, const std::string& out_dir, std::string* err);
bool Merge(const std::string& out_dir, std::string* err);

class DB {
 public:
  bool Open(const std::string& dir, std::string* err);
  std::vector<QueryResult> QueryData(const Query& q, std::string* err) const;
  void ResetMetrics();
  void PrintMetrics(std::ostream& out) const;

 private:
  struct DictEntry {
    int64_t offset = 0;
    int64_t length = 0;
    bool complement = false;
  };

  struct Partition {
    int64_t bucket = 0;
    std::string dir;
    std::string points_path;
    std::vector<std::string> delta_paths;
    std::vector<Range> ranges;
    std::vector<int64_t> row_offsets;
  };

  std::string dir_;
  std::vector<std::string> series_keys_;
  std::unordered_map<uint64_t, DictEntry> dict_;
  std::unordered_map<uint64_t, DictEntry> dict_delta_;
  std::unordered_map<int64_t, Partition> partitions_;
  int64_t partition_seconds_ = 3600;
  Builder::PointsFormat format_ = Builder::PointsFormat::kBinary;
  std::vector<int64_t> partition_buckets_;
  std::string postings_path_;
  std::string postings_delta_path_;
  size_t postings_series_count_ = 0;
  std::unordered_map<int64_t, std::string> tier_by_bucket_;

  mutable std::unordered_map<std::string, std::vector<int>> posting_cache_;
  mutable std::mutex cache_mu_;

  std::vector<int> GetPosting(const std::string& token, std::string* err) const;

  struct Metrics {
    std::atomic<int64_t> queries{0};
    std::atomic<int64_t> series_scanned{0};
    std::atomic<int64_t> rows_scanned{0};
  };
  mutable Metrics metrics_;
};

}  // namespace tsdb
