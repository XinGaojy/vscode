#include <grpcpp/grpcpp.h>
#include "dfs_proto/file.grpc.pb.h"
#include "common/logger.hpp"
#include <fstream>
namespace dfs_proto = file;

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "Usage:\n"
              << "  " << argv[0] << " upload <file>\n"
              << "  " << argv[0] << " download <hash> <dst>\n"
              << "  " << argv[0] << " delete <hash>\n";
    return 1;
  }
  auto channel = grpc::CreateChannel("localhost:3000", grpc::InsecureChannelCredentials());
  auto stub = dfs_proto::FileService::NewStub(channel);

  std::string mode = argv[1];
  if (mode == "upload") {
    std::ifstream in(argv[2], std::ios::binary);
    std::string data((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
    dfs_proto::UploadRequest req;
    req.set_name(argv[2]);
    req.set_data(data);
    dfs_proto::UploadResponse rep;
    grpc::ClientContext ctx;
    grpc::Status st = stub->Upload(&ctx, req, &rep);
    if (st.ok()) std::cout << "upload ok, hash=" << rep.hash() << "\n";
    else std::cerr << "upload fail: " << st.error_message() << "\n";
  }
  else if (mode == "download") {
    dfs_proto::DownloadRequest req;
    req.set_hash(argv[2]);
    dfs_proto::DownloadResponse rep;
    grpc::ClientContext ctx;
    grpc::Status st = stub->Download(&ctx, req, &rep);
    if (st.ok()) {
      std::ofstream out(argv[3], std::ios::binary);
      out.write(rep.data().data(), rep.data().size());
      std::cout << "download ok -> " << argv[3] << "\n";
    } else std::cerr << "download fail: " << st.error_message() << "\n";
  }
  else if (mode == "delete") {
    dfs_proto::DeleteRequest req;
    req.set_hash(argv[2]);
    dfs_proto::DeleteResponse rep;
    grpc::ClientContext ctx;
    grpc::Status st = stub->Delete(&ctx, req, &rep);
    if (st.ok()) std::cout << "delete ok\n";
    else std::cerr << "delete fail: " << st.error_message() << "\n";
  }
  return 0;
}
