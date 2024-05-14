#pragma once

#include <cstddef>
#include <iterator>
#include <optional>
#include <string_view>

namespace waxwing::str_util {
template <typename Sep>
    requires(std::convertible_to<Sep, std::string_view> ||
             std::convertible_to<Sep, char>)
class Split final {
    static constexpr size_t length(char) { return 1; }

    static constexpr size_t length(std::string_view s) { return s.size(); }

    std::string_view source_;
    Sep sep_;

public:
    struct IteratorSentinel final {};

    class Iterator final {
        friend class Split;

        std::string_view src_;
        std::optional<std::string_view> cur_;
        size_t cur_index_ = 0;
        Sep sep_;

        constexpr Iterator(std::string_view src, Sep sep)
            : src_{src}, sep_{sep} {
            const size_t index = src_.find(sep_);
            const bool found = index != std::string_view::npos;
            if (!found && src_.empty()) {
                cur_ = "";
            } else {
                cur_ = src_.substr(0, index);
                cur_index_ = std::min(index, src_.length());
            }
        }

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = std::string_view;
        using pointer = value_type const*;
        using reference = value_type const&;
        using iterator_category = std::forward_iterator_tag;

        constexpr Iterator& operator++() {
            if (!cur_) {
                return *this;
            }

            const size_t index = src_.find(sep_, cur_index_ + length(sep_));
            const bool found = index != std::string_view::npos;

            if (found) {
                cur_ = src_.substr(cur_index_ + length(sep_),
                                   index - cur_index_ - length(sep_));
                cur_index_ = index;
            } else if (cur_index_ < src_.length()) {
                cur_ = src_.substr(cur_index_ + length(sep_));
                cur_index_ = src_.length();
            } else {
                cur_ = std::nullopt;
            }

            return *this;
        }

        constexpr Iterator operator++(int) {
            Iterator prev = *this;
            ++(*this);
            return prev;
        }

        constexpr value_type operator*() const { return *cur_; }

        constexpr pointer operator->() const { return &*cur_; }

        constexpr bool operator==(const Iterator& other) const noexcept {
            return src_ == src_ && cur_ == other.cur_ &&
                   cur_index_ == other.cur_index_ && sep_ == other.sep_;
        }

        constexpr bool operator==(const IteratorSentinel&) const noexcept {
            return cur_ == std::nullopt;
        }

        constexpr std::string_view remaining() const noexcept {
            return src_.substr(cur_index_ + length(sep_));
        }
    };

    constexpr Split(const std::string_view source, const Sep sep)
        : source_(source), sep_(sep) {}

    constexpr Iterator begin() const { return Iterator{source_, sep_}; }

    constexpr IteratorSentinel end() const { return IteratorSentinel{}; }
};

constexpr Split<std::string_view> split(std::string_view str,
                                        std::string_view sep) {
    return Split(str, sep);
}

constexpr Split<char> split(std::string_view str, char sep) {
    return Split(str, sep);
}
}  // namespace waxwing::str_util
