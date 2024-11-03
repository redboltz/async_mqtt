// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_RECV_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_RECV_HPP

#include <async_mqtt/endpoint.hpp>

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/impl/buffer_to_packet_variant.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

namespace async_mqtt {

namespace detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
recv_op {
    this_type_sp ep;
    std::optional<filter> fil = std::nullopt;
    std::set<control_packet_type> types = {};
    std::optional<error_code> decided_error = std::nullopt;
    std::optional<basic_packet_variant<PacketIdBytes>> recv_packet = std::nullopt;
    using events_type = std::vector<basic_event_variant<PacketIdBytes>>;
    using events_it_type = typename events_type::iterator;
    std::shared_ptr<events_type> events = nullptr;
    events_it_type it = events_it_type{};
    enum { dispatch, read, protocol_read, process_events, sent, complete } state = dispatch;

    template <typename Self>
    bool process_one_event(
        Self& self
    ) {
        auto& a_ep{*ep};
        auto& event{*it++};
        std::visit(
            overload{
                [&](error_code ec) {
                    decided_error.emplace(ec);
                    return true;
                },
                [&](basic_event_send<PacketIdBytes>& ev) {
                    state = sent;
                    a_ep.async_send(
                        force_move(ev.get()),
                        force_move(self)
                    );
                    BOOST_ASSERT(!ev.get_release_packet_id_if_send_error());
                    return false;
                },
                [&](basic_event_packet_id_released<PacketIdBytes> ev) {
                    a_ep.notify_release_pid(ev.get());
                    return true;
                },
                [&](basic_event_packet_received<PacketIdBytes> ev) {
                    recv_packet.emplace(force_move(ev.get()));
                    return true;
                },
                [&](event_timer ev) {
                    switch (ev.get_timer_for()) {
                    case timer::pingresp_recv:
                        if (ev.get_op() == event_timer::op_type::reset) {
                            reset_pingresp_recv_timer(ep, ev.get_ms());
                        }
                        else {
                            BOOST_ASSERT(false);
                        }
                        break;
                    default:
                        BOOST_ASSERT(false);
                        break;
                    }
                    return true;
                },
                [&](event_close) {
                    state = complete;
                    a_ep.async_close(force_move(self));
                    return false;
                }
            },
            event
        );
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code ec = error_code{},
        std::size_t bytes_transferred = 0
    ) {
        auto& a_ep{*ep};
        if (ec) {
            ASYNC_MQTT_LOG("mqtt_impl", info)
                << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                << "recv error:" << ec.message();
            if (ec == as::error::operation_aborted) {
                // on cancel, not close the connection
                self.complete(
                    ec,
                    force_move(recv_packet)
                );
            }
            else {
                decided_error.emplace(ec);
                state = close;
                auto ep_copy = ep;
                a_ep.async_close(
                    force_move(ep_copy),
                    force_move(self)
                );
            }
            return;
        }

        switch (state) {
        case dispatch: {
            state = read;
            as::dispatch(
                a_ep.get_executor(),
                force_move(self)
            );
        } break;
        case read: {
            state = protocol_read;
            a_ep.mbs_ = a_ep.read_buf_.prepare(a_ep.bulk_read_buffer_size_);
            a_ep.stream_.async_read_some(
                a_ep.mbs_,
                force_move(self)
            );
        } break;
        case protocol_read: {
            state = process_events;
            auto b = as::buffer_sequence_begin(a_ep.mbs_);
            auto e = std::next(b, bytes_transferred);
            events = std::make_shared<events_type>(a_ep.con_.recv(b, e));
            for (it = events->begin(); it != events->end(); ++it) {
                if (!process_one_event(self)) return;
            }
            state = complete; // all events processed
        } break;
        case sent: {
            for (it = events->begin(); it != events->end(); ++it) {
                if (!process_one_event(self)) return;
            }
            state = complete; // all events processed
        } break;
        case complete: {
            error_code ret_rc;
            if (decided_error) ret_rc = *decided_error;
            basic_packet_variant<PacketIdBytes> ret_packet;
            if (recv_packet) ret_packet = force_move(*recv_packet);
            self.complete(
                ec,
                force_move(recv_packet)
            );
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    void send_publish_from_queue();

    bool process_qos2_publish(
        protocol_version ver,
        typename basic_packet_id_type<PacketIdBytes>::type packet_id
    );

    void handle_v3_1_1_connect(v3_1_1::connect_packet& p);
    void handle_v5_connect(v5::connect_packet& p);
    void handle_v3_1_1_connack(v3_1_1::connack_packet& p);
    void handle_v5_connack(v5::connack_packet& p);
};

} // namespace detail

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_recv(
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "recv";
    BOOST_ASSERT(impl_);
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::optional<packet_variant_type>)
        >(
            typename impl_type::recv_op{
                impl_
            },
            token,
            get_executor()
        );
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_recv(
    std::set<control_packet_type> types,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "recv";
    BOOST_ASSERT(impl_);
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::optional<packet_variant_type>)
        >(
            typename impl_type::recv_op{
                impl_,
                filter::match,
                force_move(types)
            },
            token,
            get_executor()
        );
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_recv(
    filter fil,
    std::set<control_packet_type> types,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "recv";
    BOOST_ASSERT(impl_);
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::optional<packet_variant_type>)
        >(
            typename impl_type::recv_op{
                impl_,
                fil,
                force_move(types)
            },
            token,
            get_executor()
        );
}

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/impl/endpoint_recv.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RECV_HPP
