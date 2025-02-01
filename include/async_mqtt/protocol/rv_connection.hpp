// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_RV_CONNECTION_HPP)
#define ASYNC_MQTT_PROTOCOL_RV_CONNECTION_HPP

#include <async_mqtt/protocol/connection.hpp>
#include <async_mqtt/protocol/event/event_variant.hpp>

namespace async_mqtt {

/**
 * @brief Return value (rv)-based MQTT connection.
 *
 * An I/O-independent MQTT protocol state machine.
 * @li Manages connection status.
 * @li Manages packet identifiers.
 * @li Manages Topic Aliases.
 * @li Manages packet resending on reconnection.
 * @li Manages PINGREQ/PINGRESP timers.
 * @li Manages automatic response packet sending.
 *
 * All requests and settings are implemented as synchronous functions.
 * When caller action is required, a @ref std::vector of @ref basic_event_variant
 * corresponding to the required actions is returned.
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * @tparam Role          The role used for checking whether packets can be sent.
 * @tparam PacketIdBytes The number of bytes used for the packet identifier.
 *                       According to the MQTT specification, this is 2.
 *                       You can use @ref connection for this.
 */
template <role Role, std::size_t PacketIdBytes>
class basic_rv_connection : public basic_connection<Role, PacketIdBytes> {
    using base_type = basic_connection<Role, PacketIdBytes>;

public:

    /**
     * @copydoc basic_connection::basic_connection
     */
    basic_rv_connection(protocol_version ver);

    /**
     * @brief Packet sending request.
     *
     * If the packet cannot be sent, an @ref error_code is added to the return vector.
     * Additionally, if the packet contains a packet_id, it is released, and
     * an @ref event::packet_id_released is added to the return vector.
     *
     * If the packet is a @ref v3_1_1::pingreq_packet or @ref v5::pingreq_packet
     * and Keep Alive is set, then an @ref event::timer is added to the return vector
     * for @ref timer_kind::pingreq_send.
     * Additionally, if @ref set_pingresp_recv_timeout() is set to a nonzero value,
     * an @ref event::timer is added to the return vector for @ref timer_kind::pingresp_recv.
     *
     * @li If a disconnection occurs during the sending process, an @ref event::timer is added to
     *     the return vector for all @ref timer_kind values to cancel any active timers.
     *
     * @param packet The packet to be sent.
     * @tparam Packet The type of the packet.
     * @return A vector of @ref basic_event_variant for requesting caller action.
     */
    template <typename Packet>
    std::vector<basic_event_variant<PacketIdBytes>>
    send(Packet packet);

    std::vector<basic_event_variant<PacketIdBytes>>
    recv(std::istream& is);

    std::vector<basic_event_variant<PacketIdBytes>>
    notify_timer_fired(timer_kind kind);

    std::vector<basic_event_variant<PacketIdBytes>>
    notify_closed();

    std::vector<basic_event_variant<PacketIdBytes>>
    set_pingreq_send_interval(
        std::chrono::milliseconds duration
    );

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     * @return event list
     */
    std::vector<basic_event_variant<PacketIdBytes>>
    release_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id);

private:

    void on_error(error_code ec) override final;

    void on_send(
        basic_packet_variant<PacketIdBytes> packet,
        std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
        release_packet_id_if_send_error = std::nullopt
    ) override final;

    void on_packet_id_release(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id
    ) override final;

    void on_receive(
        basic_packet_variant<PacketIdBytes> packet
    ) override final;

    void on_timer_op(
        timer_op op,
        timer_kind kind,
          std::optional<std::chrono::milliseconds> ms = std::nullopt
    ) override final;

    void on_close() override final;

private:
    std::vector<basic_event_variant<PacketIdBytes>> events_;
};

template <role Role>
using rv_connection = basic_rv_connection<Role, 2>;

} // namespace async_mqtt

#include <async_mqtt/protocol/impl/rv_connection.hpp>
#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/impl/rv_connection.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PROTOCOL_RV_CONNECTION_HPP
