#pragma once

#include "forgekv/status.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace forgekv {

class MemTable;
class WAL;

// DB is the public interface for a single local ForgeKV database.
//
// The implementation coordinates a WAL and MemTable now. SSTables will be
// introduced later, when the MemTable needs to flush to immutable disk files.
class DB {
 public:
  // Opens (or creates) a database under directory.
  [[nodiscard]] static Status Open(const std::filesystem::path& directory,
                                   std::unique_ptr<DB>* result);
  ~DB();

  [[nodiscard]] Status Put(std::string_view key, std::string_view value);

  // Searches the MemTable now. It will search SSTables in a later phase.
  [[nodiscard]] Status Get(std::string_view key, std::string* value) const;

  [[nodiscard]] Status Delete(std::string_view key);

  [[nodiscard]] Status Close();

 private:
  DB(std::filesystem::path directory, std::unique_ptr<WAL> wal,
     std::unique_ptr<MemTable> memtable);

  [[nodiscard]] Status EnsureOpen() const;

  std::filesystem::path directory_;
  std::unique_ptr<WAL> wal_;
  std::unique_ptr<MemTable> memtable_;
  bool closed_ = false;
};

}  // namespace forgekv
