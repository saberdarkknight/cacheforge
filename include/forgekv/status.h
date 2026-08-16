#pragma once

#include <string>

namespace forgekv {

// A Status represents the result of an operation without using exceptions.
// An OK status has no error message; all other statuses describe a failure.
enum class StatusCode {
  kOk,
  kInvalidArgument,
  kNotFound,
  kIOError,
  kCorruption,
};

class Status {
 public:
  static Status OK();
  static Status InvalidArgument(std::string message);
  static Status NotFound(std::string message);
  static Status IOError(std::string message);
  static Status Corruption(std::string message);

  [[nodiscard]] bool ok() const noexcept;
  [[nodiscard]] StatusCode code() const noexcept;
  [[nodiscard]] const std::string& message() const noexcept;
  [[nodiscard]] std::string ToString() const;

 private:
  Status(StatusCode code, std::string message);

  StatusCode code_;
  std::string message_;
};

}  // namespace forgekv
