#pragma once

#include <algorithm>
#include <ranges>
#include <string_view>

namespace waxwing::str_util {
constexpr std::string_view ltrim(const std::string_view s) {
    const std::string_view::iterator first_non_whitespace =
        std::ranges::find_if(s, [](const char c) { return !std::isspace(c); });
    return s.substr(first_non_whitespace - s.cbegin());
}

constexpr std::string_view rtrim(const std::string_view s) {
    const std::string_view::reverse_iterator first_non_whitespace =
        std::ranges::find_if(s | std::views::reverse,
                             [](const char c) { return !std::isspace(c); });

    const auto whitespace_len = first_non_whitespace - s.crbegin();
    return s.substr(0, s.size() - whitespace_len);
}

constexpr std::string_view trim(const std::string_view s) {
    return rtrim(ltrim(s));
}

constexpr char ascii_to_lower(const char c) {
    if ('A' <= c && c <= 'Z') {
        return c + ('a' - 'A');
    } else {
        return c;
    }
}

// constexpr std::string to_lower(const std::string_view s) {
//     std::string res{s};
//     std::for_each(res.begin(), res.end(),
//                   [](char& c) { c = ascii_to_lower(c); });
//     return res;
// }

// constexpr std::string to_lower(std::string&& s) {
//     std::for_each(s.begin(), s.end(), [](char& c) { c = ascii_to_lower(c);
//     }); return std::move(s);
// }

constexpr bool case_insensitive_eq(std::string_view lhs, std::string_view rhs) {
    return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(),
                      [](const char a, const char b) {
                          return ascii_to_lower(a) == ascii_to_lower(b);
                      });
}
}  // namespace waxwing::str_util
