// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_RECV_IPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_RECV_IPP

#include <async_mqtt/asio_bind/endpoint.hpp>
#include <async_mqtt/asio_bind/impl/endpoint_impl.hpp>
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
    bool disconnect_sent_just_before = false;
    enum { dispatch, check_buf, read_istream, process, read, finish_read, sent, closed, complete } state = dispatch;

    template <typename Self>
    bool process_one_event(Self& self) {
        auto& a_ep{*ep};
        auto event{force_move(a_ep.recv_events_.front())};
        a_ep.recv_events_.pop_front();
        return std::visit(
            overload{
                [&](error_code ec) {
                    decided_error.emplace(ec);
                    return true;
                },
                [&](async_mqtt::event::basic_send<PacketIdBytes>&& ev) {
                    auto ep_copy{ep};
                    state = sent;
                    BOOST_ASSERT(!ev.get_release_packet_id_if_send_error());
                    disconnect_sent_just_before = ev.get().type() == control_packet_type::disconnect;
                    a_ep.stream_.async_write_packet(
                        force_move(ev.get()),
                        force_move(self)
                    );
                    return false;
                },
                [&](async_mqtt::event::basic_packet_id_released<PacketIdBytes>&& ev) {
                    a_ep.notify_release_pid(ev.get());
                    try_resend_from_queue = true;
                    return true;
                },
                [&](async_mqtt::event::basic_packet_received<PacketIdBytes>&& ev) {
                    BOOST_ASSERT(!recv_packet);
                    recv_packet.emplace(force_move(ev.get()));
                    return true;
                },
                [&](async_mqtt::event::timer&& ev) {
                    switch (ev.get_kind()) {
                    case timer_kind::pingreq_send:
                        switch (ev.get_op()) {
                        case timer_op::reset:
                            // receive server keep alive property in connack
                            reset_pingreq_send_timer(ep, ev.get_ms());
                            break;
                        case timer_op::cancel:
                            cancel_pingreq_send_timer(ep);
                            break;
                        default:
                            BOOST_ASSERT(false);
                            break;
                        }
                        break;
                    case timer_kind::pingreq_recv:
                        switch (ev.get_op()) {
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
                            cancel_pingresp_recv_timer(ep);
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
                    auto ep_copy{ep};
                    state = closed;
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
            force_move(event)
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
            state = check_buf;
            as::dispatch(
                a_ep.get_executor(),
                force_move(self)
            );
        } break;
        case check_buf: {
            if (a_ep.read_buf_.size() == 0) {
                // read required
                state = read;
                as::dispatch(
                    a_ep.get_executor(),
                    force_move(self)
                );
            }
            else {
                state = read_istream;
                as::post(
                    a_ep.get_executor(),
                    force_move(self)
                );
            }
        } break;
        case read_istream: {
            // at most one packet is received except QoS2 already received packet
            auto events{a_ep.con_.recv(a_ep.is_)};
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
                state = process;
                as::dispatch(
                    a_ep.get_executor(),
                    force_move(self)
                );
            }
        } break;
        case process: {
            // The last event of events_ must be event_received or error
            bool sent_or_closed = false;
            while (!a_ep.recv_events_.empty()) {
                if (!process_one_event(self)) {
                    sent_or_closed = true;
                    break;
                }
            }
            if (!sent_or_closed) {
                // exit the above loop by break;
                if (!decided_error && !recv_packet) {
                    // QoS2 already received
                    state = check_buf;
                    as::dispatch(
                        a_ep.get_executor(),
                        force_move(self)
                    );
                }
                else {
                    // either packet_received or error
                    BOOST_ASSERT(
                        (decided_error && !recv_packet) ||
                        (!decided_error && recv_packet)
                    );
                    state = complete; // all events processed
                    as::dispatch(
                        a_ep.get_executor(),
                        force_move(self)
                    );
                }
            }
        } break;
        case read: {
            state = finish_read;
            a_ep.stream_.async_read_some(
                a_ep.read_buf_.prepare(a_ep.read_buffer_size_),
                force_move(self)
            );
        } break;
        case finish_read: {
            a_ep.read_buf_.commit(bytes_transferred);
            state = read_istream;
            as::dispatch(
                a_ep.get_executor(),
                force_move(self)
            );
        } break;
        case sent: {
            if (ec) {
                ASYNC_MQTT_LOG("mqtt_impl", info)
                    << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                    << "send (triggered by recv) error:" << ec.message();
            }
            state = process;
            as::dispatch(
                a_ep.get_executor(),
                force_move(self)
            );
        } break;
        case closed: {
            state = process;
            as::dispatch(
                a_ep.get_executor(),
                force_move(self)
            );
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
                        state = check_buf;
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
        auto vacancy_opt = ep->con_.get_receive_maximum_vacancy_for_send();
        while (
            !ep->publish_queue_.empty() && // has elements
            (
                !vacancy_opt ||    // no limit
                *vacancy_opt != 0  // has vacancy
            )
        ) {
            async_send(
                ep,
                force_move(ep->publish_queue_.front()),
                true, // from queue
                as::detached
            );
            ep->publish_queue_.pop_front();
            vacancy_opt = ep->con_.get_receive_maximum_vacancy_for_send();
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

#include <async_mqtt/asio_bind/impl/endpoint_instantiate.hpp>

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RECV_IPP
