#include "forgekv/status.h"

#include <iostream>
#include <string_view>

namespace {

bool Expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
  }
  return condition;
}

}  // namespace

int main() {
  bool passed = true;

  const forgekv::Status ok = forgekv::Status::OK();
  passed &= Expect(ok.ok(), "OK status should succeed");
  passed &= Expect(ok.code() == forgekv::StatusCode::kOk,
                   "OK status should have kOk code");
  passed &= Expect(ok.message().empty(), "OK status should have no message");
  passed &= Expect(ok.ToString() == "OK", "OK status should format as OK");

  const forgekv::Status missing = forgekv::Status::NotFound("key: user:42");
  passed &= Expect(!missing.ok(), "NotFound status should fail");
  passed &= Expect(missing.code() == forgekv::StatusCode::kNotFound,
                   "NotFound status should have kNotFound code");
  passed &= Expect(missing.message() == "key: user:42",
                   "NotFound status should preserve its message");
  passed &= Expect(missing.ToString() == "NotFound: key: user:42",
                   "NotFound status should include code and message");

  const forgekv::Status io_error = forgekv::Status::IOError("disk full");
  const forgekv::Status corruption = forgekv::Status::Corruption("bad checksum");
  passed &= Expect(io_error.code() != corruption.code(),
                   "IOError and Corruption must have distinct codes");

  return passed ? 0 : 1;
}
