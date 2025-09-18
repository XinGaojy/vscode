#include <grpcpp/grpcpp.h>
#include "dfs_proto/file.grpc.pb.h"
#include "fs/filestore.hpp"
#include "security/crypto.hpp"
#include "protocol/codec.hpp"
#include "common/logger.hpp"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
namespace dfs_proto = file;

class DFSServiceImpl final : public dfs_proto::FileService::Service {
  dfs::FileStore store_;
public:
  DFSServiceImpl() : store_("data") {}
  Status Upload(ServerContext* ctx, const dfs_proto::UploadRequest* req,
                dfs_proto::UploadResponse* rep) override {
    std::string blob = dfs::crypto::encrypt(req->data(), dfs::crypto::default_key());
    std::string hash = dfs_proto::sha256(req->name());
    store_.put(hash, blob);
    rep->set_hash(hash);
    return Status::OK;
  }
  Status Download(ServerContext* ctx, const dfs_proto::DownloadRequest* req,
                  dfs_proto::DownloadResponse* rep) override {
    std::string blob;
    if (!store_.get(req->hash(), &blob)) return Status(grpc::StatusCode::NOT_FOUND, "");
    rep->set_data(dfs::crypto::decrypt(blob, dfs::crypto::default_key()));
    return Status::OK;
  }
  Status Delete(ServerContext* ctx, const dfs_proto::DeleteRequest* req,
                dfs_proto::DeleteResponse* rep) override {
    store_.del(req->hash());
    return Status::OK;
  }
};

int main() {
  dfs::init_logger();
  std::string server_addr = "0.0.0.0:3000";
  DFSServiceImpl service;
  ServerBuilder builder;
  builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  LOG_INFO("C++11 server listening on %s", server_addr.c_str());
  server->Wait();
  return 0;
}
