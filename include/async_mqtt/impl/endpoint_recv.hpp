// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_RECV_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_RECV_HPP

#include <async_mqtt/endpoint.hpp>
#include <iostream>

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
                [&](basic_event_send<PacketIdBytes>& ev) {
                    state = sent;
                    auto ep_copy{ep};
                    std::cout << __FILE__ << ":" << __LINE__ << ":" << &a_ep << std::endl;
                    async_send(
                        force_move(ep_copy),
                        force_move(ev.get()),
                        force_move(self)
                    );
                    BOOST_ASSERT(!ev.get_release_packet_id_if_send_error());
                    return false;
                },
                [&](basic_event_packet_id_released<PacketIdBytes> ev) {
                    a_ep.notify_release_pid(ev.get());
                    try_resend_from_queue = true;
                    return true;
                },
                [&](basic_event_packet_received<PacketIdBytes> ev) {
                    if (recv_packet) {
                        std::cout << __FILE__ << ":" << __LINE__ << ":" << &a_ep << std::endl;
                        // rest events would be processed the next async_recv call
                        // back the event to the recv_events_ for the next async_recv
                        a_ep.recv_events_.push_front(force_move(event));
                        state = complete;
                        as::dispatch(
                            a_ep.get_executor(),
                            force_move(self)
                        );
                        return false;
                    }
                    else {
                        std::cout << __FILE__ << ":" << __LINE__ << ":" << &a_ep << std::endl;
                        recv_packet.emplace(force_move(ev.get()));
                    }
                    return true;
                },
                [&](event_recv) {
                    // For auto read pubrel
                    state = read;
                    as::dispatch(
                        a_ep.get_executor(),
                        force_move(self)
                    );
                    return false;
                },
                [&](event_timer ev) {
                    switch (ev.get_timer_for()) {
                    case timer::pingreq_send:
                        // receive server keep alive property in connack
                        if (ev.get_op() == event_timer::op_type::reset) {
                            reset_pingreq_send_timer(ep, ev.get_ms());
                        }
                        else {
                            BOOST_ASSERT(false);
                        }
                        break;
                    case timer::pingreq_recv:
                        switch (ev.get_op()) {
                        case event_timer::op_type::set:
                            set_pingreq_recv_timer(ep, ev.get_ms());
                            break;
                        case event_timer::op_type::reset:
                            reset_pingreq_recv_timer(ep, ev.get_ms());
                            break;
                        case event_timer::op_type::cancel:
                            cancel_pingreq_recv_timer(ep);
                            break;
                        }
                        break;
                    case timer::pingresp_recv:
                        if (ev.get_op() == event_timer::op_type::cancel) {
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
            ASYNC_MQTT_LOG("mqtt_impl", error)
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
                std::cout << __FILE__ << ":" << __LINE__ << ":" << &a_ep << std::endl;
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
            a_ep.mbs_ = a_ep.read_buf_.prepare(a_ep.read_buffer_size_);
            a_ep.stream_.async_read_some(
                a_ep.mbs_,
                force_move(self)
            );
        } break;
        case protocol_read: {
            auto b = as::buffers_iterator<as::streambuf::mutable_buffers_type>::begin(a_ep.mbs_);
            auto e = std::next(b, std::ptrdiff_t(bytes_transferred));
            auto events{a_ep.con_.recv(b, e)};
            std::move(events.begin(), events.end(), std::back_inserter(a_ep.recv_events_));
            a_ep.read_buf_.commit(bytes_transferred);
            if (a_ep.recv_events_.empty()) {
                // required more bytes
                std::cout << __FILE__ << ":" << __LINE__ << ":" << &a_ep << std::endl;
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
                as::dispatch(
                    a_ep.get_executor(),
                    force_move(self)
                );
            }
        } break;
        case sent: {
            while (!a_ep.recv_events_.empty()) {
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
                        std::cout << __FILE__ << ":" << __LINE__ << ":" << &a_ep << std::endl;
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
                std::cout << __FILE__ << ":" << __LINE__ << ":" << &a_ep << std::endl;
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

    void send_publish_from_queue();
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
