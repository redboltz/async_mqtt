// Copyright Takatoshi Kondo 2025
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_SEND_HPP)
#define ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_SEND_HPP

#include <async_mqtt/protocol/impl/connection_impl.hpp>

namespace async_mqtt::detail {

template <role Role, std::size_t PacketIdBytes>
template <typename Packet>
inline
void
basic_connection_impl<Role, PacketIdBytes>::
send(Packet packet) {
    auto send_and_post_process =
        [&](auto&& actual_packet) {
            if (process_send_packet(actual_packet)) {
                if constexpr(is_connack<std::remove_reference_t<decltype(actual_packet)>>()) {
                    // server send stored packets after connack sent
                    send_stored();
                }
                if constexpr(Role == role::client || Role == role::any) {
                    if (is_client_ && pingreq_send_interval_ms_) {
                        if (status_ == connection_status::disconnected) return;
                        pingreq_send_set_ = true;
                        con_.on_timer_op(
                            timer_op::reset,
                            timer_kind::pingreq_send,
                            *pingreq_send_interval_ms_
                        );
                    }
                }
            }
        };

    if constexpr(
        std::is_same_v<std::decay_t<Packet>, basic_packet_variant<PacketIdBytes>> ||
        std::is_same_v<std::decay_t<Packet>, basic_store_packet_variant<PacketIdBytes>>
    ) {
        force_move(packet).visit(
            [&](auto actual_packet) {
                // MQTT protocol sendable packet check (Runtime)
                using packet_type = std::decay_t<decltype(actual_packet)>;
                if (
                    !(
                        (can_send_as_client(Role) && is_client_sendable<packet_type>()) ||
                        (can_send_as_server(Role) && is_server_sendable<packet_type>())
                    )
                ) {
                    con_.on_error(
                        make_error_code(
                            mqtt_error::packet_not_allowed_to_send
                        )
                    );
                    return;
                }
                send_and_post_process(force_move(actual_packet));
            }
        );
    }
    else {
    // MQTT protocol sendable packet check (Compile time)
    using packet_type = std::decay_t<Packet>;

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)
        if constexpr (
            !(
                (can_send_as_client(Role) && is_client_sendable<packet_type>()) ||
                (can_send_as_server(Role) && is_server_sendable<packet_type>())
            )
        ) {
            con_.on_error(
                make_error_code(
                    mqtt_error::packet_not_allowed_to_send
                )
            );
            return;
        }
#else  // defined(ASYNC_MQTT_SEPARATE_COMPILATION)
        static_assert(
            (can_send_as_client(Role) && is_client_sendable<packet_type>()) ||
            (can_send_as_server(Role) && is_server_sendable<packet_type>()),
            "packet not allowed to send. Check the role and packet type."
        );
#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)
        send_and_post_process(std::forward<Packet>(packet));
    }
}

} // namespace async_mqtt::detail

#endif // ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_SEND_HPP
