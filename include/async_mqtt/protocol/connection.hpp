// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_CONNECTION_HPP)
#define ASYNC_MQTT_PROTOCOL_CONNECTION_HPP

#include <memory>

#include <async_mqtt/protocol/detail/connection_impl.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes>
class basic_connection {
    using impl_type = detail::basic_connection_impl<Role, PacketIdBytes>;

public:
    basic_connection(protocol_version ver);

    template <typename Packet>
    std::vector<basic_event_variant<PacketIdBytes>>
    send(Packet packet);

    template <typename Begin, typename End>
    std::vector<basic_event_variant<PacketIdBytes>>
    recv(Begin b, End d);

    std::vector<basic_event_variant<PacketIdBytes>>
    notify_timer_fired(timer kind);

    void
    notify_closed();

    std::vector<basic_event_variant<PacketIdBytes>>
    set_pingreq_send_interval(
        std::chrono::milliseconds duration
    );

    std::size_t get_receive_maximum_vacancy_for_send() const;

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
    std::vector<basic_event_variant<PacketIdBytes>>
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
    error_code  regulate_for_store(
        v5::basic_publish_packet<PacketIdBytes>& packet
    ) const;

    connection_status get_connection_status() const;

private:
    std::shared_ptr<impl_type> impl_;
};

} // namespace async_mqtt

#include <async_mqtt/protocol/impl/connection_send.hpp>
#include <async_mqtt/protocol/impl/connection_recv.hpp>

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/impl/connection_impl.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PROTOCOL_CONNECTION_HPP
