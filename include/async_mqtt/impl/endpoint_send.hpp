// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_SEND_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_SEND_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

namespace detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename Packet>
struct basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
send_op {
    this_type_sp ep;
    Packet packet;
    bool from_queue = false;
    std::optional<error_code> decided_error = std::nullopt;
    using events_type = std::vector<basic_event_variant<PacketIdBytes>>;
    using events_it_type = typename events_type::iterator;
    std::shared_ptr<events_type> events = nullptr;
    events_it_type it = events_it_type{};
    enum { dispatch, write, sent, complete } state = dispatch;

    template <typename Self>
    bool process_one_event(
        Self& self
    ) {
        auto& a_ep{*ep};
        auto& event{*it++};
        return std::visit(
            overload{
                [&](error_code ec) {
                    decided_error.emplace(ec);
                    return true;
                },
                [&](event_timer const& ev) {
                    switch (ev.get_timer_for()) {
                    case timer::pingreq_send:
                        if (ev.get_op() == event_timer::op_type::reset) {
                            reset_pingreq_send_timer(ep, ev.get_ms());
                        }
                        else {
                            BOOST_ASSERT(false);
                        }
                        break;
                    case timer::pingresp_recv:
                        if (ev.get_op() == event_timer::op_type::reset) {
                            // TBD
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
                [&](basic_event_packet_id_released<PacketIdBytes> const& ev) {
                    a_ep.notify_release_pid(ev.get());
                    return true;
                },
                [&](event_send& ev) {
                    state = sent;
                    a_ep.stream_.async_write_packet(
                        force_move(ev.get()),
                        as::append(
                            force_move(self),
                            ev.get_release_packet_id_if_send_error()
                        )
                    );
                    return false;
                },
                [&](event_close) {
                    state = complete;
                    auto ep_copy{ep};
                    async_close(
                        force_move(ep_copy),
                        force_move(self)
                    );
                    return false;
                },
                [&](auto const&) {
                    BOOST_ASSERT(false);
                    return true;
                }
            },
            event
        );
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code ec = error_code{},
        std::size_t /*bytes_transferred*/ = 0,
        std::optional<typename basic_packet_id_type<PacketIdBytes>::type> release_pid_opt = std::nullopt
    ) {
        auto& a_ep{*ep};
        if (ec) {
            if (ec == disconnect_reason_code::receive_maximum_exceeded) {
                if constexpr (std::is_same_v<Packet, v5::basic_publish_packet<PacketIdBytes>>) {
                    a_ep.enqueue_publish(packet);
                    self.complete(
                        make_error_code(
                            mqtt_error::packet_enqueued
                        )
                    );
                    return;
                }
                BOOST_ASSERT(false);
            }
            else {
                ASYNC_MQTT_LOG("mqtt_impl", info)
                    << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                    << "send error:" << ec.message();
                if (release_pid_opt) {
                    a_ep.con_.release_packet_id(*release_pid_opt);
                }
                self.complete(ec);
                return;
            }
        }

        switch (state) {
        case dispatch: {
            state = write;
            as::dispatch(
                a_ep.get_executor(),
                force_move(self)
            );
        } break;
        case write: {
            events = std::make_shared<events_type>(a_ep.con_.send(packet));
            it = events->begin();
            while (it != events->end()) {
                if (!process_one_event(self)) return;
            }
            state = complete; // all events processed
            as::dispatch(
                a_ep.get_executor(),
                force_move(self)
            );
        } break;
        case sent: {
            while (it != events->end()) {
                if (!process_one_event(self)) return;
            }
            state = complete; // all events processed
            as::dispatch(
                a_ep.get_executor(),
                force_move(self)
            );
        } break;
        case complete: {
            if (decided_error) {
                self.complete(*decided_error);
            }
            else {
                self.complete(ec);
            }
        } break;
        }
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename Packet, typename CompletionToken>
auto
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::async_send(
    this_type_sp impl,
    Packet packet,
    bool from_queue,
    CompletionToken&& token
) {
    BOOST_ASSERT(impl);
    auto exe = impl->get_executor();
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            send_op<Packet>{
                force_move(impl),
                force_move(packet),
                from_queue
            },
            token,
            exe
        );
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename Packet, typename CompletionToken>
auto
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::async_send(
    this_type_sp impl,
    Packet packet,
    CompletionToken&& token
) {
    BOOST_ASSERT(impl);
    auto exe = impl->get_executor();
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            send_op<Packet>{
                force_move(impl),
                force_move(packet),
                false
            },
            token,
            exe
        );
}

} // namespace detail

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename Packet, typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_send(
    Packet packet,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "send:" << packet;
    BOOST_ASSERT(impl_);
    if constexpr(!std::is_same_v<Packet, basic_packet_variant<PacketIdBytes>>) {
        static_assert(
            (impl_type::can_send_as_client(Role) && is_client_sendable<std::decay_t<Packet>>()) ||
            (impl_type::can_send_as_server(Role) && is_server_sendable<std::decay_t<Packet>>()),
            "Packet cannot be send by MQTT protocol"
        );
    }

    return
        impl_type::async_send(
            impl_,
            force_move(packet),
            std::forward<CompletionToken>(token)
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_SEND_HPP
