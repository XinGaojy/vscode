#include "security/crypto.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
namespace dfs { namespace crypto {

static const std::string kKey = "0123456789abcdef0123456789abcdef"; // 示例

const std::string& default_key() { return kKey; }

std::string encrypt(const std::string& plain, const std::string& key) {
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  std::string iv(12, '\0');
  RAND_bytes(reinterpret_cast<unsigned char*>(&iv[0]), 12);
  std::string cipher;
  cipher.resize(plain.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()) + 12 + 16);
  int len, total = 0;
  EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr,
                     reinterpret_cast<const unsigned char*>(key.data()),
                     reinterpret_cast<const unsigned char*>(iv.data()));
  EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(&cipher[0]), &len,
                    reinterpret_cast<const unsigned char*>(plain.data()), plain.size());
  total += len;
  EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(&cipher[0]) + total, &len);
  total += len;
  std::string tag(16, '\0');
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, &tag[0]);
  EVP_CIPHER_CTX_free(ctx);
  cipher.resize(total);
  return iv + tag + cipher;
}

std::string decrypt(const std::string& pack, const std::string& key) {
  if (pack.size() < 28) throw std::runtime_error("bad cipher");
  std::string iv  = pack.substr(0, 12);
  std::string tag = pack.substr(12, 16);
  std::string cipher = pack.substr(28);
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  std::string plain;
  plain.resize(cipher.size());
  int len, total = 0;
  EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr,
                     reinterpret_cast<const unsigned char*>(key.data()),
                     reinterpret_cast<const unsigned char*>(iv.data()));
  EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(&plain[0]), &len,
                    reinterpret_cast<const unsigned char*>(cipher.data()), cipher.size());
  total += len;
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, const_cast<char*>(tag.data()));
  int rc = EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(&plain[0]) + total, &len);
  EVP_CIPHER_CTX_free(ctx);
  if (rc <= 0) throw std::runtime_error("auth fail");
  total += len;
  plain.resize(total);
  return plain;
}

} } // namespace dfs::crypto
