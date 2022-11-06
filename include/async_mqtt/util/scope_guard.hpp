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

template <typename Proc>
inline auto unique_scope_guard(Proc&& proc) {
    auto deleter = [proc = std::forward<Proc>(proc)](void*) mutable { std::forward<Proc>(proc)(); };
    return std::unique_ptr<void, decltype(deleter)>(&deleter, force_move(deleter));
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_UNIQUE_SCOPE_GUARD_HPP
