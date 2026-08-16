#include "forgekv/status.h"

#include <utility>

namespace forgekv {

Status::Status(StatusCode code, std::string message)
    : code_(code), message_(std::move(message)) {}

Status Status::OK() {
  return Status(StatusCode::kOk, "");
}

Status Status::NotFound(std::string message) {
  return Status(StatusCode::kNotFound, std::move(message));
}

Status Status::IOError(std::string message) {
  return Status(StatusCode::kIOError, std::move(message));
}

Status Status::Corruption(std::string message) {
  return Status(StatusCode::kCorruption, std::move(message));
}

bool Status::ok() const noexcept {
  return code_ == StatusCode::kOk;
}

StatusCode Status::code() const noexcept {
  return code_;
}

const std::string& Status::message() const noexcept {
  return message_;
}

std::string Status::ToString() const {
  switch (code_) {
    case StatusCode::kOk:
      return "OK";
    case StatusCode::kNotFound:
      return "NotFound: " + message_;
    case StatusCode::kIOError:
      return "IOError: " + message_;
    case StatusCode::kCorruption:
      return "Corruption: " + message_;
  }

  return "Unknown status";
}

}  // namespace forgekv
