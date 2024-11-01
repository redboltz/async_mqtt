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
    enum { dispatch, read, process } state = dispatch;

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
                    packet_variant_type{}
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
            state = process;
            a_ep.mbs_ = a_ep.read_buf_.prepare(a_ep.bulk_read_buffer_size_);
            a_ep.stream_.async_read_some(
                a_ep.mbs_,
                force_move(self)
            );
        } break;
        case process: {
            state = complete;
            auto b = as::buffer_sequence_begin(a_ep.mbs_);
            auto e = std::next(b, bytes_transferred);
            auto events = a_ep.con_.recv(b, e);
            for (auto const& event : events) {
                std::visit(
                    overload{
                        [&](error_code ec) {
                        },
                        [&](basic_event_send<PacketIdBytes> ev) {
                        },
                        [&](basic_event_packet_id_released<PacketIdBytes> ev) {
                        },
                        [&](basic_event_packet_received<PacketIdBytes> ev) {
                        },
                        [&](event_recv) {
                        },
                        [&](event_timer ev) {
                        },
                        [&](event_close) {
                        }
                    },
                    event
                );
            }
        } break;
        case read: {
            if (buf.size() > a_ep.maximum_packet_size_recv_) {
                // on v3.1.1 maximum_packet_size_recv_ is initialized as packet_size_no_limit
                BOOST_ASSERT(a_ep.protocol_version_ == protocol_version::v5);
                state = disconnect;
                decided_error.emplace(
                    make_error_code(
                        disconnect_reason_code::packet_too_large
                    )
                );
                auto ep_copy = ep;
                a_ep.async_send(
                    force_move(ep_copy),
                    v5::disconnect_packet{
                        disconnect_reason_code::packet_too_large
                    },
                    force_move(self)
                );
                return;
            }

            bool call_complete = true;
            error_code ec = error_code{};
            auto v = buffer_to_basic_packet_variant<PacketIdBytes>(buf, a_ep.protocol_version_, ec);
            auto ep_copy = ep;
            if (ec) {
                decided_error.emplace(ec);
                if (a_ep.protocol_version_ == protocol_version::v5) {
                    state = disconnect;
                    if constexpr (can_send_as_server(Role)) {
                        if (ec.category() == get_connect_reason_code_category()) {
                            a_ep.status_ = connection_status::connecting;
                            auto ep_copy = ep;
                            async_send(
                                force_move(ep_copy),
                                v5::connack_packet{
                                    false,
                                    static_cast<connect_reason_code>(ec.value())
                                },
                                force_move(self)
                            );
                        }
                        return;
                    }
                    if (ec.category() == get_disconnect_reason_code_category()) {
                        state = disconnect;
                        if (a_ep.status_ == connection_status::connected) {
                            auto ep_copy = ep;
                            a_ep.async_send(
                                force_move(ep_copy),
                                v5::disconnect_packet{
                                    static_cast<disconnect_reason_code>(ec.value())
                                },
                                force_move(self)
                            );
                            return;
                        }
                    }
                }
                state = close;
                auto ep_copy = ep;
                a_ep.async_close(
                    force_move(ep_copy),
                    force_move(self)
                );
                return;
            }
            else {
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                    << "recv:" << v;
                v.visit(
                    // do internal protocol processing
                    overload {
                        [&](v3_1_1::connect_packet& p) {
                            handle_v3_1_1_connect(p);
                        },
                        [&](v5::connect_packet& p) {
                            handle_v5_connect(p);
                        },
                        [&](v3_1_1::connack_packet& p) {
                            handle_v3_1_1_connack(p);
                        },
                        [&](v5::connack_packet& p) {
                            handle_v5_connack(p);
                        },
                        [&](v3_1_1::basic_publish_packet<PacketIdBytes>& p) {
                            switch (p.opts().get_qos()) {
                            case qos::at_least_once: {
                                if (a_ep.auto_pub_response_ &&
                                    a_ep.status_ == connection_status::connected) {
                                    a_ep.async_send(
                                        ep,
                                        v3_1_1::basic_puback_packet<PacketIdBytes>(p.packet_id()),
                                        as::detached
                                    );
                                }
                            } break;
                            case qos::exactly_once: {
                                call_complete = process_qos2_publish(protocol_version::v3_1_1, p.packet_id());
                                if (!call_complete) {
                                    // do the next read
                                    a_ep.stream_.async_read_packet(
                                        force_move(self)
                                    );
                                }
                            } break;
                            default:
                                break;
                            }
                        },
                        [&](v5::basic_publish_packet<PacketIdBytes>& p) {
                            switch (p.opts().get_qos()) {
                            case qos::at_least_once: {
                                if (a_ep.publish_recv_.size() == a_ep.publish_recv_max_) {
                                    state = disconnect;
                                    decided_error.emplace(
                                        make_error_code(
                                            disconnect_reason_code::receive_maximum_exceeded
                                        )
                                    );
                                    auto ep_copy = ep;
                                    a_ep.async_send(
                                        force_move(ep_copy),
                                        v5::disconnect_packet{
                                            disconnect_reason_code::receive_maximum_exceeded
                                        },
                                        force_move(self)
                                    );
                                    return;
                                }
                                auto packet_id = p.packet_id();
                                a_ep.publish_recv_.insert(packet_id);
                                if (a_ep.auto_pub_response_ && a_ep.status_ == connection_status::connected) {
                                    a_ep.async_send(
                                        ep,
                                        v5::basic_puback_packet<PacketIdBytes>{packet_id},
                                        as::detached
                                    );
                                }
                            } break;
                            case qos::exactly_once: {
                                if (a_ep.publish_recv_.size() == a_ep.publish_recv_max_) {
                                    state = disconnect;
                                    decided_error.emplace(
                                        make_error_code(
                                            disconnect_reason_code::receive_maximum_exceeded
                                        )
                                    );
                                    auto ep_copy = ep;
                                    a_ep.async_send(
                                        force_move(ep_copy),
                                        v5::disconnect_packet{
                                            disconnect_reason_code::receive_maximum_exceeded
                                        },
                                        force_move(self)
                                    );
                                    return;
                                }
                                auto packet_id = p.packet_id();
                                a_ep.publish_recv_.insert(packet_id);
                                call_complete = process_qos2_publish(protocol_version::v5, packet_id);
                                if (!call_complete) {
                                    // do the next read
                                    a_ep.stream_.async_read_packet(
                                        force_move(self)
                                    );
                                }
                            } break;
                            default:
                                break;
                            }

                            if (p.topic().empty()) {
                                if (auto ta_opt = get_topic_alias(p.props())) {
                                    // extract topic from topic_alias
                                    if (*ta_opt == 0 ||
                                        !a_ep.topic_alias_recv_ || // topic_alias_maximum is 0
                                        *ta_opt > a_ep.topic_alias_recv_->max()) {
                                        state = disconnect;
                                        decided_error.emplace(
                                            make_error_code(
                                                disconnect_reason_code::topic_alias_invalid
                                            )
                                        );
                                        auto ep_copy = ep;
                                        a_ep.async_send(
                                            force_move(ep_copy),
                                            v5::disconnect_packet{
                                                disconnect_reason_code::topic_alias_invalid
                                            },
                                            force_move(self)
                                        );
                                        return;
                                    }
                                    else {
                                        BOOST_ASSERT(a_ep.topic_alias_recv_);
                                        auto topic = a_ep.topic_alias_recv_->find(*ta_opt);
                                        if (topic.empty()) {
                                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                                << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                                                << "no matching topic alias: "
                                                << *ta_opt;
                                            state = disconnect;
                                            decided_error.emplace(
                                                make_error_code(
                                                    disconnect_reason_code::topic_alias_invalid
                                                )
                                            );
                                            auto ep_copy = ep;
                                            a_ep.async_send(
                                                force_move(ep_copy),
                                                v5::disconnect_packet{
                                                    disconnect_reason_code::topic_alias_invalid
                                                },
                                                force_move(self)
                                            );
                                            return;
                                        }
                                        else {
                                            p.add_topic(force_move(topic));
                                        }
                                    }
                                }
                                else {
                                    ASYNC_MQTT_LOG("mqtt_impl", error)
                                        << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                                        << "topic is empty but topic_alias isn't set";
                                    state = disconnect;
                                    decided_error.emplace(
                                        make_error_code(
                                            disconnect_reason_code::topic_alias_invalid
                                        )
                                    );
                                    auto ep_copy = ep;
                                    a_ep.async_send(
                                        force_move(ep_copy),
                                        v5::disconnect_packet{
                                            disconnect_reason_code::topic_alias_invalid
                                        },
                                        force_move(self)
                                    );
                                    return;
                                }
                            }
                            else {
                                if (auto ta_opt = get_topic_alias(p.props())) {
                                    if (*ta_opt == 0 ||
                                        !a_ep.topic_alias_recv_ || // topic_alias_maximum is 0
                                        *ta_opt > a_ep.topic_alias_recv_->max()) {
                                        state = disconnect;
                                        decided_error.emplace(
                                            make_error_code(
                                                disconnect_reason_code::topic_alias_invalid
                                            )
                                        );
                                        auto ep_copy = ep;
                                        a_ep.async_send(
                                            force_move(ep_copy),
                                            v5::disconnect_packet{
                                                disconnect_reason_code::topic_alias_invalid
                                            },
                                            force_move(self)
                                        );
                                        return;
                                    }
                                    else {
                                        BOOST_ASSERT(a_ep.topic_alias_recv_);
                                        // extract topic from topic_alias
                                        a_ep.topic_alias_recv_->insert_or_update(p.topic(), *ta_opt);
                                    }
                                }
                            }
                        },
                        [&](v3_1_1::basic_puback_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            if (a_ep.pid_puback_.erase(packet_id)) {
                                a_ep.store_.erase(response_packet::v3_1_1_puback, packet_id);
                                a_ep.release_pid(packet_id);
                            }
                            else {
                                ASYNC_MQTT_LOG("mqtt_impl", info)
                                    << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                                    << "invalid packet_id puback received packet_id:" << packet_id;
                                state = disconnect;
                                decided_error.emplace(
                                    make_error_code(
                                        disconnect_reason_code::protocol_error
                                    )
                                );
                                as::dispatch(
                                    a_ep.get_executor(),
                                    force_move(self)
                                );
                                return;
                            }
                        },
                        [&](v5::basic_puback_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            if (a_ep.pid_puback_.erase(packet_id)) {
                                a_ep.store_.erase(response_packet::v5_puback, packet_id);
                                a_ep.release_pid(packet_id);
                                --a_ep.publish_send_count_;
                                send_publish_from_queue();
                            }
                            else {
                                ASYNC_MQTT_LOG("mqtt_impl", info)
                                    << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                                    << "invalid packet_id puback received packet_id:" << packet_id;
                                state = disconnect;
                                decided_error.emplace(
                                    make_error_code(
                                        disconnect_reason_code::protocol_error
                                    )
                                );
                                auto ep_copy = ep;
                                a_ep.async_send(
                                    force_move(ep_copy),
                                    v5::disconnect_packet{
                                        disconnect_reason_code::protocol_error
                                    },
                                    force_move(self)
                                );
                                return;
                            }
                        },
                        [&](v3_1_1::basic_pubrec_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            if (a_ep.pid_pubrec_.erase(packet_id)) {
                                a_ep.store_.erase(response_packet::v3_1_1_pubrec, packet_id);
                                if (a_ep.auto_pub_response_ && a_ep.status_ == connection_status::connected) {
                                    a_ep.async_send(
                                        ep,
                                        v3_1_1::basic_pubrel_packet<PacketIdBytes>(packet_id),
                                        as::detached
                                    );
                                }
                            }
                            else {
                                ASYNC_MQTT_LOG("mqtt_impl", info)
                                    << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                                    << "invalid packet_id pubrec received packet_id:" << packet_id;
                                state = disconnect;
                                decided_error.emplace(
                                    make_error_code(
                                        disconnect_reason_code::protocol_error
                                    )
                                );
                                as::dispatch(
                                    a_ep.get_executor(),
                                    force_move(self)
                                );
                                return;
                            }
                        },
                        [&](v5::basic_pubrec_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            if (a_ep.pid_pubrec_.erase(packet_id)) {
                                a_ep.store_.erase(response_packet::v5_pubrec, packet_id);
                                if (make_error_code(p.code())) {
                                    a_ep.release_pid(packet_id);
                                    a_ep.qos2_publish_processing_.erase(packet_id);
                                    --a_ep.publish_send_count_;
                                    send_publish_from_queue();
                                }
                                else if (a_ep.auto_pub_response_ && a_ep.status_ == connection_status::connected) {
                                    a_ep.async_send(
                                        ep,
                                        v5::basic_pubrel_packet<PacketIdBytes>(packet_id),
                                        as::detached
                                    );
                                }
                            }
                            else {
                                ASYNC_MQTT_LOG("mqtt_impl", info)
                                    << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                                    << "invalid packet_id pubrec received packet_id:" << packet_id;
                                state = disconnect;
                                decided_error.emplace(
                                    make_error_code(
                                        disconnect_reason_code::protocol_error
                                    )
                                );
                                auto ep_copy = ep;
                                a_ep.async_send(
                                    force_move(ep_copy),
                                    v5::disconnect_packet{
                                        disconnect_reason_code::protocol_error
                                    },
                                    force_move(self)
                                );
                                return;
                            }
                        },
                        [&](v3_1_1::basic_pubrel_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            a_ep.qos2_publish_handled_.erase(packet_id);
                            if (a_ep.auto_pub_response_ && a_ep.status_ == connection_status::connected) {
                                a_ep.async_send(
                                    ep,
                                    v3_1_1::basic_pubcomp_packet<PacketIdBytes>(packet_id),
                                    as::detached
                                );
                            }
                        },
                        [&](v5::basic_pubrel_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            a_ep.qos2_publish_handled_.erase(packet_id);
                            if (a_ep.auto_pub_response_ && a_ep.status_ == connection_status::connected) {
                                a_ep.async_send(
                                    ep,
                                    v5::basic_pubcomp_packet<PacketIdBytes>(packet_id),
                                    as::detached
                                );
                            }
                        },
                        [&](v3_1_1::basic_pubcomp_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            if (a_ep.pid_pubcomp_.erase(packet_id)) {
                                a_ep.store_.erase(response_packet::v3_1_1_pubcomp, packet_id);
                                a_ep.release_pid(packet_id);
                                a_ep.qos2_publish_processing_.erase(packet_id);
                            }
                            else {
                                ASYNC_MQTT_LOG("mqtt_impl", info)
                                    << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                                    << "invalid packet_id pubcomp received packet_id:" << packet_id;
                                state = disconnect;
                                decided_error.emplace(
                                    make_error_code(
                                        disconnect_reason_code::protocol_error
                                    )
                                );
                                as::dispatch(
                                    a_ep.get_executor(),
                                    force_move(self)
                                );
                                return;
                            }
                        },
                        [&](v5::basic_pubcomp_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            if (a_ep.pid_pubcomp_.erase(packet_id)) {
                                a_ep.store_.erase(response_packet::v5_pubcomp, packet_id);
                                a_ep.release_pid(packet_id);
                                a_ep.qos2_publish_processing_.erase(packet_id);
                                --a_ep.publish_send_count_;
                                send_publish_from_queue();
                            }
                            else {
                                ASYNC_MQTT_LOG("mqtt_impl", info)
                                    << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                                    << "invalid packet_id pubcomp received packet_id:" << packet_id;
                                state = disconnect;
                                decided_error.emplace(
                                    make_error_code(
                                        disconnect_reason_code::protocol_error
                                    )
                                );
                                auto ep_copy = ep;
                                a_ep.async_send(
                                    force_move(ep_copy),
                                    v5::disconnect_packet{
                                        disconnect_reason_code::protocol_error
                                    },
                                    force_move(self)
                                );
                                return;
                            }
                        },
                        [&](v3_1_1::basic_subscribe_packet<PacketIdBytes>&) {
                        },
                        [&](v5::basic_subscribe_packet<PacketIdBytes>&) {
                        },
                        [&](v3_1_1::basic_suback_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            if (a_ep.pid_suback_.erase(packet_id)) {
                                a_ep.release_pid(packet_id);
                            }
                        },
                        [&](v5::basic_suback_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            if (a_ep.pid_suback_.erase(packet_id)) {
                                a_ep.release_pid(packet_id);
                            }
                        },
                        [&](v3_1_1::basic_unsubscribe_packet<PacketIdBytes>&) {
                        },
                        [&](v5::basic_unsubscribe_packet<PacketIdBytes>&) {
                        },
                        [&](v3_1_1::basic_unsuback_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            if (a_ep.pid_unsuback_.erase(packet_id)) {
                                a_ep.release_pid(packet_id);
                            }
                        },
                        [&](v5::basic_unsuback_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            if (a_ep.pid_unsuback_.erase(packet_id)) {
                                a_ep.release_pid(packet_id);
                            }
                        },
                        [&](v3_1_1::pingreq_packet&) {
                            if constexpr(can_send_as_server(Role)) {
                                if (a_ep.auto_ping_response_ &&
                                    a_ep.status_ == connection_status::connected) {
                                    a_ep.async_send(
                                        ep,
                                        v3_1_1::pingresp_packet(),
                                        as::detached
                                    );
                                }
                            }
                        },
                        [&](v5::pingreq_packet&) {
                            if constexpr(can_send_as_server(Role)) {
                                if (a_ep.auto_ping_response_ &&
                                    a_ep.status_ == connection_status::connected) {
                                    a_ep.async_send(
                                        ep,
                                        v5::pingresp_packet(),
                                        as::detached
                                    );
                                }
                            }
                        },
                        [&](v3_1_1::pingresp_packet&) {
                            a_ep.tim_pingresp_recv_.cancel();
                        },
                        [&](v5::pingresp_packet&) {
                            a_ep.tim_pingresp_recv_.cancel();
                        },
                        [&](v3_1_1::disconnect_packet&) {
                            a_ep.status_ = connection_status::disconnecting;
                        },
                        [&](v5::disconnect_packet&) {
                            a_ep.status_ = connection_status::disconnecting;
                        },
                        [&](v5::auth_packet&) {
                        },
                        [&](std::monostate&) {
                        }
                    }
                );
            }
            reset_pingreq_recv_timer(force_move(ep_copy));

            auto try_to_comp =
                [&] {
                    if (call_complete && !decided_error) {
                        self.complete(
                            error_code{},
                            force_move(v)
                        );
                    }
                };

            if (fil) {
                if (auto type_opt = v.type()) {
                    if ((*fil == filter::match  && types.find(*type_opt) == types.end()) ||
                        (*fil == filter::except && types.find(*type_opt) != types.end())
                    ) {
                        // read the next packet
                        state = initiate;
                        as::dispatch(
                            a_ep.get_executor(),
                            force_move(self)
                        );
                    }
                    else {
                        try_to_comp();
                    }
                }
                else {
                    try_to_comp();
                }
            }
            else {
                try_to_comp();
            }
        } break;
        case disconnect: {
            state = close;
            auto ep_copy = ep;
            a_ep.async_close(
                force_move(ep_copy),
                force_move(self)
            );
        } break;
        case close: {
            BOOST_ASSERT(decided_error);
            ASYNC_MQTT_LOG("mqtt_impl", info)
                << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                << "recv code triggers close:" << decided_error->message();
            self.complete(force_move(*decided_error), packet_variant_type{});
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
            void(error_code, packet_variant_type)
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
            void(error_code, packet_variant_type)
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
            void(error_code, packet_variant_type)
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
