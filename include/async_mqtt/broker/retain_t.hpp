// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_RETAIN_T_HPP)
#define ASYNC_MQTT_BROKER_RETAIN_T_HPP

#include <boost/asio/steady_timer.hpp>

#include <async_mqtt/buffer.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/packet/subopts.hpp>

namespace async_mqtt {

// A collection of messages that have been retained in
// case clients add a new subscription to the associated topics.
struct retain_t {
    retain_t(
        buffer topic,
        buffer contents,
        properties props,
        qos qos_value,
        std::shared_ptr<as::steady_timer> tim_message_expiry = std::shared_ptr<as::steady_timer>())
        :topic(force_move(topic)),
         contents(force_move(contents)),
         props(force_move(props)),
         qos_value(qos_value),
         tim_message_expiry(force_move(tim_message_expiry))
    { }

    buffer topic;
    buffer contents;
    properties props;
    qos qos_value;
    std::shared_ptr<as::steady_timer> tim_message_expiry;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_RETAIN_T_HPP
