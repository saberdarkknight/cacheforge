#include "forgekv/db.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace {

bool Expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
  }
  return condition;
}

std::filesystem::path TestDirectory() {
  const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("forgekv-db-test-" + std::to_string(timestamp));
}

}  // namespace

int main() {
  bool passed = true;
  const std::filesystem::path directory = TestDirectory();
  std::error_code error;
  std::filesystem::remove_all(directory, error);

  std::unique_ptr<forgekv::DB> db;
  forgekv::Status status = forgekv::DB::Open(directory, &db);
  passed &= Expect(status.ok(), "DB::Open should succeed");
  passed &= Expect(db != nullptr, "DB::Open should return an owned DB");

  status = db->Put("user:42", "Alice");
  passed &= Expect(status.ok(), "Put should succeed");
  status = db->Put("user:42", "Alicia");
  passed &= Expect(status.ok(), "overwriting a key should succeed");

  std::string value;
  status = db->Get("user:42", &value);
  passed &= Expect(status.ok() && value == "Alicia",
                   "Get should return the newest value");

  status = db->Delete("user:42");
  passed &= Expect(status.ok(), "Delete should succeed");
  status = db->Get("user:42", &value);
  passed &= Expect(status.code() == forgekv::StatusCode::kNotFound,
                   "deleted key should not be found");

  status = db->Put("persistent", "survives restart");
  passed &= Expect(status.ok(), "persistent write should succeed");
  passed &= Expect(db->Close().ok(), "Close should succeed");
  db.reset();

  // Simulate a crash partway through the next WAL record. Recovery must retain
  // complete records before it and ignore this incomplete tail.
  {
    std::ofstream wal(directory / "wal.log", std::ios::binary | std::ios::app);
    const char incomplete_record[] = {1, 2, 3};
    wal.write(incomplete_record, sizeof(incomplete_record));
  }

  status = forgekv::DB::Open(directory, &db);
  passed &= Expect(status.ok(), "reopening after a truncated WAL tail should work");
  status = db->Get("persistent", &value);
  passed &= Expect(status.ok() && value == "survives restart",
                   "WAL replay should recover complete records");
  passed &= Expect(db->Close().ok(), "final Close should succeed");

  std::filesystem::remove_all(directory, error);
  passed &= Expect(!error, "temporary database directory should be removed");
  return passed ? 0 : 1;
}
