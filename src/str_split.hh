#pragma once

#include <cstddef>
#include <iterator>
#include <optional>
#include <string_view>

namespace waxwing {
namespace str_util {
template <typename Sep>
    requires(std::convertible_to<Sep, std::string_view> ||
             std::convertible_to<Sep, char>)
class Split final {
    static constexpr size_t length(const char) {
        return 1;
    }

    static constexpr size_t length(const std::string_view& s) {
        return s.size();
    }

    std::string_view source_;
    Sep sep_;

public:
    struct IteratorSentinel final {};

    class Iterator final {
        friend class Split;

        std::optional<std::string_view> remaining_;
        std::optional<std::string_view> cur_;
        Sep sep_;

        constexpr Iterator(std::string_view src, Sep sep)
            : remaining_{src}, sep_{sep} {
            ++(*this);
        }

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = std::string_view;
        using pointer = value_type const*;
        using reference = value_type const&;
        using iterator_category = std::forward_iterator_tag;

        constexpr Iterator& operator++() {
            const size_t len = remaining_->find(sep_);
            const bool found = len != std::string_view::npos;

            if (found) {
                cur_ = remaining_->substr(0, len);
                remaining_ = remaining_->substr(len + length(sep_));
            } else if (remaining_) {
                cur_ = remaining_;
                remaining_ = std::nullopt;
            } else {
                cur_ = std::nullopt;
                remaining_ = std::nullopt;
            }

            return *this;
        }

        constexpr Iterator operator++(int) {
            Iterator prev = *this;
            ++(*this);
            return prev;
        }

        constexpr value_type operator*() const {
            return cur_.value();
        }

        constexpr pointer operator->() const {
            return &cur_.value();
        }

        constexpr bool operator==(const Iterator& other) const noexcept {
            return remaining_ == other.remaining_ && cur_ == other.cur_ &&
                   sep_ == other.sep_;
        }

        constexpr bool operator==(const IteratorSentinel&) const noexcept {
            return cur_ == std::nullopt;
        }

        constexpr std::string_view remaining() const noexcept {
            return remaining_.value_or("");
        }
    };

    constexpr Split(const std::string_view source, const Sep sep)
        : source_(source), sep_(sep) {}

    constexpr Iterator begin() const {
        return Iterator{source_, sep_};
    }

    constexpr IteratorSentinel end() const {
        return IteratorSentinel{};
    }
};

constexpr Split<std::string_view> split(const std::string_view str,
                                        const std::string_view sep) {
    return Split(str, sep);
}

constexpr Split<char> split(const std::string_view str, const char sep) {
    return Split(str, sep);
}
}  // namespace str_util
}  // namespace waxwing
