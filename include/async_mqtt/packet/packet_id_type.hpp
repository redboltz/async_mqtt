// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PACKET_ID_TYPE_HPP)
#define ASYNC_MQTT_PACKET_PACKET_ID_TYPE_HPP

#include <cstdint>
#include <cstddef>

namespace async_mqtt {

template <std::size_t Bytes>
struct packet_id_type;

template <>
struct packet_id_type<2> {
    using type = std::uint16_t;
};
template <>
struct packet_id_type<4> {
    using type = std::uint32_t;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PACKET_ID_TYPE_HPP
