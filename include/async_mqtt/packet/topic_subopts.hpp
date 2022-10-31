// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP)
#define ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP

#include <async_mqtt/buffer.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/packet/subopts.hpp>

namespace async_mqtt {

struct topic_subopts {
    topic_subopts(
        buffer topic,
        sub::opts opts
    ): topic{force_move(topic)}, opts{opts}
    {}

    buffer topic;
    sub::opts opts;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP
