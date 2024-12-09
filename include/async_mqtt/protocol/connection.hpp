// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_CONNECTION_HPP)
#define ASYNC_MQTT_PROTOCOL_CONNECTION_HPP

#include <memory>
#include <chrono>
#include <set>

#include <async_mqtt/protocol/detail/connection_impl_fwd.hpp>
#include <async_mqtt/protocol/connection_status.hpp>
#include <async_mqtt/protocol/protocol_version.hpp>
#include <async_mqtt/protocol/timer.hpp>
#include <async_mqtt/protocol/packet/store_packet_variant.hpp>
namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes>
class basic_connection {
    using impl_type = detail::basic_connection_impl<Role, PacketIdBytes>;

public:
    basic_connection(protocol_version ver);

    virtual ~basic_connection() = default;

    /**
     * @brief Packet sending request.
     *
     * @li If the packet cannot be sent, @ref on_error is called.
     * @li If the packet is a @ref v3_1_1::pingreq_packet or @ref v5::pingreq_packet
     *     and Keep Alive is set, then @ref on_timer_op() for @ref timer_kind::pingreq_send is called.
     *
     * @param packet The packet to be sent.
     */
    template <typename Packet>
    void
    send(Packet packet);

    /**
     * @brief Notify that some bytes of the packet have been received.
     *
     * All bytes will be processed in the input stream.
     *
     * @li If the packet is malformed or a protocol error occurs, @ref on_error is called.
     *     If the protocol_version is v5, then @ref on_send() is called with either
     *     a @ref v5::connack_packet or @ref v5::disconnect_packet .
     *     Finally, @ref on_close() is called.
     * @li If a complete and valid packet is constructed, @ref on_receive() is called with
     *     the constructed packet.
     * @li If the packet_id becomes reusable, @ref on_packet_id_release() is called with
     *     the packet_id.
     * @li If a timer operation is required, @ref on_timer_op() is called.
     *     - If the connection acts as a client, a timer operation is required when a
     *       PINGRESP packet is received or a CONNACK packet with @ref property::server_keep_alive
     *       is received.
     *     - If the connection acts as a server, a timer operation is required whenever
     *       a packet is received, provided that Keep Alive is activated.
     * @li If the bytes are incomplete for a packet, they are stored and processed during
     *     the next @ref recv() call.
     *
     * @param is Input stream containing some bytes of the packet.
     */
    void
    recv(std::istream& is);

    /**
     * @brief Notify that a timer has been fired.
     *
     * Timer operations are requested through @ref on_timer_op().
     * Users are responsible for setting, resetting, or canceling the timer. When the timer fires,
     * this function is expected to be called.
     *
     * @param kind The kind of timer.
     */
    void
    notify_timer_fired(timer_kind kind);

    /**
     * @brief notify the connection is closed
     *
     * @li if the packet can't send @ref on_error is called.
     * @li if the packet is @ref v3_1_1::pingreq_packet or @ref v5::pingreq_packet,
     *     and Keep Alive is set, then @ref on_timer_op() for @ref timer_kind::pingreq_send is called.
     *
     * @param packet The packet to send.
     */
    void
    notify_closed();

    /**
     * @brief Set the PINGREQ packet sending interval.
     *
     * @note By default, the PINGREQ packet sending interval is set to the same value as the
     *       CONNECT packet's keep-alive duration in seconds. If the CONNACK packet includes
     *       the Server Keep Alive property, its value (in seconds) is used instead.
     *       This function overrides the default value.
     *
     * @param duration If set to zero, the timer is disabled; otherwise, the specified duration is used.
     *                 The minimum resolution is in milliseconds.
     */
    void
    set_pingreq_send_interval(
        std::chrono::milliseconds duration
    );

