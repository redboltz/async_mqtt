// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_UNIQUE_SCOPE_GUARD_HPP)
#define ASYNC_MQTT_UTIL_UNIQUE_SCOPE_GUARD_HPP

#include <memory>
#include <utility>

namespace async_mqtt {

namespace detail {
template <typename Proc>
class unique_sg {
public:
    unique_sg(Proc proc) noexcept
        :proc_{std::move(proc)}
    {}
    unique_sg(unique_sg const&) = delete;
    unique_sg(unique_sg&& other) noexcept
        :proc_{other.proc_} {
        other.moved_ = true;
    }
    ~unique_sg() noexcept {
        if (!moved_) std::move(proc_)();
    }

private:
    Proc proc_;
    bool moved_ = false;
};

} // namespace detail

template <typename Proc>
inline detail::unique_sg<Proc> unique_scope_guard(Proc&& proc) noexcept {
    return detail::unique_sg<Proc>{std::forward<Proc>(proc)};
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_UNIQUE_SCOPE_GUARD_HPP
