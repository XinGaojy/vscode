#pragma once

#include "tsdb.h"

#include <cstdint>
#include <string>
#include <vector>

namespace kronos {

struct ClusterConfig {
  int shards = 1;
  std::string root_dir;
  std::string online_dir;
  std::string mq_dir;
  std::string pangu_dir;
  std::string latest_version;
};

struct PublishOptions {
  std::vector<int64_t> rollup_seconds;
};

struct IngestOptions {
  size_t max_messages = 10000;
};

struct BuildOptions {
  int64_t partition_seconds = 3600;
  tsdb::Builder::PointsFormat format = tsdb::Builder::PointsFormat::kOrc;
  std::vector<int64_t> rollup_seconds;
  size_t max_messages = 10000;
};

struct QueryOptions {
  tsdb::Query query;
  bool exact_series = false;
};

bool InitCluster(ClusterConfig* config, std::string* err);
bool LoadClusterConfig(const std::string& root_dir, ClusterConfig* config, std::string* err);
bool SaveClusterConfig(const ClusterConfig& config, std::string* err);

bool Publish(const ClusterConfig& config,
             const std::string& input_path,
             const PublishOptions& options,
             std::string* err);

bool OnlineIngest(const ClusterConfig& config, const IngestOptions& options, std::string* err);

bool OfflineBuild(const ClusterConfig& config, const BuildOptions& options, std::string* err);

std::vector<tsdb::QueryResult> QueryCluster(const ClusterConfig& config,
                                            const QueryOptions& options,
                                            std::string* err);

}  // namespace kronos
