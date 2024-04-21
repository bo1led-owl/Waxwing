#include "str_util.hh"

#include <algorithm>
#include <cctype>
#include <ranges>
#include <string>
#include <string_view>

namespace http {
namespace str_util {
std::string_view ltrim(const std::string_view s) {
    const std::string_view::iterator first_non_whitespace =
        std::ranges::find_if(s, [](const char c) { return !std::isspace(c); });
    return s.substr(first_non_whitespace - s.cbegin());
}

std::string_view rtrim(const std::string_view s) {
    const std::string_view::reverse_iterator first_non_whitespace =
        std::ranges::find_if(s | std::views::reverse,
                             [](const char c) { return !std::isspace(c); });
    return s.substr(0, first_non_whitespace - s.crend() - 1);
}

std::string_view trim(const std::string_view s) {
    return rtrim(ltrim(s));
}

std::string to_lower(const std::string_view s) {
    std::string res{s};
    std::for_each(res.begin(), res.end(), [](char& c) { c = std::tolower(c); });
    return res;
}
}  // namespace str_util
}  // namespace http
