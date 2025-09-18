#include "fs/filestore.hpp"
#include <rocksdb/db.h>

dfs::FileStore::FileStore(const std::string& dir) {
  rocksdb::Options opt;
  opt.create_if_missing = true;
  rocksdb::DB* raw = nullptr;
  rocksdb::DB::Open(opt, dir + "/chunks", &raw);
  db_.reset(raw);
}

bool dfs::FileStore::put(const std::string& hash, const std::string& blob) {
  return db_->Put(rocksdb::WriteOptions(), hash, blob).ok();
}

bool dfs::FileStore::get(const std::string& hash, std::string* blob) {
  return db_->Get(rocksdb::ReadOptions(), hash, blob).ok();
}

bool dfs::FileStore::del(const std::string& hash) {
  return db_->Delete(rocksdb::WriteOptions(), hash).ok();
}
