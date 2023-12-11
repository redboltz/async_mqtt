// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_MAKE_SHARED_HELPER_HPP)
#define ASYNC_MQTT_UTIL_MAKE_SHARED_HELPER_HPP

#include <memory>

namespace async_mqtt {

template <typename T>
class make_shared_helper {
    friend T;
    struct target : public T {
        template<typename... Args>
        target(Args&&... args)
            :T{std::forward<Args>(args)...}
        {}
    };

    template <typename... Args>
    static std::shared_ptr<T> make_shared(Args&&... args) {
        return std::make_shared<target>(std::forward<Args>(args)...);
    }

    template<typename Alloc, typename... Args>
    static std::shared_ptr<T> allocate_shared(Alloc const& alloc, Args&&... args) {
        return std::allocate_shared<target>(alloc, std::forward<Args>(args)...);
    }
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_MAKE_SHARED_HELPER_HPP
