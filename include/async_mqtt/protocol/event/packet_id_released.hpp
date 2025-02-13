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

/**
 * @brief Packet identifier released event.
 *
 * This corresponds to @ref basic_connection::on_packet_id_released().
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * @tparam PacketIdBytes The number of bytes used for the packet identifier.
 *                       According to the MQTT specification, this is 2.
 *                       You can use @ref event::packet_id_released for this.
 */
template <std::size_t PacketIdBytes>
class basic_packet_id_released {
public:
    /**
     * @brief Constructor.
     *
     * @param packet_id The released packet_id.
     */
    explicit basic_packet_id_released(typename basic_packet_id_type<PacketIdBytes>::type packet_id)
        : packet_id_{packet_id} {}

    /**
     * @brief Get the released packet_id.
     *
     * @return The released packet_id.
     */
    typename basic_packet_id_type<PacketIdBytes>::type get() const {
        return packet_id_;
    }

private:
    typename basic_packet_id_type<PacketIdBytes>::type packet_id_;
};


/**
 * @brief Type alias of @ref basic_packet_id_released (PacketIdBytes=2).
 *        This is for typical usecase (e.g. MQTT client).
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 */
using packet_id_released = basic_packet_id_released<2>;

} // namespace async_mqtt::event

#endif // ASYNC_MQTT_PROTOCOL_EVENT_PACKET_ID_RELEASED_HPP
