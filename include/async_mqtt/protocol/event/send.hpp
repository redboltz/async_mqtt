// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_EVENT_SEND_HPP)
#define ASYNC_MQTT_PROTOCOL_EVENT_SEND_HPP

#include <cstddef>

#include <async_mqtt/protocol/packet/packet_variant.hpp>

namespace async_mqtt::event {

/**
 * @brief Packet send request event.
 *
 * This corresponds to @ref basic_connection::on_send().
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * @tparam PacketIdBytes The number of bytes used for the packet identifier.
 *                       According to the MQTT specification, this is 2.
 *                       You can use @ref event::send for this.
 */
template <std::size_t PacketIdBytes>
class basic_send {
public:
    /**
     * @brief Constructor.
     *
     * @param packet The packet to send request.
     * @param release_packet_id_if_send_error
     *        If a send failure occurs and `release_packet_id_if_send_error` has a value,
     *        the implementation must call @ref release_packet_id() with the value of `release_packet_id_if_send_error`.
     */
    explicit basic_send(
        basic_packet_variant<PacketIdBytes> packet,
        std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
        release_packet_id_if_send_error = std::nullopt
    )
        :packet_{force_move(packet)},
         release_packet_id_if_send_error_{release_packet_id_if_send_error}
    {
    }

    /**
     * @brief Get the packet to be sent.
     *
     * @return A const reference to the packet.
     */
    basic_packet_variant<PacketIdBytes> const& get() const {
        return packet_;
    }

    /**
     * @brief Get the packet to be sent.
     *
     * @return A reference to the packet.
     */
    basic_packet_variant<PacketIdBytes>& get() {
        return packet_;
    }

    /**
     * @brief Get the packet_id to be released if a send error occurs.
     *
     * @return The packet_id to be released (optional).
     */
    std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
    get_release_packet_id_if_send_error() const {
        return release_packet_id_if_send_error_;
    }

private:
    basic_packet_variant<PacketIdBytes> packet_;
    std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
    release_packet_id_if_send_error_;
};

/**
 * @brief Type alias of @ref basic_send (PacketIdBytes=2).
 *        This is for typical usecase (e.g. MQTT client).
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 */
using send = basic_send<2>;

} // namespace async_mqtt::event

#endif // ASYNC_MQTT_PROTOCOL_EVENT_SEND_HPP