    /**
     * @brief Get the receive maximum vacancy for sending PUBLISH (QoS1, QoS2) packets.
     *
     * When the Receive Maximum property is included in a CONNECT/CONNACK packet received
     * from the other side of the connection, the maximum number of inflight packets for
     * sending is determined by that property.
     * If no Receive Maximum property is provided, there is no limit.
     *
     * @note Even if the Receive Maximum property imposes no limit, the number of packet_id
     *       values is inherently limited. The maximum value is calculated as
     *       2^(PacketIdBytes * 8) - 1. For example, if PacketIdBytes is 2 (the default in MQTT),
     *       the packet_id limit is 65535.
     *
     * @return If there is no limit, returns std::nullopt; otherwise, returns the current
     *         number of PUBLISH (QoS1, QoS2) packets that can be sent.
     */
    std::optional<std::size_t>
    get_receive_maximum_vacancy_for_send() const;

    /**
     * @brief set offline publish support
     * \n This function should be called before async_send() call.
     * @note By default offline publish is not supported.
     * @param val if true, offline publish is supported, otherwise not supported
     */
    void set_offline_publish(bool val);

    /**
     * @brief auto publish response setter.
     * @note By default not automatically sending.
     * @param val if true, puback, pubrec, pubrel, and pubcomp are automatically sent
     */
    void set_auto_pub_response(bool val);

    /**
     * @brief auto publish response setter.
     * @note By default not automatically sending.
     * @param val if true, puback, pubrec, pubrel, and pubcomp are automatically sent
     */
    void set_auto_ping_response(bool val);

    /**
     * @brief auto map (allocate) topic alias on send PUBLISH packet.
     * If all topic aliases are used, then overwrite by LRU algorithm.
     * \n This function should be called before async_send() call.
     * @note By default not automatically mapping.
     * @param val if true, enable auto mapping, otherwise disable.
     */
    void set_auto_map_topic_alias_send(bool val);

    /**
     * @brief auto replace topic with corresponding topic alias on send PUBLISH packet.
     * Registering topic alias need to do manually.
     * \n This function should be called before async_send() call.
     * @note By default not automatically replacing.
     * @param val if true, enable auto replacing, otherwise disable.
     */
    void set_auto_replace_topic_alias_send(bool val);

    /**
     * @brief Set timeout for receiving PINGRESP packet after PINGREQ packet is sent.
     * If the timer is fired, then the underlying layer is closed from the client side.
     * If the protocol_version is v5, then send DISCONNECT packet with the reason code
     * disconnect_reason_code::keep_alive_timeout automatically before underlying layer is closed.
     * \n This function should be called before async_send() call.
     * @note By default timeout is not set.
     * @param duration if zero, timer is not set; otherwise duration is set.
     *                 The minimum resolution is in milliseconds.
     */
    void set_pingresp_recv_timeout(std::chrono::milliseconds duration);

    /**
     * @brief acuire unique packet_id.
     * @return std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
     * if acquired return acquired packet id, otherwise std::nullopt
     */
    std::optional<typename basic_packet_id_type<PacketIdBytes>::type> acquire_unique_packet_id();

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @return If true, success, otherwise the packet_id has already been used.
     */
    bool register_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id);

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     * @return event list
     */
    void
    release_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id);

    /**
     * @brief Get processed but not released QoS2 packet ids
     *        This function should be called after disconnection
     * @return set of packet_ids
     */
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> get_qos2_publish_handled_pids() const;

    /**
     * @brief Restore processed but not released QoS2 packet ids
     *        This function should be called before receive the first publish
     * @param pids packet ids
     */
    void restore_qos2_publish_handled_pids(std::set<typename basic_packet_id_type<PacketIdBytes>::type> pids);

    /**
     * @brief restore packets
     *        the restored packets would automatically send when CONNACK packet is received
     * @param pvs packets to restore
     */
    void restore_packets(
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs
    );

    /**
     * @brief get stored packets
     *        sotred packets mean inflight packets.
     *        @li PUBLISH packet (QoS1) not received PUBACK packet
     *        @li PUBLISH packet (QoS1) not received PUBREC packet
     *        @li PUBREL  packet not received PUBCOMP packet
     * @return std::vector<basic_store_packet_variant<PacketIdBytes>>
     */
    std::vector<basic_store_packet_variant<PacketIdBytes>> get_stored_packets() const;

    /**
     * @brief get MQTT protocol version
     * @return MQTT protocol version
     */
    protocol_version get_protocol_version() const;

    /**
     * @brief Get MQTT PUBLISH packet processing status
     * @param pid packet_id corresponding to the publish packet.
     * @return If the packet is processing, then true, otherwise false.
     */
    bool is_publish_processing(typename basic_packet_id_type<PacketIdBytes>::type pid) const;

    /**
     * @brief Regulate publish packet for store
     *        If topic is empty, extract topic from topic alias, and remove topic alias
     *        Otherwise, remove topic alias if exists.
     * @param packet packet to regulate
     * @return error_code for repoting error
     */
    error_code regulate_for_store(
        v5::basic_publish_packet<PacketIdBytes>& packet
    ) const;

    connection_status get_connection_status() const;

