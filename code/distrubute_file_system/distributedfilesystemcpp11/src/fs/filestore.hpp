#ifndef DFS_FILESTORE_HPP
#define DFS_FILESTORE_HPP
#include <string>
#include <memory>
namespace rocksdb { class DB; }

namespace dfs {

class FileStore {
public:
  explicit FileStore(const std::string& dir);
  bool put(const std::string& hash, const std::string& blob);
  bool get(const std::string& hash, std::string* blob);
  bool del(const std::string& hash);
private:
  std::unique_ptr<rocksdb::DB> db_;
};

} // namespace dfs
#endif
