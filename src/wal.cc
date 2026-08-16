#include "forgekv/wal.h"

#include <array>
#include <cstdint>
#include <limits>
#include <utility>

namespace forgekv {
namespace {

constexpr std::uint32_t kMaximumRecordPartSize = 16U * 1024U * 1024U;

void WriteUint32(std::ofstream* output, std::uint32_t value) {
  std::array<char, 4> bytes{};
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    bytes[index] = static_cast<char>((value >> (index * 8U)) & 0xFFU);
  }
  output->write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

bool ReadUint32(std::ifstream* input, std::uint32_t* value) {
  std::array<unsigned char, 4> bytes{};
  input->read(reinterpret_cast<char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
  if (!*input) {
    return false;
  }

  *value = static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[3]) << 24U);
  return true;
}

bool ReadString(std::ifstream* input, std::uint32_t size, std::string* value) {
  value->resize(size);
  input->read(value->data(), static_cast<std::streamsize>(size));
  return static_cast<bool>(*input);
}

}  // namespace

WAL::WAL(std::filesystem::path path, std::ofstream output)
    : path_(std::move(path)), output_(std::move(output)) {}

Status WAL::Open(const std::filesystem::path& path, std::unique_ptr<WAL>* result) {
  if (result == nullptr) {
    return Status::InvalidArgument("WAL::Open result must not be null");
  }
  result->reset();

  std::ofstream output(path, std::ios::binary | std::ios::app);
  if (!output.is_open()) {
    return Status::IOError("failed to open WAL: " + path.string());
  }

  *result = std::unique_ptr<WAL>(new WAL(path, std::move(output)));
  return Status::OK();
}

Status WAL::AppendPut(std::string_view key, std::string_view value) {
  return Append(WALRecordType::kPut, key, value);
}

Status WAL::AppendDelete(std::string_view key) {
  return Append(WALRecordType::kDelete, key, "");
}

Status WAL::Replay(const std::function<Status(const WALRecord&)>& apply) const {
  std::ifstream input(path_, std::ios::binary);
  if (!input.is_open()) {
    return Status::IOError("failed to read WAL: " + path_.string());
  }

  while (true) {
    char type_byte = 0;
    input.get(type_byte);
    if (input.eof()) {
      return Status::OK();
    }
    if (!input) {
      return Status::IOError("failed while reading WAL: " + path_.string());
    }

    std::uint32_t key_size = 0;
    std::uint32_t value_size = 0;
    if (!ReadUint32(&input, &key_size) || !ReadUint32(&input, &value_size)) {
      return Status::OK();  // Ignore an incomplete final record after a crash.
    }
    if (key_size > kMaximumRecordPartSize || value_size > kMaximumRecordPartSize) {
      return Status::Corruption("WAL record is larger than the configured limit");
    }

    WALRecord record{.type = static_cast<WALRecordType>(type_byte)};
    if (record.type != WALRecordType::kPut &&
        record.type != WALRecordType::kDelete) {
      return Status::Corruption("WAL record has an unknown type");
    }
    if (!ReadString(&input, key_size, &record.key) ||
        !ReadString(&input, value_size, &record.value)) {
      return Status::OK();  // Ignore an incomplete final record after a crash.
    }

    Status status = apply(record);
    if (!status.ok()) {
      return status;
    }
  }
}

Status WAL::Append(WALRecordType type, std::string_view key,
                   std::string_view value) {
  if (key.size() > std::numeric_limits<std::uint32_t>::max() ||
      value.size() > std::numeric_limits<std::uint32_t>::max()) {
    return Status::InvalidArgument("WAL record is too large");
  }

  output_.put(static_cast<char>(type));
  WriteUint32(&output_, static_cast<std::uint32_t>(key.size()));
  WriteUint32(&output_, static_cast<std::uint32_t>(value.size()));
  output_.write(key.data(), static_cast<std::streamsize>(key.size()));
  output_.write(value.data(), static_cast<std::streamsize>(value.size()));
  output_.flush();

  if (!output_) {
    return Status::IOError("failed to append to WAL: " + path_.string());
  }
  return Status::OK();
}

}  // namespace forgekv
