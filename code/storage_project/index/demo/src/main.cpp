#include "tsdb.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

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
            << "  tsdb build <input_file> <output_dir> [partition=<seconds>] "
               "[format=orc|binary|text] [rollup=20,60,600,3600] "
               "[tier_hot=<seconds>] [tier_warm=<seconds>]\n"
            << "  tsdb ingest <input_file> <output_dir>\n"
            << "  tsdb merge <output_dir>\n"
            << "  tsdb query <output_dir> metric=<name> tag=<k=v> tag=<k=v> start=<ts> end=<ts> "
               "fields=min,max,avg,sum,count threads=<n> tier=hot,warm resolution=<seconds>\n";
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

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    PrintUsage();
    return 1;
  }

  std::string cmd = argv[1];
  if (cmd == "build") {
    if (argc < 4) {
      PrintUsage();
      return 1;
    }
    std::string input_path = argv[2];
    std::string output_dir = argv[3];
    tsdb::Builder::BuildOptions options;
    options.format = tsdb::Builder::PointsFormat::kBinary;
    for (int i = 4; i < argc; ++i) {
      std::string arg = argv[i];
      auto pos = arg.find('=');
      if (pos == std::string::npos) {
        continue;
      }
      std::string key = arg.substr(0, pos);
      std::string value = arg.substr(pos + 1);
      if (key == "partition") {
        try {
          options.partition_seconds = std::stoll(value);
        } catch (...) {
          std::cerr << "Invalid partition seconds\n";
          return 1;
        }
      } else if (key == "format") {
        if (value == "orc") {
          options.format = tsdb::Builder::PointsFormat::kOrc;
        } else if (value == "binary") {
          options.format = tsdb::Builder::PointsFormat::kBinary;
        } else if (value == "text") {
          options.format = tsdb::Builder::PointsFormat::kText;
        } else {
          std::cerr << "Invalid format (use orc, binary, or text)\n";
          return 1;
        }
      } else if (key == "binary") {
        options.format = (value == "1" || value == "true")
                             ? tsdb::Builder::PointsFormat::kBinary
                             : tsdb::Builder::PointsFormat::kText;
      } else if (key == "rollup") {
        auto parts = SplitByChar(value, ',');
        for (const auto& part : parts) {
          try {
            options.rollup_seconds.push_back(std::stoll(part));
          } catch (...) {
            std::cerr << "Invalid rollup seconds\n";
            return 1;
          }
        }
      } else if (key == "tier_hot") {
        try {
          options.tier_hot_seconds = std::stoll(value);
        } catch (...) {
          std::cerr << "Invalid tier_hot seconds\n";
          return 1;
        }
      } else if (key == "tier_warm") {
        try {
          options.tier_warm_seconds = std::stoll(value);
        } catch (...) {
          std::cerr << "Invalid tier_warm seconds\n";
          return 1;
        }
      }
    }
    tsdb::Builder builder;
    std::string err;
    if (!builder.Build(input_path, output_dir, options, &err)) {
      std::cerr << "Build failed: " << err << "\n";
      return 1;
    }
    std::cout << "Build succeeded. Output: " << output_dir << "\n";
    return 0;
  }

  if (cmd == "ingest") {
    if (argc < 4) {
      PrintUsage();
      return 1;
    }
    std::string input_path = argv[2];
    std::string output_dir = argv[3];
    std::string err;
    if (!tsdb::Ingest(input_path, output_dir, &err)) {
      std::cerr << "Ingest failed: " << err << "\n";
      return 1;
    }
    std::cout << "Ingest succeeded. Output: " << output_dir << "\n";
    return 0;
  }

  if (cmd == "merge") {
    if (argc < 3) {
      PrintUsage();
      return 1;
    }
    std::string output_dir = argv[2];
    std::string err;
    if (!tsdb::Merge(output_dir, &err)) {
      std::cerr << "Merge failed: " << err << "\n";
      return 1;
    }
    std::cout << "Merge succeeded. Output: " << output_dir << "\n";
    return 0;
  }

  if (cmd == "query") {
    if (argc < 3) {
      PrintUsage();
      return 1;
    }
    std::string output_dir = argv[2];
    tsdb::Query query;

    for (int i = 3; i < argc; ++i) {
      std::string arg = argv[i];
      auto pos = arg.find('=');
      if (pos == std::string::npos) {
        continue;
      }
      std::string key = arg.substr(0, pos);
      std::string value = arg.substr(pos + 1);
      if (key == "metric") {
        query.metric = value;
      } else if (key == "tag") {
        auto eq = value.find('=');
        if (eq != std::string::npos) {
          query.tags.emplace_back(value.substr(0, eq), value.substr(eq + 1));
        }
      } else if (key == "start") {
        try {
          query.start_time = std::stoll(value);
        } catch (...) {
          std::cerr << "Invalid start time\n";
          return 1;
        }
      } else if (key == "end") {
        try {
          query.end_time = std::stoll(value);
        } catch (...) {
          std::cerr << "Invalid end time\n";
          return 1;
        }
      } else if (key == "fields") {
        query.fields = SplitByChar(value, ',');
      } else if (key == "threads") {
        try {
          query.threads = std::stoi(value);
        } catch (...) {
          std::cerr << "Invalid threads value\n";
          return 1;
        }
      } else if (key == "tier" || key == "tiers") {
        auto parts = SplitByChar(value, ',');
        for (const auto& part : parts) {
          query.tiers.push_back(part);
        }
      } else if (key == "resolution") {
        try {
          query.resolution = std::stoll(value);
        } catch (...) {
          std::cerr << "Invalid resolution value\n";
          return 1;
        }
      }
    }

    if (query.resolution > 0) {
      std::string rollup_dir = output_dir + "/rollup_" + std::to_string(query.resolution);
      if (std::filesystem::exists(rollup_dir)) {
        output_dir = rollup_dir;
      } else {
        std::cerr << "Rollup directory not found: " << rollup_dir << "\n";
        return 1;
      }
    }

    tsdb::DB db;
    std::string err;
    if (!db.Open(output_dir, &err)) {
      std::cerr << "Open failed: " << err << "\n";
      return 1;
    }

    auto results = db.QueryData(query, &err);
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
        if (query.fields.empty()) {
          for (const auto& field : pt.fields) {
            std::cout << " " << field.name << "=" << field.value;
          }
        } else {
          for (const auto& name : query.fields) {
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
