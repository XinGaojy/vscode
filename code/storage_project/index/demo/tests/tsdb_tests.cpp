#include "kronos.h"
#include "tsdb.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void AssertTrue(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "ASSERT FAILED: " << msg << "\n";
    ++g_failures;
  }
}

std::string MakeTempDir(const std::string& prefix) {
  auto base = std::filesystem::temp_directory_path();
  auto now = std::chrono::system_clock::now().time_since_epoch();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  std::string dir = (base / (prefix + std::to_string(ms))).string();
  std::filesystem::create_directories(dir);
  return dir;
}

bool WriteFile(const std::string& path, const std::vector<std::string>& lines) {
  std::ofstream out(path);
  if (!out) {
    return false;
  }
  for (const auto& line : lines) {
    out << line << "\n";
  }
  return true;
}

void TestDynamicFields() {
  std::string dir = MakeTempDir("tsdb_dynamic_");
  std::string input = dir + "/input.txt";
  std::vector<std::string> lines = {
      "cpu host=10.0.0.1 domain=beijing timestamp=100 temp=1.5 hum=2.5",
      "cpu host=10.0.0.1 domain=beijing timestamp=101 temp=1.6 hum=2.6"};
  AssertTrue(WriteFile(input, lines), "write input");

  tsdb::Builder builder;
  tsdb::Builder::BuildOptions options;
  options.format = tsdb::Builder::PointsFormat::kBinary;
  std::string err;
  AssertTrue(builder.Build(input, dir + "/out", options, &err), "build: " + err);

  tsdb::DB db;
  AssertTrue(db.Open(dir + "/out", &err), "open: " + err);
  tsdb::Query q;
  q.metric = "cpu";
  q.tags.emplace_back("host", "10.0.0.1");
  q.start_time = 100;
  q.end_time = 101;
  auto results = db.QueryData(q, &err);
  AssertTrue(!results.empty(), "query results not empty");
  if (results.empty()) {
    return;
  }
  AssertTrue(!results[0].points.empty(), "query points not empty");
  if (results[0].points.empty()) {
    return;
  }

  bool has_temp = false;
  bool has_hum = false;
  for (const auto& field : results[0].points[0].fields) {
    if (field.name == "temp") {
      has_temp = true;
    }
    if (field.name == "hum") {
      has_hum = true;
    }
  }
  AssertTrue(has_temp && has_hum, "dynamic fields present");

  tsdb::Query q2 = q;
  q2.fields = {"temp"};
  auto results2 = db.QueryData(q2, &err);
  AssertTrue(!results2.empty(), "filtered query results not empty");
  if (results2.empty()) {
    return;
  }
  if (results2[0].points.empty()) {
    AssertTrue(false, "filtered points not empty");
    return;
  }
  AssertTrue(results2[0].points[0].fields.size() == 1, "filtered fields size");
  AssertTrue(results2[0].points[0].fields[0].name == "temp", "filtered field name");
}

void TestWalReplay() {
  std::string dir = MakeTempDir("tsdb_wal_");
  std::string input = dir + "/input.txt";
  std::vector<std::string> lines = {
      "cpu host=10.0.0.2 domain=beijing timestamp=200 min=1 max=2 avg=1.5 sum=3 count=2"};
  AssertTrue(WriteFile(input, lines), "write base input");

  tsdb::Builder builder;
  tsdb::Builder::BuildOptions options;
  options.format = tsdb::Builder::PointsFormat::kBinary;
  std::string err;
  AssertTrue(builder.Build(input, dir + "/out", options, &err), "build base: " + err);

  std::string wal_path = dir + "/out/wal.log";
  std::vector<std::string> wal_lines = {
      "cpu host=10.0.0.2 domain=beijing timestamp=201 min=2 max=3 avg=2.5 sum=5 count=2"};
  AssertTrue(WriteFile(wal_path, wal_lines), "write wal");

  tsdb::DB db;
  AssertTrue(db.Open(dir + "/out", &err), "open with wal: " + err);

  tsdb::Query q;
  q.metric = "cpu";
  q.tags.emplace_back("host", "10.0.0.2");
  q.start_time = 200;
  q.end_time = 201;
  auto results = db.QueryData(q, &err);
  AssertTrue(!results.empty(), "wal query results");
  if (results.empty()) {
    return;
  }
  AssertTrue(results[0].points.size() >= 2, "wal replay applied");
  AssertTrue(!std::filesystem::exists(wal_path), "wal cleared");
}

