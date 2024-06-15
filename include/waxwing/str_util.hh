#pragma once

#include <string_view>

namespace waxwing::str_util {
std::string_view ltrim(std::string_view s);
std::string_view rtrim(std::string_view s);
std::string_view trim(std::string_view s);
bool case_insensitive_eq(std::string_view lhs, std::string_view rhs);
}  // namespace waxwing::str_util
