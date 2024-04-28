#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace http {
namespace internal {
struct string_hash {
    using hash_type = std::hash<std::string_view>;
    using is_transparent = void;

    size_t operator()(const char* str) const {
        return hash_type{}(str);
    }
    size_t operator()(std::string_view str) const {
        return hash_type{}(str);
    }
    size_t operator()(std::string const& str) const {
        return hash_type{}(str);
    }
};

using Headers =
    std::unordered_map<std::string, std::string, string_hash, std::equal_to<>>;
using Params =
    std::unordered_map<std::string, std::string, string_hash, std::equal_to<>>;
}  // namespace internal
}  // namespace http