void TestPostingsDelta() {
  std::string dir = MakeTempDir("tsdb_delta_");
  std::string input = dir + "/input.txt";
  std::vector<std::string> lines = {
      "cpu host=10.0.0.3 domain=beijing timestamp=300 min=1 max=2 avg=1.5 sum=3 count=2"};
  AssertTrue(WriteFile(input, lines), "write base input");

  tsdb::Builder builder;
  tsdb::Builder::BuildOptions options;
  options.format = tsdb::Builder::PointsFormat::kBinary;
  std::string err;
  AssertTrue(builder.Build(input, dir + "/out", options, &err), "build base: " + err);

  std::string ingest_input = dir + "/ingest.txt";
  std::vector<std::string> ingest_lines = {
      "cpu host=10.0.0.4 domain=beijing timestamp=301 min=2 max=3 avg=2.5 sum=5 count=2"};
  AssertTrue(WriteFile(ingest_input, ingest_lines), "write ingest input");
  AssertTrue(tsdb::Ingest(ingest_input, dir + "/out", &err), "ingest: " + err);
  AssertTrue(std::filesystem::exists(dir + "/out/postings_delta.txt"),
             "postings delta exists");

  tsdb::DB db;
  AssertTrue(db.Open(dir + "/out", &err), "open after ingest: " + err);
  tsdb::Query q;
  q.metric = "cpu";
  q.tags.emplace_back("host", "10.0.0.4");
  q.start_time = 301;
  q.end_time = 301;
  auto results = db.QueryData(q, &err);
  AssertTrue(!results.empty(), "delta query results");
  if (results.empty()) {
    return;
  }
}

void TestKronosPipeline() {
  std::string dir = MakeTempDir("kronos_cluster_");
  kronos::ClusterConfig config;
  config.root_dir = dir;
  config.shards = 2;
  std::string err;
  AssertTrue(kronos::InitCluster(&config, &err), "init cluster: " + err);

  std::string input = dir + "/input.txt";
  std::vector<std::string> lines = {
      "cpu host=10.0.0.10 domain=beijing timestamp=400 min=1 max=2 avg=1.5 sum=3 count=2",
      "cpu host=10.0.0.11 domain=beijing timestamp=401 min=2 max=3 avg=2.5 sum=5 count=2"};
  AssertTrue(WriteFile(input, lines), "write publish input");
  kronos::PublishOptions publish_opts;
  AssertTrue(kronos::Publish(config, input, publish_opts, &err), "publish: " + err);

  kronos::IngestOptions ingest_opts;
  ingest_opts.max_messages = 10;
  AssertTrue(kronos::OnlineIngest(config, ingest_opts, &err), "online ingest: " + err);

  kronos::QueryOptions q;
  q.query.metric = "cpu";
  q.query.tags.emplace_back("host", "10.0.0.10");
  q.query.tags.emplace_back("domain", "beijing");
  q.query.start_time = 400;
  q.query.end_time = 400;
  q.exact_series = true;
  auto results = kronos::QueryCluster(config, q, &err);
  AssertTrue(!results.empty(), "kronos query results");
}

}  // namespace

int main() {
  TestDynamicFields();
  TestWalReplay();
  TestPostingsDelta();
  TestKronosPipeline();
  if (g_failures == 0) {
    std::cout << "All tests passed\n";
    return 0;
  }
  std::cerr << g_failures << " tests failed\n";
  return 1;
}
