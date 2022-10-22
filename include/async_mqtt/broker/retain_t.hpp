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
    template <
        typename BufferSequence,
        typename std::enable_if_t<
            is_buffer_sequence<std::decay_t<BufferSequence>>::value,
            std::nullptr_t
        > = nullptr
    >
    retain_t(
        buffer topic,
        BufferSequence&& payload,
        properties props,
        qos qos_value,
        std::shared_ptr<as::steady_timer> tim_message_expiry = std::shared_ptr<as::steady_timer>())
        :topic(force_move(topic)),
         props(force_move(props)),
         qos_value(qos_value),
         tim_message_expiry(force_move(tim_message_expiry))
    {
        auto it = buffer_sequence_begin(payload);
        auto end = buffer_sequence_end(payload);
        for (; it != end; ++it) {
            this->payload.emplace_back(*it);
        }
    }

    buffer topic;
    std::vector<buffer> payload;
    properties props;
    qos qos_value;
    std::shared_ptr<as::steady_timer> tim_message_expiry;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_RETAIN_T_HPP
