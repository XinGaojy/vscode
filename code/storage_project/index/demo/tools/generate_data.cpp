#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct TagPool {
  std::vector<std::string> domains = {"beijing", "shanghai", "shenzhen", "guangzhou"};
  std::vector<std::string> zones = {"x", "y", "z"};
};

std::string HostFor(int series_id) {
  int host = (series_id % 50) + 1;
  return "10.0.0." + std::to_string(host);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 6) {
    std::cerr << "Usage: tsdb_gen <output> <series_count> <points_per_series> <start_ts> <step>\n";
    return 1;
  }

  const std::string output_path = argv[1];
  const int series_count = std::stoi(argv[2]);
  const int points_per_series = std::stoi(argv[3]);
  const int64_t start_ts = std::stoll(argv[4]);
  const int64_t step = std::stoll(argv[5]);

  std::ofstream out(output_path);
  if (!out) {
    std::cerr << "Failed to open output file: " << output_path << "\n";
    return 1;
  }

  TagPool pool;
  for (int series_id = 0; series_id < series_count; ++series_id) {
    std::string metric = (series_id % 2 == 0) ? "cpu" : "mem";
    std::string host = HostFor(series_id);
    std::string domain = pool.domains[series_id % pool.domains.size()];
    std::string zone = pool.zones[series_id % pool.zones.size()];

    double base = static_cast<double>((series_id % 10) + 1);
    double min_v = base;
    double max_v = base + 1;
    double avg_v = base + 2;
    double sum_v = base + 3;
    double count_v = base + 4;

    for (int i = 0; i < points_per_series; ++i) {
      int64_t ts = start_ts + static_cast<int64_t>(i) * step;
      out << metric << " host=" << host
          << " domain=" << domain
          << " zone=" << zone
          << " timestamp=" << ts
          << " " << min_v
          << " " << max_v
          << " " << avg_v
          << " " << sum_v
          << " " << count_v
          << "\n";
    }
  }

  std::cout << "Generated " << series_count << " series x "
            << points_per_series << " points to " << output_path << "\n";
  return 0;
}
