#include "forgekv/memtable.h"

namespace forgekv {

void MemTable::Put(std::string_view key, std::string_view value) {
  entries_[std::string(key)] = Entry{.deleted = false, .value = std::string(value)};
}

void MemTable::Delete(std::string_view key) {
  entries_[std::string(key)] = Entry{.deleted = true, .value = ""};
}

Status MemTable::Get(std::string_view key, std::string* value) const {
  const auto found = entries_.find(key);
  if (found == entries_.end() || found->second.deleted) {
    return Status::NotFound("key not found: " + std::string(key));
  }

  *value = found->second.value;
  return Status::OK();
}

}  // namespace forgekv
