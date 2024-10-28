// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_RETAIN_TYPE_HPP)
#define ASYNC_MQTT_BROKER_RETAIN_TYPE_HPP

#include <boost/asio/steady_timer.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/protocol/packet/property_variant.hpp>
#include <async_mqtt/protocol/packet/subopts.hpp>

namespace async_mqtt {

// A collection of messages that have been retained in
// case clients add a new subscription to the associated topics.
struct retain_type {
    retain_type(
        std::string topic,
        std::vector<buffer> payload,
        properties props,
        qos qos_value,
        std::shared_ptr<as::steady_timer> tim_message_expiry = std::shared_ptr<as::steady_timer>())
        :topic(force_move(topic)),
         props(force_move(props)),
         qos_value(qos_value),
         tim_message_expiry(force_move(tim_message_expiry))
    {
        auto it = std::cbegin(payload);
        auto end = std::cend(payload);
        for (; it != end; ++it) {
            this->payload.emplace_back(*it);
        }
    }

    std::string topic;
    std::vector<buffer> payload;
    properties props;
    qos qos_value;
    std::shared_ptr<as::steady_timer> tim_message_expiry;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_RETAIN_TYPE_HPP
