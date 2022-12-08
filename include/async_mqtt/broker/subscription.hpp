// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_SUBSCRIPTION_HPP)
#define ASYNC_MQTT_BROKER_SUBSCRIPTION_HPP

#include <async_mqtt/util/optional.hpp>
#include <async_mqtt/packet/subopts.hpp>
#include <async_mqtt/buffer.hpp>
#include <async_mqtt/broker/session_state_fwd.hpp>

namespace async_mqtt {

template <typename... NextLayer>
struct subscription {
    subscription(
        session_state_ref<NextLayer...> ss,
        buffer sharename,
        buffer topic,
        sub::opts opts,
        optional<std::size_t> sid)
        :ss{ss},
         sharename{force_move(sharename)},
         topic{force_move(topic)},
         opts{opts},
         sid{sid}
    {}

    session_state_ref<NextLayer...> ss;
    buffer sharename;
    buffer topic;
    sub::opts opts;
    optional<std::size_t> sid;
};

template <typename... NextLayer>
inline bool operator<(subscription<NextLayer...> const& lhs, subscription<NextLayer...> const& rhs) {
    return &lhs.ss.get() < &rhs.ss.get();
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_SUBSCRIPTION_HPP
