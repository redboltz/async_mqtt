// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_SEND_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_SEND_HPP

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/impl/endpoint_impl.hpp>
#include <async_mqtt/protocol/event/timer.hpp>
#include <async_mqtt/protocol/event/close.hpp>
#include <async_mqtt/protocol/event/packet_received.hpp>

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
    bool disconnect_sent_just_before = false;
    enum { dispatch, write, sent, close, complete } state = dispatch;

    template <typename Self>
    bool process_one_event(
        Self& self
    ) {
        auto& a_ep{*ep};
        auto& event{*it++};
        return std::visit(
            overload{
                [&](error_code ec) {
                    if (ec == disconnect_reason_code::receive_maximum_exceeded) {
                        if constexpr (std::is_same_v<Packet, v5::basic_publish_packet<PacketIdBytes>>) {
                            auto success = a_ep.register_packet_id(packet.packet_id());
                            if (success) {
                                a_ep.enqueue_publish(packet);
                                decided_error.emplace(
                                    make_error_code(
                                        mqtt_error::packet_enqueued
                                    )
                                );
                            }
                            else {
                                BOOST_ASSERT(false);
                            }
                            return true;
                        }
                    }
                    decided_error.emplace(ec);
                    return true;
                },
                [&](async_mqtt::event::timer const& ev) {
                    switch (ev.get_kind()) {
                    case timer_kind::pingreq_send:
                        if (ev.get_op() == timer_op::reset) {
                            reset_pingreq_send_timer(ep, ev.get_ms());
                        }
                        else {
                            BOOST_ASSERT(false);
                        }
                        break;
                    case timer_kind::pingresp_recv:
                        if (ev.get_op() == timer_op::reset) {
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
                [&](async_mqtt::event::basic_packet_id_released<PacketIdBytes> const& ev) {
                    a_ep.notify_release_pid(ev.get());
                    return true;
                },
                [&](async_mqtt::event::basic_send<PacketIdBytes>& ev) {
                    state = sent;
                    disconnect_sent_just_before = ev.get().type() == control_packet_type::disconnect;
                    a_ep.stream_.async_write_packet(
                        force_move(ev.get()),
                        as::append(
                            force_move(self),
                            ev.get_release_packet_id_if_send_error()
                        )
                    );
                    return false;
                },
                [&](async_mqtt::event::close) {
                    state = close;
                    if (disconnect_sent_just_before &&
                        a_ep.duration_close_by_disconnect_ != std::chrono::milliseconds::zero()
                    ) {
                        a_ep.tim_close_by_disconnect_.expires_after(a_ep.duration_close_by_disconnect_);
                        a_ep.tim_close_by_disconnect_.async_wait(
                            force_move(self)
                        );
                    }
                    else {
                        as::post(
                            a_ep.get_executor(),
                            force_move(self)
                        );
                    }
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
            ASYNC_MQTT_LOG("mqtt_impl", info)
                << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                << "send error:" << ec.message();
            if (release_pid_opt) {
                a_ep.con_.release_packet_id(*release_pid_opt);
            }
            self.complete(ec);
            return;
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
        case close: {
            state = complete;
            auto ep_copy{ep};
            async_close(
                force_move(ep_copy),
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
    return
        impl_type::async_send(
            impl_,
            force_move(packet),
            std::forward<CompletionToken>(token)
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_SEND_HPP
