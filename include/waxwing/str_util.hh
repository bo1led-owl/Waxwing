#pragma once

#include <algorithm>
#include <cctype>
#include <string_view>

namespace waxwing::str_util {
constexpr std::string_view ltrim(std::string_view s) {
    const std::string_view::iterator first_non_whitespace = std::find_if(
        s.begin(), s.end(), [](const char c) { return !std::isspace(c); });

    const auto whitespace_len = first_non_whitespace - s.cbegin();
    s.remove_prefix(whitespace_len);
    return s;
}

constexpr std::string_view rtrim(std::string_view s) {
    const auto first_non_whitespace = std::find_if(
        s.rbegin(), s.rend(), [](const char c) { return !std::isspace(c); });

    const auto whitespace_len = first_non_whitespace - s.rbegin();
    s.remove_suffix(whitespace_len);
    return s;
}

constexpr std::string_view trim(std::string_view s) { return rtrim(ltrim(s)); }

constexpr bool case_insensitive_eq(std::string_view lhs, std::string_view rhs) {
    return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(),
                      [](const char a, const char b) {
                          return std::tolower(a) == std::tolower(b);
                      });
}
}  // namespace waxwing::str_util
