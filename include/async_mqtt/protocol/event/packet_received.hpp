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

/**
 * @brief Packet received event.
 *
 * This corresponds to @ref basic_connection::on_receive().
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * @tparam PacketIdBytes The number of bytes used for the packet identifier.
 *                       According to the MQTT specification, this is 2.
 *                       You can use @ref event::packet_received for this.
 */
template <std::size_t PacketIdBytes>
class basic_packet_received {
public:
    /**
     * @brief Constructor.
     *
     * @param packet The received packet.
     */
    explicit basic_packet_received(basic_packet_variant<PacketIdBytes> packet)
        : packet_{force_move(packet)} {}

    /**
     * @brief Get the received packet.
     *
     * @return A const reference to the received packet.
     */
    const basic_packet_variant<PacketIdBytes>& get() const {
        return packet_;
    }

    /**
     * @brief Get the received packet.
     *
     * @return A reference to the received packet.
     */
    basic_packet_variant<PacketIdBytes>& get() {
        return packet_;
    }

private:
    basic_packet_variant<PacketIdBytes> packet_;
};

/**
 * @brief Type alias of @ref basic_packet_received (PacketIdBytes=2).
 *        This is for typical usecase (e.g. MQTT client).
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 */
using packet_received = basic_packet_received<2>;

} // namespace async_mqtt::event

#endif // ASYNC_MQTT_PROTOCOL_EVENT_SEND_HPP
