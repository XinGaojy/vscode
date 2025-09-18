#ifndef DFS_CRYPTO_HPP
#define DFS_CRYPTO_HPP
#include <string>

namespace dfs { namespace crypto {

const std::string& default_key(); // 32-byte
std::string encrypt(const std::string& plain, const std::string& key);
std::string decrypt(const std::string& cipher, const std::string& key);

} } // namespace dfs::crypto
#endif
