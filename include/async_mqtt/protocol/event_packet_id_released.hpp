// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_EVENT_PACKET_ID_RELEASED_HPP)
#define ASYNC_MQTT_PROTOCOL_EVENT_PACKET_ID_RELEASED_HPP

#include <cstddef>

#include <async_mqtt/packet/packet_variant.hpp>

namespace async_mqtt {

template <std::size_t PacketIdBytes>
class basic_event_packet_id_released {
public:
    basic_event_packet_id_released(basic_packet_id_type<PacketIdBytes> packet_id)
        :packet_id_{packet_id}
    {
    }

    basic_packet_id_type<PacketIdBytes> get() const {
        return packet_id_;
    }

private:
    basic_packet_id_type<PacketIdBytes> packet_id_;
};

using event_packet_id_released = basic_event_packet_id_released<2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_EVENT_PACKET_ID_RELEASED_HPP