#if defined(ASYNC_MQTT_MRDOCS)
public:
#else  // defined(ASYNC_MQTT_MRDOCS)
private:
#endif // defined(ASYNC_MQTT_MRDOCS)

    /**
     * @brief Handler for error notifications.
     *
     * This function is called when an error occurs in @ref send(), @ref recv(), @ref notify_timer_fired(),
     * @ref set_pingreq_send_interval(), or @ref release_packet_id() **before these functions return**.
     *
     * @param ec The error_code indicating the error.
     */
    virtual void on_error(error_code ec) = 0;

    /**
     * @brief Handler for send requests.
     *
     * This function is called when a packet needs to be sent due to a situation arising in
     * @ref send() or @ref recv() **before these functions return**.
     * The implementation is responsible for sending the packet to the socket in its specific environment.
     * If a send failure occurs and `release_packet_id_if_send_error` has a value,
     * the implementation must call @ref release_packet_id() with the value of `release_packet_id_if_send_error`.
     *
     * @param packet The packet to send. You can retrieve a `ConstBufferSequence` using
     *               @ref basic_packet_variant::const_buffer_sequence().
     *               If a continuous buffer is required, you can use
     *               <a href="../to_string-0d.html">to_string</a>() to the result of
     *               @ref basic_packet_variant::const_buffer_sequence().
     * @param release_packet_id_if_send_error
     *        Specifies the packet_id to release if a send error occurs during implementation.
     *        If `std::nullopt`, no action is taken.
     */
    virtual void on_send(
        basic_packet_variant<PacketIdBytes> packet,
        std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
        release_packet_id_if_send_error = std::nullopt
    ) = 0;

    /**
     * @brief Handler for packet_id release notifications.
     *
     * This function is called when a packet_id is released in
     * @ref release_packet_id(), @ref send(), or @ref recv() **before these functions return**.
     * After this notification, the packet_id becomes reusable.
     *
     * @param packet_id The packet_id that was released.
     */
    virtual void on_packet_id_release(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id
    ) = 0;

    /**
     * @brief Handler for packet received notifications.
     *
     * This function is called when a complete packet is received in
     * @ref recv() **before the function returns**.
     *
     * @param packet_id The packet_id associated with the received packet.
     */
    virtual void on_receive(
        basic_packet_variant<PacketIdBytes> packet
    ) = 0;

    /**
     * @brief Handler for timer operation requests.
     *
     * This function is called when a timer operation is required in situations arising from
     * @ref set_pingreq_send_interval(), @ref send(), or @ref recv() **before these functions return**.
     *
     * @param timer_op The type of operation (set, reset, or cancel).
     * @param kind     The kind of timer.
     */
    virtual void on_timer_op(
        timer_op op,
        timer_kind kind,
          std::optional<std::chrono::milliseconds> ms = std::nullopt
    ) = 0;

    /**
     * @brief Handler for socket closing requests.
     *
     * This function is called when a socket closing operation is required due to a situation arising in
     * @ref notify_timer_fired(), @ref send(), or @ref recv() **before these functions return**.
     */
    virtual void on_close() = 0;

private:
    friend class detail::basic_connection_impl<Role, PacketIdBytes>;
    std::shared_ptr<impl_type> impl_;
};

template <role Role>
using connection = basic_connection<Role, 2>;

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/impl/connection_impl.ipp>
#include <async_mqtt/protocol/impl/connection_send.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PROTOCOL_CONNECTION_HPP
