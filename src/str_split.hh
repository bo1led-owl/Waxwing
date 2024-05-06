#pragma once

#include <cstddef>
#include <iterator>
#include <optional>
#include <string_view>

namespace http {
namespace str_util {
template <typename Sep>
class Split final {
    constexpr static size_t length(const std::string_view s) {
        return s.size();
    }
    constexpr static size_t length(const char) {
        return 1;
    }

    std::string_view source_;
    Sep sep_;

public:
    struct IteratorTerminator final {};

    class Iterator final {
        friend class Split;

        std::optional<std::string_view> remaining_;
        std::optional<std::string_view> cur_;
        Sep sep_;

        constexpr Iterator(const std::string_view src, const Sep sep)
            : remaining_{src}, sep_{sep} {
            ++(*this);
        }

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = std::string_view;
        using pointer = const std::string_view*;
        using reference = const std::string_view&;
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

        constexpr value_type operator*() {
            return cur_.value();
        }

        constexpr pointer operator->() {
            return &cur_.value();
        }

        constexpr bool operator==(const Iterator& other) const noexcept {
            return remaining_ == other.remaining_ && cur_ == other.cur_ &&
                   sep_ == other.sep_;
        }

        constexpr bool operator==(const IteratorTerminator&) const noexcept {
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

    constexpr IteratorTerminator end() const {
        return IteratorTerminator{};
    }
};

template <typename Sep>
constexpr Split<Sep> split(const std::string_view str, const Sep sep) {
    return Split(str, sep);
}
}  // namespace str_util
}  // namespace http
