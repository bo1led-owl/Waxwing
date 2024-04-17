#pragma once

#include <functional>
#include <optional>

namespace http {
namespace iter {
template <typename Item>
class Iterator {
protected:
    using F = std::function<void(Item)>;
    bool finished_ = false;

public:
    virtual ~Iterator() = default;

    void finish() {
        finished_ = true;
    }

    virtual std::optional<Item> next() = 0;

    virtual void for_each(const F f) {
        while (!finished_) {
            std::optional<Item> capture = this->next();
            if (!capture) {
                break;
            }
            f(capture.value());
        }
    }
};

template <typename Item>
class CountIterator final : public Iterator<Item> {
    size_t count_;
    Iterator<Item> iter_;

public:
    CountIterator(Iterator<Item> iter, const size_t count)
        : count_{count}, iter_{std::move(iter)} {}

    std::optional<Item> next() override {
        if (this->count_ > static_cast<size_t>(0)) {
            this->count_ -= 1;
            return iter_.next();
        }
        return std::nullopt;
    }
};

template <typename Item>
class Enumerate final : public Iterator<std::pair<size_t, Item>> {
    using F = std::function<void(std::pair<size_t, Item>)>;

    size_t i_ = 0;
    Iterator<Item> iter_;

public:
    Enumerate(Iterator<Item>& iter) : iter_{std::move(iter)} {}

    std::optional<std::pair<size_t, Item>> next() override {
        std::optional<Item> value = iter_.next();
        if (!value.has_value()) {
            return std::nullopt;
        }
        return std::pair<size_t, Item>{this->i_++, value.value()};
    }

    void for_each(const F f) override {
        while (!this->finished_) {
            std::optional<std::pair<size_t, Item>> capture = this->next();
            if (!capture) {
                break;
            }
            f(capture.value());
        }
    }
};
}  // namespace iter
}  // namespace http
