#pragma once

#include <string>
#include <string_view>

namespace http {
namespace str_util {
std::string_view ltrim(std::string_view s);
std::string_view rtrim(std::string_view s);
std::string_view trim(std::string_view s);

std::string to_lower(std::string_view s);
}  // namespace str_util
}  // namespace http
