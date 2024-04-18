#pragma once

#include <iterator>

namespace http {
namespace str_util {

template <typename Sep>
class Split final {
    static size_t length(const std::string_view s) {
        return s.size();
    }
    static size_t length(const char) {
        return 1;
    }

    std::string_view source_;
    const Sep sep_;

public:
    class Iterator final {
        friend class Split;

        std::string_view source_;
        Sep sep_;
        std::string_view cur_;

        Iterator(const std::string_view src, const Sep sep)
            : source_{src}, sep_{sep}, cur_{src.substr(0, src.find(sep))} {}

        static Iterator empty(const Sep sep) {
            return Iterator{std::string_view{}, sep};
        }

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = std::string_view;
        using pointer = const std::string_view*;
        using reference = const std::string_view&;
        using iterator_category = std::forward_iterator_tag;

        Iterator() {}

        Iterator& operator++() {
            if (source_.empty()) {
                return *this;
            }

            source_ = source_.substr(
                std::min(source_.size(), cur_.size() + length(sep_)));
            size_t pos = source_.find(sep_);
            size_t len = pos;
            if (pos != std::string_view::npos) {
                pos += length(sep_);
            }

            cur_ = source_.substr(0, len);
            return *this;
        }

        Iterator operator++(int) {
            Iterator prev = *this;
            ++(*this);
            return prev;
        }

        value_type operator*() {
            return cur_;
        }

        pointer operator->() {
            return &cur_;
        }

        bool operator==(const Iterator& other) const {
            return source_ == other.source_ && cur_ == other.cur_ &&
                   sep_ == other.sep_;
        }

        std::string_view remaining() const {
            return source_;
        }
    };

    Split(const std::string_view source, const Sep sep)
        : source_(source), sep_(sep) {}

    std::string_view remaining() const {
        return source_;
    }

    Iterator begin() const {
        return Iterator{source_, sep_};
    }

    Iterator end() const {
        return Iterator::empty(sep_);
    }
};

template <typename Sep>
Split<Sep> split(const std::string_view str, const Sep sep) {
    return Split(str, sep);
}

}  // namespace str_util
}  // namespace http
