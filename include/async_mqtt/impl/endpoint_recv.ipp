// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_RECV_IPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_RECV_IPP

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/impl/endpoint_impl.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
recv_op {
    this_type_sp ep;
    std::optional<filter> fil = std::nullopt;
    std::set<control_packet_type> types = {};
    std::optional<error_code> decided_error = std::nullopt;
    std::optional<basic_packet_variant<PacketIdBytes>> recv_packet = std::nullopt;
    bool try_resend_from_queue = false;
    enum { dispatch, prev, read, protocol_read, sent, complete } state = dispatch;

    template <typename Self>
    bool process_one_event(
        Self& self
    ) {
        auto& a_ep{*ep};
        auto event{force_move(a_ep.recv_events_.front())};
        a_ep.recv_events_.pop_front();
        return std::visit(
            overload{
                [&](error_code ec) {
                    decided_error.emplace(ec);
                    return true;
                },
                [&](async_mqtt::event::basic_send<PacketIdBytes>& ev) {
                    state = sent;
                    auto ep_copy{ep};
                    async_send(
                        force_move(ep_copy),
                        force_move(ev.get()),
                        force_move(self)
                    );
                    BOOST_ASSERT(!ev.get_release_packet_id_if_send_error());
                    return false;
                },
                [&](async_mqtt::event::basic_packet_id_released<PacketIdBytes> ev) {
                    a_ep.notify_release_pid(ev.get());
                    try_resend_from_queue = true;
                    return true;
                },
                [&](async_mqtt::event::basic_packet_received<PacketIdBytes> ev) {
                    if (recv_packet) {
                        // rest events would be processed the next async_recv call
                        // back the event to the recv_events_ for the next async_recv
                        a_ep.recv_events_.push_front(force_move(event));
                        state = complete;
                        as::post(
                            a_ep.get_executor(),
                            force_move(self)
                        );
                        return false;
                    }
                    else {
                        recv_packet.emplace(force_move(ev.get()));
                    }
                    return true;
                },
                [&](async_mqtt::event::timer ev) {
                    switch (ev.get_kind()) {
                    case timer_kind::pingreq_send:
                        // receive server keep alive property in connack
                        if (ev.get_op() == timer_op::reset) {
                            reset_pingreq_send_timer(ep, ev.get_ms());
                        }
                        else {
                            BOOST_ASSERT(false);
                        }
                        break;
                    case timer_kind::pingreq_recv:
                        switch (ev.get_op()) {
                        case timer_op::set:
                            set_pingreq_recv_timer(ep, ev.get_ms());
                            break;
                        case timer_op::reset:
                            reset_pingreq_recv_timer(ep, ev.get_ms());
                            break;
                        case timer_op::cancel:
                            cancel_pingreq_recv_timer(ep);
                            break;
                        }
                        break;
                    case timer_kind::pingresp_recv:
                        if (ev.get_op() == timer_op::cancel) {
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
                [&](async_mqtt::event::close) {
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
                state = complete;
                decided_error.emplace(ec);
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
            state = prev;
            as::dispatch(
                a_ep.get_executor(),
                force_move(self)
            );
        } break;
        case prev: {
            while (!a_ep.recv_events_.empty()) {
                if (!process_one_event(self)) return;
            }
            // all previous events processed
            // and receive event is included in them
            if (recv_packet) {
                state = complete;
                as::post(
                    a_ep.get_executor(),
                    force_move(self)
                );
            }
            else {
                state = read;
                as::dispatch(
                    a_ep.get_executor(),
                    force_move(self)
                );
            }
        } break;
        case read: {
            state = protocol_read;
            a_ep.stream_.async_read_some(
                a_ep.read_buf_.prepare(a_ep.read_buffer_size_),
                force_move(self)
            );
        } break;
        case protocol_read: {
            a_ep.read_buf_.commit(bytes_transferred);
            std::istream is{&a_ep.read_buf_};
            auto events{a_ep.con_.recv(is)};
            std::move(events.begin(), events.end(), std::back_inserter(a_ep.recv_events_));
            if (a_ep.recv_events_.empty()) {
                // required more bytes
                state = read;
                as::dispatch(
                    a_ep.get_executor(),
                    force_move(self)
                );
            }
            else {
                while (!a_ep.recv_events_.empty()) {
                    if (!process_one_event(self)) return;
                }
                state = complete; // all events processed
                as::post(
                    a_ep.get_executor(),
                    force_move(self)
                );
            }
        } break;
        case sent: {
            while (!a_ep.recv_events_.empty()) {
                if (!process_one_event(self)) return;
            }
            // all events processed
            if (recv_packet || decided_error) {
                state = complete;
                as::post(
                    a_ep.get_executor(),
                    force_move(self)
                );
            }
            else {
                // auto response send but no received packet
                // (PUBLISH QoS2 received again after PUBREC sent)
                state = read;
                as::dispatch(
                    a_ep.get_executor(),
                    force_move(self)
                );
            }
        } break;
        case complete: {
            if (decided_error) {
                self.complete(
                    *decided_error,
                    std::nullopt
                );
            }
            else {
                BOOST_ASSERT(recv_packet);
                if (fil) {
                    auto type = recv_packet->type();
                    if ((*fil == filter::match  && types.find(type) == types.end()) ||
                        (*fil == filter::except && types.find(type) != types.end())
                    ) {
                        // read the next packet
                        state = prev;
                        recv_packet.reset();
                        as::dispatch(
                            a_ep.get_executor(),
                            force_move(self)
                        );
                        return;
                    }
                }
                if (try_resend_from_queue) {
                    send_publish_from_queue();
                }
                self.complete(
                    error_code{},
                    force_move(recv_packet)
                );
            }
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    void send_publish_from_queue() {
        if (ep->status_ != close_status::open) return;
        while (!ep->publish_queue_.empty() &&
               ep->con_.get_receive_maximum_vacancy_for_send() != 0) {
            async_send(
                ep,
                force_move(ep->publish_queue_.front()),
                true, // from queue
                as::detached
            );
            ep->publish_queue_.pop_front();
        }
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
async_recv(
    this_type_sp impl,
    std::optional<filter> fil,
    std::set<control_packet_type> types,
    as::any_completion_handler<
        void(error_code, std::optional<packet_variant_type>)
    > handler
) {
    auto exe = impl->get_executor();
    as::async_compose<
        as::any_completion_handler<
            void(error_code, std::optional<packet_variant_type>)
        >,
        void(error_code, std::optional<packet_variant_type>)
    >(
        recv_op{
            force_move(impl),
            fil,
            force_move(types)
        },
        handler,
        exe
    );
}

} // namespace async_mqtt::detail

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RECV_IPP
