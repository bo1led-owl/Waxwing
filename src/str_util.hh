#pragma once

#include <optional>
#include <string_view>

#include "iter.hh"

namespace http {
namespace str_util {
size_t length(const std::string_view s);
size_t length(const char);

template <typename Sep>
class SplitIterator final : public iter::Iterator<std::string_view> {
    std::string_view source_;
    const Sep sep_;

public:
    SplitIterator(const std::string_view source, const Sep sep)
        : source_(source), sep_(sep) {}

    std::string_view remaining() const {
        return source_;
    }

    std::optional<std::string_view> next() override {
        if (source_.empty()) {
            return std::nullopt;
        }

        size_t pos = source_.find(sep_);
        size_t len = pos;
        if (pos != std::string_view::npos) {
            pos += length(sep_);
        }

        const std::string_view result = source_.substr(0, len);

        source_ = source_.substr(std::min(source_.size(), pos));
        return result;
    }
};

template <typename Sep>
SplitIterator<Sep> split(const std::string_view str, const Sep sep) {
    return SplitIterator(str, sep);
}

std::string_view ltrim(const std::string_view s);
std::string_view rtrim(const std::string_view& s);
std::string_view trim(const std::string_view s);

std::string to_lower(const std::string_view s);
}  // namespace str_util
}  // namespace http
