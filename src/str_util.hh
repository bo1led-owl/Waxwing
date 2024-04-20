#pragma once

#include <string_view>

namespace http {
namespace str_util {
std::string_view ltrim(const std::string_view s);
std::string_view rtrim(const std::string_view s);
std::string_view trim(const std::string_view s);

std::string to_lower(const std::string_view s);
}  // namespace str_util
}  // namespace http
