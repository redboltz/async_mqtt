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
 * #### Event returning member functions
 *    @li @ref basic_rv_connection::send()
 *    @li @ref basic_rv_connection::recv()
 *    @li @ref basic_rv_connection::notify_timer_fired()
 *    @li @ref basic_rv_connection::notify_closed()
 *    @li @ref basic_rv_connection::set_pingreq_send_interval()
 *    @li @ref basic_rv_connection::release_packet_id()
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
     * @li If the packet cannot be sent, an @ref error_code is added to the return vector.
     *     - Additionally, if the packet contains a packet_id, it is released, and
     *       an @ref event::packet_id_released is added to the return vector.
     *
     * @li If the packet is a @ref v3_1_1::pingreq_packet or @ref v5::pingreq_packet
     *     and Keep Alive is set, then an @ref event::timer is added to the return vector
     *     for @ref timer_kind::pingreq_send .
     *     - Additionally, if @ref set_pingresp_recv_timeout() is set to a nonzero value,
     *       an @ref event::timer is added to the return vector for @ref timer_kind::pingresp_recv.
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

    /**
     * @brief Notify that some bytes of the packet have been received.
     *
     * At most, the bytes for a single packet are processed from the input stream.
     * Once one packet has been processed, the function returns, even if additional bytes remain in the stream.
     *
     * @li If the packet is malformed or a protocol error occurs, an @ref error_code is added to the return vector.
     *     If the protocol_version is v5, @ref event::send is added to the return vector with either
     *     a @ref v5::connack_packet or a @ref v5::disconnect_packet .
     *     Finally, @ref event::close is added to the return vector.
     * @li If a complete and valid packet is constructed, @ref event::basic_packet_received is added to the return vector with
     *     the constructed packet.
     * @li If the packet_id becomes reusable, @ref event::basic_packet_id_released is added to the return vector with
     *     the packet_id.
     * @li If a timer operation is required, @ref event::timer is added to the return vector.
     *     - If the connection acts as a client, a timer operation is triggered when a
     *       PINGRESP packet is received or a CONNACK packet with the @ref property::server_keep_alive
     *       property is received.
     *     - If the connection acts as a server, a timer operation is triggered whenever
     *       a packet is received, provided that Keep Alive is activated.
     * @li If the bytes are incomplete for a packet, they are stored and processed during
     *     the next @ref recv() call.
     *
     * @param is The input stream containing some bytes of the packet.
     * @return A vector of @ref basic_event_variant for requesting caller action.
     */
    std::vector<basic_event_variant<PacketIdBytes>>
    recv(std::istream& is);

    /**
     * @brief Notify that a timer has fired.
     *
     * Timer operations are requested through @ref event::timer in the return vector.
     * Users are responsible for resetting (canceling and then setting) or canceling the timer.
     * When the timer fires, this function is expected to be called.
     *
     * @li If the kind is @ref timer_kind::pingreq_send, @ref event::send is added to the return vector with
     *     a PINGREQ packet.
     * @li If the kind is @ref timer_kind::pingreq_recv
     *     - If the protocol_version is v5, @ref event::send is added to the return vector with
     *       a @ref v5::disconnect_packet .
     *     - Finally, @ref event::close is added to the return vector.
     * @li If the kind is @ref timer_kind::pingresp_recv
     *     - If the protocol_version is v5, @ref event::send is added to the return vector with
     *       a @ref v5::disconnect_packet .
     *     - Finally, @ref event::close is added to the return vector.
     *
     * @param kind The type of timer that has fired.
     * @return A vector of @ref basic_event_variant for requesting caller action.
     */
    std::vector<basic_event_variant<PacketIdBytes>>
    notify_timer_fired(timer_kind kind);

    /**
     * @brief Notify that the underlying connection is closed.
     *
     * @li If a packet with an acquired or registered packet_id is being processed,
     *     it is released, and @ref event::basic_packet_id_released is added to the return vector.
     * @li @ref event::timer is added to the return vector for all @ref timer_kind values to cancel
     *     any active timers.
     * @li If the packet is a @ref v3_1_1::pingreq_packet or @ref v5::pingreq_packet,
     *     and Keep Alive is set, then @ref event::timer is added to the return vector for @ref timer_kind::pingreq_send.
     * @return A vector of @ref basic_event_variant for requesting caller action.
     */
    std::vector<basic_event_variant<PacketIdBytes>>
    notify_closed();

    /**
     * @brief Set the PINGREQ packet sending interval.
     *
     * Calls @ref event::timer with @ref timer_kind::pingreq_send is added to the return vector.
     *
     * @note By default, the PINGREQ packet sending interval is set to the same value as the
     *       CONNECT packet's keep-alive duration (in seconds). If the CONNACK packet includes
     *       the Server Keep Alive property, its value (in seconds) is used instead.
     *       This function overrides the default value.
     *
     * @param duration If set to zero, the timer is disabled; otherwise, the specified duration is used.
     *                 The minimum resolution is in milliseconds.
     * @return A vector of @ref basic_event_variant for requesting caller action.
     */
    std::vector<basic_event_variant<PacketIdBytes>>
    set_pingreq_send_interval(
        std::chrono::milliseconds duration
    );

    /**
     * @brief Release a packet_id.
     *
     * @li If the packet_id is currently in use, it is released and an @ref event::basic_packet_id_released
     *     event is added to the return vector;
     *     otherwise, the return vector remains empty.
     *
     * @param packet_id The packet_id to release.
     * @return A vector of @ref basic_event_variant for requesting caller action.
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
