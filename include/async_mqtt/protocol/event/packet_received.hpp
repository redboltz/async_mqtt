// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_EVENT_PACKET_RECEIVED_HPP)
#define ASYNC_MQTT_PROTOCOL_EVENT_PACKET_RECEIVED_HPP

#include <cstddef>

#include <async_mqtt/protocol/packet/packet_variant.hpp>

namespace async_mqtt::event {

template <std::size_t PacketIdBytes>
class basic_packet_received {
public:
    basic_packet_received(
        basic_packet_variant<PacketIdBytes> packet
    )
        :packet_{force_move(packet)}
    {
    }

    basic_packet_variant<PacketIdBytes> const& get() const {
        return packet_;
    }

    basic_packet_variant<PacketIdBytes>& get() {
        return packet_;
    }

private:
    basic_packet_variant<PacketIdBytes> packet_;
};

using packet_received = basic_packet_received<2>;

} // namespace async_mqtt::event

#endif // ASYNC_MQTT_PROTOCOL_EVENT_SEND_HPP
