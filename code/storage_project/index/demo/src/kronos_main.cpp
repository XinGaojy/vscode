#include "kronos.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

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

double FindFieldValue(const tsdb::Point& point, const std::string& name, bool* found) {
  for (const auto& field : point.fields) {
    if (field.name == name) {
      if (found) {
        *found = true;
      }
      return field.value;
    }
  }
  if (found) {
    *found = false;
  }
  return 0.0;
}

void PrintUsage() {
  std::cout << "Usage:\n"
            << "  kronos init <cluster_dir> shards=<n> [online=<dir>] [mq=<dir>] "
               "[pangu=<dir>]\n"
            << "  kronos publish <cluster_dir> <input_file> [rollup=20,60,600,3600]\n"
            << "  kronos online_ingest <cluster_dir> [max=<n>]\n"
            << "  kronos offline_build <cluster_dir> [partition=<sec>] [format=orc|binary|text] "
               "[rollup=20,60,600,3600] [max=<n>]\n"
            << "  kronos query <cluster_dir> metric=<name> tag=<k=v> tag=<k=v> start=<ts> "
               "end=<ts> fields=a,b,c tier=hot,warm,cold resolution=<sec> exact=1\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    PrintUsage();
    return 1;
  }

  std::string cmd = argv[1];
  if (cmd == "init") {
    if (argc < 3) {
      PrintUsage();
      return 1;
    }
    kronos::ClusterConfig config;
    config.root_dir = argv[2];
    for (int i = 3; i < argc; ++i) {
      std::string arg = argv[i];
      auto pos = arg.find('=');
      if (pos == std::string::npos) {
        continue;
      }
      std::string key = arg.substr(0, pos);
      std::string value = arg.substr(pos + 1);
      if (key == "shards") {
        config.shards = std::stoi(value);
      } else if (key == "online") {
        config.online_dir = value;
      } else if (key == "mq") {
        config.mq_dir = value;
      } else if (key == "pangu") {
        config.pangu_dir = value;
      }
    }
    std::string err;
    if (!kronos::InitCluster(&config, &err)) {
      std::cerr << "Init failed: " << err << "\n";
      return 1;
    }
    std::cout << "Cluster initialized at " << config.root_dir << "\n";
    return 0;
  }

  if (cmd == "publish") {
    if (argc < 4) {
      PrintUsage();
      return 1;
    }
    std::string cluster_dir = argv[2];
    std::string input_path = argv[3];
    kronos::ClusterConfig config;
    std::string err;
    if (!kronos::LoadClusterConfig(cluster_dir, &config, &err)) {
      std::cerr << "Load config failed: " << err << "\n";
      return 1;
    }
    kronos::PublishOptions options;
    for (int i = 4; i < argc; ++i) {
      std::string arg = argv[i];
      auto pos = arg.find('=');
      if (pos == std::string::npos) {
        continue;
      }
      std::string key = arg.substr(0, pos);
      std::string value = arg.substr(pos + 1);
      if (key == "rollup") {
        auto parts = SplitByChar(value, ',');
        for (const auto& part : parts) {
          options.rollup_seconds.push_back(std::stoll(part));
        }
      }
    }
    if (!kronos::Publish(config, input_path, options, &err)) {
      std::cerr << "Publish failed: " << err << "\n";
      return 1;
    }
    std::cout << "Publish succeeded\n";
    return 0;
  }

  if (cmd == "online_ingest") {
    if (argc < 3) {
      PrintUsage();
      return 1;
    }
    std::string cluster_dir = argv[2];
    kronos::ClusterConfig config;
    std::string err;
    if (!kronos::LoadClusterConfig(cluster_dir, &config, &err)) {
      std::cerr << "Load config failed: " << err << "\n";
      return 1;
    }
    kronos::IngestOptions options;
    for (int i = 3; i < argc; ++i) {
      std::string arg = argv[i];
      auto pos = arg.find('=');
      if (pos == std::string::npos) {
        continue;
      }
      std::string key = arg.substr(0, pos);
      std::string value = arg.substr(pos + 1);
      if (key == "max") {
        options.max_messages = static_cast<size_t>(std::stoll(value));
      }
    }
    if (!kronos::OnlineIngest(config, options, &err)) {
      std::cerr << "Online ingest failed: " << err << "\n";
      return 1;
    }
    std::cout << "Online ingest succeeded\n";
    return 0;
  }

  if (cmd == "offline_build") {
    if (argc < 3) {
      PrintUsage();
      return 1;
    }
    std::string cluster_dir = argv[2];
    kronos::ClusterConfig config;
    std::string err;
    if (!kronos::LoadClusterConfig(cluster_dir, &config, &err)) {
      std::cerr << "Load config failed: " << err << "\n";
      return 1;
    }
    kronos::BuildOptions options;
    for (int i = 3; i < argc; ++i) {
      std::string arg = argv[i];
      auto pos = arg.find('=');
      if (pos == std::string::npos) {
        continue;
      }
      std::string key = arg.substr(0, pos);
      std::string value = arg.substr(pos + 1);
      if (key == "partition") {
        options.partition_seconds = std::stoll(value);
      } else if (key == "format") {
        if (value == "orc") {
          options.format = tsdb::Builder::PointsFormat::kOrc;
        } else if (value == "binary") {
          options.format = tsdb::Builder::PointsFormat::kBinary;
        } else if (value == "text") {
          options.format = tsdb::Builder::PointsFormat::kText;
        }
      } else if (key == "rollup") {
        auto parts = SplitByChar(value, ',');
        for (const auto& part : parts) {
          options.rollup_seconds.push_back(std::stoll(part));
        }
      } else if (key == "max") {
        options.max_messages = static_cast<size_t>(std::stoll(value));
      }
    }
    if (!kronos::OfflineBuild(config, options, &err)) {
      std::cerr << "Offline build failed: " << err << "\n";
      return 1;
    }
    std::cout << "Offline build succeeded\n";
    return 0;
  }

  if (cmd == "query") {
    if (argc < 3) {
      PrintUsage();
      return 1;
    }
    std::string cluster_dir = argv[2];
    kronos::ClusterConfig config;
    std::string err;
    if (!kronos::LoadClusterConfig(cluster_dir, &config, &err)) {
      std::cerr << "Load config failed: " << err << "\n";
      return 1;
    }
    kronos::QueryOptions options;
    for (int i = 3; i < argc; ++i) {
      std::string arg = argv[i];
      auto pos = arg.find('=');
      if (pos == std::string::npos) {
        continue;
      }
      std::string key = arg.substr(0, pos);
      std::string value = arg.substr(pos + 1);
      if (key == "metric") {
        options.query.metric = value;
      } else if (key == "tag") {
        auto eq = value.find('=');
        if (eq != std::string::npos) {
          options.query.tags.emplace_back(value.substr(0, eq), value.substr(eq + 1));
        }
      } else if (key == "start") {
        options.query.start_time = std::stoll(value);
      } else if (key == "end") {
        options.query.end_time = std::stoll(value);
      } else if (key == "fields") {
        options.query.fields = SplitByChar(value, ',');
      } else if (key == "tier" || key == "tiers") {
        options.query.tiers = SplitByChar(value, ',');
      } else if (key == "resolution") {
        options.query.resolution = std::stoll(value);
      } else if (key == "exact") {
        options.exact_series = (value == "1" || value == "true");
      }
    }

    auto results = kronos::QueryCluster(config, options, &err);
    if (!err.empty() && results.empty()) {
      std::cerr << "Query failed: " << err << "\n";
      return 1;
    }
    if (results.empty()) {
      std::cout << "No results\n";
      return 0;
    }
    for (const auto& res : results) {
      std::cout << "series: " << res.series_key << "\n";
      for (const auto& pt : res.points) {
        std::cout << "  timestamp=" << pt.timestamp;
        if (options.query.fields.empty()) {
          for (const auto& field : pt.fields) {
            std::cout << " " << field.name << "=" << field.value;
          }
        } else {
          for (const auto& name : options.query.fields) {
            bool found = false;
            double value = FindFieldValue(pt, name, &found);
            std::cout << " " << name << "=" << (found ? value : 0.0);
          }
        }
        std::cout << "\n";
      }
    }
    return 0;
  }

  PrintUsage();
  return 1;
}
