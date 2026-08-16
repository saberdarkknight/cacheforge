#pragma once

#include "forgekv/status.h"

#include <map>
#include <string>
#include <string_view>

namespace forgekv {

// MemTable holds the newest value (or deletion marker) for each key in memory.
// A future phase will replace or augment this with a more scalable structure.
class MemTable {
 public:
  void Put(std::string_view key, std::string_view value);
  void Delete(std::string_view key);
  [[nodiscard]] Status Get(std::string_view key, std::string* value) const;

 private:
  struct Entry {
    bool deleted = false;
    std::string value;
  };

  std::map<std::string, Entry, std::less<>> entries_;
};

}  // namespace forgekv
