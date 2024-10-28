// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_EVENT_PACKET_ID_RELEASED_HPP)
#define ASYNC_MQTT_PROTOCOL_EVENT_PACKET_ID_RELEASED_HPP

#include <cstddef>

#include <async_mqtt/protocol/packet/packet_id_type.hpp>

namespace async_mqtt::event {

template <std::size_t PacketIdBytes>
class basic_packet_id_released {
public:
    basic_packet_id_released(typename basic_packet_id_type<PacketIdBytes>::type packet_id)
        :packet_id_{packet_id}
    {
    }

    typename basic_packet_id_type<PacketIdBytes>::type get() const {
        return packet_id_;
    }

private:
    typename basic_packet_id_type<PacketIdBytes>::type packet_id_;
};

using packet_id_released = basic_packet_id_released<2>;

} // namespace async_mqtt::event

#endif // ASYNC_MQTT_PROTOCOL_EVENT_PACKET_ID_RELEASED_HPP
