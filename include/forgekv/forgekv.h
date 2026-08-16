#pragma once

#include <string_view>

namespace forgekv {

// Temporary smoke-test API. Replace this with the DB interface in the next step.
std::string_view Name() noexcept;

}  // namespace forgekv
