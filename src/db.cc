#include "forgekv/db.h"

#include "forgekv/memtable.h"
#include "forgekv/wal.h"

#include <system_error>
#include <utility>

namespace forgekv {
DB::DB(std::filesystem::path directory, std::unique_ptr<WAL> wal,
       std::unique_ptr<MemTable> memtable)
    : directory_(std::move(directory)),
      wal_(std::move(wal)),
      memtable_(std::move(memtable)) {}

DB::~DB() = default;

Status DB::Open(const std::filesystem::path& directory,
                std::unique_ptr<DB>* result) {
  if (result == nullptr) {
    return Status::InvalidArgument("DB::Open result must not be null");
  }
  result->reset();

  std::error_code error;
  std::filesystem::create_directories(directory, error);
  if (error) {
    return Status::IOError("failed to create database directory: " +
                           error.message());
  }

  std::unique_ptr<WAL> wal;
  Status status = WAL::Open(directory / "wal.log", &wal);
  if (!status.ok()) {
    return status;
  }

  auto memtable = std::make_unique<MemTable>();
  auto db = std::unique_ptr<DB>(
      new DB(directory, std::move(wal), std::move(memtable)));

  status = db->wal_->Replay([&db](const WALRecord& record) {
    if (record.type == WALRecordType::kPut) {
      db->memtable_->Put(record.key, record.value);
    } else {
      db->memtable_->Delete(record.key);
    }
    return Status::OK();
  });
  if (!status.ok()) {
    return status;
  }

  *result = std::move(db);
  return Status::OK();
}

Status DB::Put(std::string_view key, std::string_view value) {
  Status status = EnsureOpen();
  if (!status.ok()) {
    return status;
  }
  status = wal_->AppendPut(key, value);
  if (!status.ok()) {
    return status;
  }
  memtable_->Put(key, value);
  return Status::OK();
}

Status DB::Get(std::string_view key, std::string* value) const {
  if (value == nullptr) {
    return Status::InvalidArgument("DB::Get value must not be null");
  }
  Status status = EnsureOpen();
  if (!status.ok()) {
    return status;
  }
  return memtable_->Get(key, value);
}

Status DB::Delete(std::string_view key) {
  Status status = EnsureOpen();
  if (!status.ok()) {
    return status;
  }
  status = wal_->AppendDelete(key);
  if (!status.ok()) {
    return status;
  }
  memtable_->Delete(key);
  return Status::OK();
}

Status DB::Close() {
  if (closed_) {
    return Status::OK();
  }
  wal_.reset();
  closed_ = true;
  return Status::OK();
}

Status DB::EnsureOpen() const {
  if (closed_) {
    return Status::IOError("database is closed");
  }
  return Status::OK();
}

}  // namespace forgekv
