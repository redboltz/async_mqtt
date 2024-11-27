// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_EVENT_CONNECTION_HPP)
#define ASYNC_MQTT_PROTOCOL_EVENT_CONNECTION_HPP

#include <async_mqtt/protocol/connection.hpp>
#include <async_mqtt/protocol/event_variant.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes>
class basic_event_connection : public basic_connection<Role, PacketIdBytes> {
    using base_type = basic_connection<Role, PacketIdBytes>;

public:
    basic_event_connection(protocol_version ver);

    template <typename Packet>
    std::vector<basic_event_variant<PacketIdBytes>>
    send(Packet packet);

    std::vector<basic_event_variant<PacketIdBytes>>
    recv(std::istream& is);

    std::vector<basic_event_variant<PacketIdBytes>>
    notify_timer_fired(timer_kind kind);

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
using event_connection = basic_event_connection<Role, 2>;

} // namespace async_mqtt

#include <async_mqtt/protocol/impl/event_connection.hpp>
#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/impl/event_connection.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PROTOCOL_EVENT_CONNECTION_HPP
