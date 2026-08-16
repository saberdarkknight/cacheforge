#pragma once

#include "forgekv/status.h"

#include <filesystem>
#include <functional>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>

namespace forgekv {

enum class WALRecordType : unsigned char {
  kPut = 1,
  kDelete = 2,
};

struct WALRecord {
  WALRecordType type;
  std::string key;
  std::string value;
};

// WAL is an append-only local file. Each record is encoded as:
// [type: 1 byte][key length: 4 bytes][value length: 4 bytes][key][value]
// This initial local format has no checksum; a truncated final record is
// ignored during replay and will be upgraded with checksums in a later phase.
class WAL {
 public:
  [[nodiscard]] static Status Open(const std::filesystem::path& path,
                                   std::unique_ptr<WAL>* result);

  [[nodiscard]] Status AppendPut(std::string_view key, std::string_view value);
  [[nodiscard]] Status AppendDelete(std::string_view key);
  [[nodiscard]] Status Replay(
      const std::function<Status(const WALRecord&)>& apply) const;

 private:
  WAL(std::filesystem::path path, std::ofstream output);

  [[nodiscard]] Status Append(WALRecordType type, std::string_view key,
                              std::string_view value);

  std::filesystem::path path_;
  std::ofstream output_;
};

}  // namespace forgekv
