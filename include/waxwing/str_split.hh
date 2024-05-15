#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

namespace waxwing::str_util {
template <typename Sep>
    requires(std::same_as<Sep, std::string_view> || std::same_as<Sep, char>)
class Split final {
    static constexpr size_t length(char) { return 1; }
    static constexpr size_t length(std::string_view s) { return s.size(); }

    std::string_view src_;
    size_t cur_index_ = 0;
    Sep sep_;
    bool found_on_prev_step_ = true;

public:
    constexpr Split(const std::string_view source, const Sep sep)
        : src_(source), sep_(sep) {}

    constexpr std::optional<std::string_view> next() {
        const size_t index = src_.find(sep_, cur_index_);
        const bool found = index != std::string_view::npos;

        if (found) {
            std::string_view result =
                src_.substr(cur_index_, index - cur_index_);
            cur_index_ = index + length(sep_);
            return result;
        } else if (cur_index_ < src_.length()) {
            std::string_view result = src_.substr(cur_index_);
            cur_index_ = src_.length();
            found_on_prev_step_ = false;
            return result;
        } else if (found_on_prev_step_ && cur_index_ == src_.length()) {
            found_on_prev_step_ = false;
            return src_.substr(src_.length());
        } else {
            return std::nullopt;
        }
    }

    constexpr std::string_view remaining() const noexcept {
        return src_.substr(cur_index_);
    }
};

constexpr Split<char> split(const std::string_view source,
                            const char separator) {
    return {source, separator};
}

constexpr Split<std::string_view> split(const std::string_view source,
                                        const std::string_view separator) {
    return {source, separator};
}
}  // namespace waxwing::str_util
