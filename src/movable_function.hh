#pragma once

#include <functional>
#include <memory>
#include <utility>

namespace waxwing::internal {
namespace function {
template <typename R, typename... Args>
struct FunctorContainerBase {
    virtual ~FunctorContainerBase() = default;
    virtual R operator()(Args&&...) = 0;
};

template <typename F, typename R, typename... Args>
class FunctorContainer final : public FunctorContainerBase<R, Args...> {
    F f;

public:
    FunctorContainer(const F& f) : f{f} {}
    FunctorContainer(F&& f) : f{std::move(f)} {}

    R operator()(Args&&... args) override {
        return std::invoke(f, std::forward<Args>(args)...);
    }
};
}  // namespace function

template <typename>
class MovableFunction;

template <typename R, typename... Args>
class MovableFunction<R(Args...)> final {
    std::unique_ptr<function::FunctorContainerBase<R, Args...>> f_;

public:
    MovableFunction() : f_{nullptr} {}
    ~MovableFunction() = default;

    template <typename F>
        requires(!std::is_same_v<F, MovableFunction<R(Args...)>>)
    MovableFunction(F&& f)
        : f_{std::make_unique<function::FunctorContainer<F, R, Args...>>(
              std::forward<F>(f))} {}

    MovableFunction(const MovableFunction&) = delete;
    MovableFunction& operator=(const MovableFunction& other) = delete;

    MovableFunction(MovableFunction&& other) noexcept
        : f_{std::exchange(other.f_, nullptr)} {}

    MovableFunction& operator=(MovableFunction&& other) noexcept {
        std::swap(f_, other.f_);
        return *this;
    }

    R operator()(Args&&... args) const {
        return std::invoke(*f_, std::forward<Args>(args)...);
    }

    operator bool() const {
        return f_ != nullptr;
    }
};

}  // namespace waxwing::internal
