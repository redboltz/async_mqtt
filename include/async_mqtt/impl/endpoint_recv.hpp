// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_RECV_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_RECV_HPP

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/impl/buffer_to_packet_variant.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint<Role, PacketIdBytes, NextLayer>::
recv_op {
    this_type& ep;
    std::optional<filter> fil = std::nullopt;
    std::set<control_packet_type> types = {};
    std::optional<system_error> decided_error = std::nullopt;
    enum { initiate, disconnect, close, read, complete } state = initiate;

    template <typename Self>
    void operator()(
        Self& self,
        error_code ec = error_code{},
        buffer buf = buffer{}
    ) {
        if (ec) {
            ASYNC_MQTT_LOG("mqtt_impl", info)
                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                << "recv error:" << ec.message();
            decided_error.emplace(ec);
            ep.recv_processing_ = false;
            state = close;
            auto& a_ep{ep};
            a_ep.async_close(
                force_move(self)
            );
            return;
        }

        switch (state) {
        case initiate: {
            state = read;
            auto& a_ep{ep};
            a_ep.stream_->async_read_packet(
                force_move(self)
            );
        } break;
        case read: {
            if (buf.size() > ep.maximum_packet_size_recv_) {
                // on v3.1.1 maximum_packet_size_recv_ is initialized as packet_size_no_limit
                BOOST_ASSERT(ep.protocol_version_ == protocol_version::v5);
                state = disconnect;
                decided_error.emplace(
                    make_error(
                        errc::bad_message,
                        "too large packet received"
                    )
                );
                auto& a_ep{ep};
                a_ep.async_send(
                    v5::disconnect_packet{
                        disconnect_reason_code::packet_too_large
                    },
                    force_move(self)
                );
                return;
            }

            auto v = buffer_to_basic_packet_variant<PacketIdBytes>(buf, ep.protocol_version_);
            ASYNC_MQTT_LOG("mqtt_impl", trace)
                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                << "recv:" << v;
            bool call_complete = true;
            v.visit(
                // do internal protocol processing
                overload {
                    [&](v3_1_1::connect_packet& p) {
                        ep.initialize();
                        ep.protocol_version_ = protocol_version::v3_1_1;
                        ep.status_ = connection_status::connecting;
                        auto keep_alive = p.keep_alive();
                        if (keep_alive != 0) {
                            ep.pingreq_recv_timeout_ms_.emplace(keep_alive * 1000 * 3 / 2);
                        }
                        if (p.clean_session()) {
                            ep.need_store_ = false;
                        }
                        else {
                            ep.need_store_ = true;
                        }
                    },
                    [&](v5::connect_packet& p) {
                        ep.initialize();
                        ep.protocol_version_ = protocol_version::v5;
                        ep.status_ = connection_status::connecting;
                        auto keep_alive = p.keep_alive();
                        if (keep_alive != 0) {
                            ep.pingreq_recv_timeout_ms_.emplace(keep_alive * 1000 * 3 / 2);
                        }
                        for (auto const& prop : p.props()) {
                            prop.visit(
                                overload {
                                    [&](property::topic_alias_maximum const& p) {
                                        if (p.val() > 0) {
                                            ep.topic_alias_send_.emplace(p.val());
                                        }
                                    },
                                    [&](property::receive_maximum const& p) {
                                        BOOST_ASSERT(p.val() != 0);
                                        ep.publish_send_max_ = p.val();
                                    },
                                    [&](property::maximum_packet_size const& p) {
                                        BOOST_ASSERT(p.val() != 0);
                                        ep.maximum_packet_size_send_ = p.val();
                                    },
                                    [&](property::session_expiry_interval const& p) {
                                        if (p.val() != 0) {
                                            ep.need_store_ = true;
                                        }
                                    },
                                    [](auto const&) {
                                    }
                                }
                            );
                        }
                    },
                    [&](v3_1_1::connack_packet& p) {
                        if (p.code() == connect_return_code::accepted) {
                            ep.status_ = connection_status::connected;
                            if (p.session_present()) {
                                ep.send_stored();
                            }
                            else {
                                ep.clear_pid_man();
                                ep.store_.clear();
                            }
                        }
                    },
                    [&](v5::connack_packet& p) {
                        if (p.code() == connect_reason_code::success) {
                            ep.status_ = connection_status::connected;
                             for (auto const& prop : p.props()) {
                                prop.visit(
                                    overload {
                                        [&](property::topic_alias_maximum const& p) {
                                            if (p.val() > 0) {
                                                ep.topic_alias_send_.emplace(p.val());
                                            }
                                        },
                                        [&](property::receive_maximum const& p) {
                                            BOOST_ASSERT(p.val() != 0);
                                            ep.publish_send_max_ = p.val();
                                        },
                                        [&](property::maximum_packet_size const& p) {
                                            BOOST_ASSERT(p.val() != 0);
                                            ep.maximum_packet_size_send_ = p.val();
                                        },
                                        [](auto const&) {
                                        }
                                    }
                                );
                            }

                            if (p.session_present()) {
                                ep.send_stored();
                            }
                            else {
                                ep.clear_pid_man();
                                ep.store_.clear();
                            }
                        }
                    },
                    [&](v3_1_1::basic_publish_packet<PacketIdBytes>& p) {
                        switch (p.opts().get_qos()) {
                        case qos::at_least_once: {
                            if (ep.auto_pub_response_ && ep.status_ == connection_status::connected) {
                                ep.async_send(
                                    v3_1_1::basic_puback_packet<PacketIdBytes>(p.packet_id()),
                                    [](system_error const&){}
                                );
                            }
                        } break;
                        case qos::exactly_once:
                            call_complete = process_qos2_publish(self, protocol_version::v3_1_1, p.packet_id());
                            break;
                        default:
                            break;
                        }
                    },
                    [&](v5::basic_publish_packet<PacketIdBytes>& p) {
                        switch (p.opts().get_qos()) {
                        case qos::at_least_once: {
                            if (ep.publish_recv_.size() == ep.publish_recv_max_) {
                                state = disconnect;
                                decided_error.emplace(
                                    make_error(
                                        errc::bad_message,
                                        "receive maximum exceeded"
                                    )
                                );
                                auto& a_ep{ep};
                                a_ep.async_send(
                                    v5::disconnect_packet{
                                        disconnect_reason_code::receive_maximum_exceeded
                                    },
                                    force_move(self)
                                );
                                return;
                            }
                            auto packet_id = p.packet_id();
                            ep.publish_recv_.insert(packet_id);
                            if (ep.auto_pub_response_ && ep.status_ == connection_status::connected) {
                                ep.async_send(
                                    v5::basic_puback_packet<PacketIdBytes>{packet_id},
                                    [](system_error const&){}
                                );
                            }
                        } break;
                        case qos::exactly_once: {
                            if (ep.publish_recv_.size() == ep.publish_recv_max_) {
                                state = disconnect;
                                decided_error.emplace(
                                    make_error(
                                        errc::bad_message,
                                        "receive maximum exceeded"
                                    )
                                );
                                auto& a_ep{ep};
                                a_ep.async_send(
                                    v5::disconnect_packet{
                                        disconnect_reason_code::receive_maximum_exceeded
                                    },
                                    force_move(self)
                                );
                                return;
                            }
                            auto packet_id = p.packet_id();
                            ep.publish_recv_.insert(packet_id);
                            call_complete = process_qos2_publish(self, protocol_version::v5, packet_id);
                        } break;
                        default:
                            break;
                        }

                        if (p.topic().empty()) {
                            if (auto ta_opt = get_topic_alias(p.props())) {
                                // extract topic from topic_alias
                                if (*ta_opt == 0 ||
                                    *ta_opt > ep.topic_alias_recv_->max()) {
                                    state = disconnect;
                                    decided_error.emplace(
                                        make_error(
                                            errc::bad_message,
                                            "topic alias invalid"
                                        )
                                    );
                                    auto& a_ep{ep};
                                    a_ep.async_send(
                                        v5::disconnect_packet{
                                            disconnect_reason_code::topic_alias_invalid
                                        },
                                        force_move(self)
                                    );
                                    return;
                                }
                                else {
                                    auto topic = ep.topic_alias_recv_->find(*ta_opt);
                                    if (topic.empty()) {
                                        ASYNC_MQTT_LOG("mqtt_impl", error)
                                            << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                            << "no matching topic alias: "
                                            << *ta_opt;
                                        state = disconnect;
                                        decided_error.emplace(
                                            make_error(
                                                errc::bad_message,
                                                "topic alias invalid"
                                            )
                                        );
                                        auto& a_ep{ep};
                                        a_ep.async_send(
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
                                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                    << "topic is empty but topic_alias isn't set";
                                state = disconnect;
                                decided_error.emplace(
                                    make_error(
                                        errc::bad_message,
                                        "topic alias invalid"
                                    )
                                );
                                auto& a_ep{ep};
                                a_ep.async_send(
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
                                    *ta_opt > ep.topic_alias_recv_->max()) {
                                    state = disconnect;
                                    decided_error.emplace(
                                        make_error(
                                            errc::bad_message,
                                            "topic alias invalid"
                                        )
                                    );
                                    auto& a_ep{ep};
                                    a_ep.async_send(
                                        v5::disconnect_packet{
                                            disconnect_reason_code::topic_alias_invalid
                                        },
                                        force_move(self)
                                    );
                                    return;
                                }
                                else {
                                    // extract topic from topic_alias
                                    if (ep.topic_alias_recv_) {
                                        ep.topic_alias_recv_->insert_or_update(p.topic(), *ta_opt);
                                    }
                                }
                            }
                        }
                    },
                    [&](v3_1_1::basic_puback_packet<PacketIdBytes>& p) {
                        auto packet_id = p.packet_id();
                        if (ep.pid_puback_.erase(packet_id)) {
                            ep.store_.erase(response_packet::v3_1_1_puback, packet_id);
                            ep.release_pid(packet_id);
                        }
                        else {
                            ASYNC_MQTT_LOG("mqtt_impl", info)
                                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                << "invalid packet_id puback received packet_id:" << packet_id;
                            state = disconnect;
                            decided_error.emplace(
                                make_error(
                                    errc::bad_message,
                                    "packet_id invalid"
                                )
                            );
                            auto& a_ep{ep};
                            a_ep.async_send(
                                v5::disconnect_packet{
                                    disconnect_reason_code::topic_alias_invalid
                                },
                                force_move(self)
                            );
                            return;
                        }
                    },
                    [&](v5::basic_puback_packet<PacketIdBytes>& p) {
                        auto packet_id = p.packet_id();
                        if (ep.pid_puback_.erase(packet_id)) {
                            ep.store_.erase(response_packet::v5_puback, packet_id);
                            ep.release_pid(packet_id);
                            --ep.publish_send_count_;
                            send_publish_from_queue();
                        }
                        else {
                            ASYNC_MQTT_LOG("mqtt_impl", info)
                                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                << "invalid packet_id puback received packet_id:" << packet_id;
                            state = disconnect;
                            decided_error.emplace(
                                make_error(
                                    errc::bad_message,
                                    "packet_id invalid"
                                )
                            );
                            auto& a_ep{ep};
                            a_ep.async_send(
                                v5::disconnect_packet{
                                    disconnect_reason_code::topic_alias_invalid
                                },
                                force_move(self)
                            );
                            return;
                        }
                    },
                    [&](v3_1_1::basic_pubrec_packet<PacketIdBytes>& p) {
                        auto packet_id = p.packet_id();
                        if (ep.pid_pubrec_.erase(packet_id)) {
                            ep.store_.erase(response_packet::v3_1_1_pubrec, packet_id);
                            if (ep.auto_pub_response_ && ep.status_ == connection_status::connected) {
                                ep.async_send(
                                    v3_1_1::basic_pubrel_packet<PacketIdBytes>(packet_id),
                                    [](system_error const&){}
                                );
                            }
                        }
                        else {
                            ASYNC_MQTT_LOG("mqtt_impl", info)
                                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                << "invalid packet_id pubrec received packet_id:" << packet_id;
                            state = disconnect;
                            decided_error.emplace(
                                make_error(
                                    errc::bad_message,
                                    "packet_id invalid"
                                )
                            );
                            auto& a_ep{ep};
                            a_ep.async_send(
                                v5::disconnect_packet{
                                    disconnect_reason_code::topic_alias_invalid
                                },
                                force_move(self)
                            );
                            return;
                        }
                    },
                    [&](v5::basic_pubrec_packet<PacketIdBytes>& p) {
                        auto packet_id = p.packet_id();
                        if (ep.pid_pubrec_.erase(packet_id)) {
                            ep.store_.erase(response_packet::v5_pubrec, packet_id);
                            if (is_error(p.code())) {
                                ep.release_pid(packet_id);
                                ep.qos2_publish_processing_.erase(packet_id);
                                --ep.publish_send_count_;
                                send_publish_from_queue();
                            }
                            else if (ep.auto_pub_response_ && ep.status_ == connection_status::connected) {
                                ep.async_send(
                                    v5::basic_pubrel_packet<PacketIdBytes>(packet_id),
                                    [](system_error const&){}
                                );
                            }
                        }
                        else {
                            ASYNC_MQTT_LOG("mqtt_impl", info)
                                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                << "invalid packet_id pubrec received packet_id:" << packet_id;
                            state = disconnect;
                            decided_error.emplace(
                                make_error(
                                    errc::bad_message,
                                    "packet_id invalid"
                                )
                            );
                            auto& a_ep{ep};
                            a_ep.async_send(
                                v5::disconnect_packet{
                                    disconnect_reason_code::topic_alias_invalid
                                },
                                force_move(self)
                            );
                            return;
                        }
                    },
                    [&](v3_1_1::basic_pubrel_packet<PacketIdBytes>& p) {
                        auto packet_id = p.packet_id();
                        ep.qos2_publish_handled_.erase(packet_id);
                        if (ep.auto_pub_response_ && ep.status_ == connection_status::connected) {
                            ep.async_send(
                                v3_1_1::basic_pubcomp_packet<PacketIdBytes>(packet_id),
                                [](system_error const&){}
                            );
                        }
                    },
                    [&](v5::basic_pubrel_packet<PacketIdBytes>& p) {
                        auto packet_id = p.packet_id();
                        ep.qos2_publish_handled_.erase(packet_id);
                        if (ep.auto_pub_response_ && ep.status_ == connection_status::connected) {
                            ep.async_send(
                                v5::basic_pubcomp_packet<PacketIdBytes>(packet_id),
                                [](system_error const&){}
                            );
                        }
                    },
                    [&](v3_1_1::basic_pubcomp_packet<PacketIdBytes>& p) {
                        auto packet_id = p.packet_id();
                        if (ep.pid_pubcomp_.erase(packet_id)) {
                            ep.store_.erase(response_packet::v3_1_1_pubcomp, packet_id);
                            ep.release_pid(packet_id);
                            ep.qos2_publish_processing_.erase(packet_id);
                            --ep.publish_send_count_;
                            send_publish_from_queue();
                        }
                        else {
                            ASYNC_MQTT_LOG("mqtt_impl", info)
                                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                << "invalid packet_id pubcomp received packet_id:" << packet_id;
                            state = disconnect;
                            decided_error.emplace(
                                make_error(
                                    errc::bad_message,
                                    "packet_id invalid"
                                )
                            );
                            auto& a_ep{ep};
                            a_ep.async_send(
                                v5::disconnect_packet{
                                    disconnect_reason_code::topic_alias_invalid
                                },
                                force_move(self)
                            );
                            return;
                        }
                    },
                    [&](v5::basic_pubcomp_packet<PacketIdBytes>& p) {
                        auto packet_id = p.packet_id();
                        if (ep.pid_pubcomp_.erase(packet_id)) {
                            ep.store_.erase(response_packet::v5_pubcomp, packet_id);
                            ep.release_pid(packet_id);
                            ep.qos2_publish_processing_.erase(packet_id);
                        }
                        else {
                            ASYNC_MQTT_LOG("mqtt_impl", info)
                                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                << "invalid packet_id pubcomp received packet_id:" << packet_id;
                            state = disconnect;
                            decided_error.emplace(
                                make_error(
                                    errc::bad_message,
                                    "packet_id invalid"
                                )
                            );
                            auto& a_ep{ep};
                            a_ep.async_send(
                                v5::disconnect_packet{
                                    disconnect_reason_code::topic_alias_invalid
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
                        if (ep.pid_suback_.erase(packet_id)) {
                            ep.release_pid(packet_id);
                        }
                    },
                    [&](v5::basic_suback_packet<PacketIdBytes>& p) {
                        auto packet_id = p.packet_id();
                        if (ep.pid_suback_.erase(packet_id)) {
                            ep.release_pid(packet_id);
                        }
                    },
                    [&](v3_1_1::basic_unsubscribe_packet<PacketIdBytes>&) {
                    },
                    [&](v5::basic_unsubscribe_packet<PacketIdBytes>&) {
                    },
                    [&](v3_1_1::basic_unsuback_packet<PacketIdBytes>& p) {
                        auto packet_id = p.packet_id();
                        if (ep.pid_unsuback_.erase(packet_id)) {
                            ep.release_pid(packet_id);
                        }
                    },
                    [&](v5::basic_unsuback_packet<PacketIdBytes>& p) {
                        auto packet_id = p.packet_id();
                        if (ep.pid_unsuback_.erase(packet_id)) {
                            ep.release_pid(packet_id);
                        }
                    },
                    [&](v3_1_1::pingreq_packet&) {
                        if constexpr(can_send_as_server(Role)) {
                            if (ep.auto_ping_response_ && ep.status_ == connection_status::connected) {
                                ep.async_send(
                                    v3_1_1::pingresp_packet(),
                                    [](system_error const&){}
                                );
                            }
                        }
                    },
                    [&](v5::pingreq_packet&) {
                        if constexpr(can_send_as_server(Role)) {
                            if (ep.auto_ping_response_ && ep.status_ == connection_status::connected) {
                                ep.async_send(
                                    v5::pingresp_packet(),
                                    [](system_error const&){}
                                );
                            }
                        }
                    },
                    [&](v3_1_1::pingresp_packet&) {
                        ep.tim_pingresp_recv_->cancel();
                    },
                    [&](v5::pingresp_packet&) {
                        ep.tim_pingresp_recv_->cancel();
                    },
                    [&](v3_1_1::disconnect_packet&) {
                        ep.status_ = connection_status::disconnecting;
                    },
                    [&](v5::disconnect_packet&) {
                        ep.status_ = connection_status::disconnecting;
                    },
                    [&](v5::auth_packet&) {
                    },
                    [&](system_error&) {
                        ep.status_ = connection_status::closed;
                    }
                }
            );
            ep.reset_pingreq_recv_timer();
            ep.recv_processing_ = false;

            auto try_to_comp =
                [&] {
                    if (call_complete && !decided_error) {
                        self.complete(force_move(v));
                    }
                };

            if (fil) {
                if (auto type_opt = v.type()) {
                    if ((*fil == filter::match  && types.find(*type_opt) == types.end()) ||
                        (*fil == filter::except && types.find(*type_opt) != types.end())
                    ) {
                        // read the next packet
                        state = initiate;
                        auto& a_ep{ep};
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
            auto& a_ep{ep};
            a_ep.async_close(
                force_move(self)
            );
        } break;
        case close: {
            BOOST_ASSERT(decided_error);
            ep.recv_processing_ = false;
            ASYNC_MQTT_LOG("mqtt_impl", info)
                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                << "recv code triggers close:" << decided_error->code().message();
            self.complete(force_move(*decided_error));
            state = complete;
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    template <typename Self>
    void operator()(
        Self& self,
        system_error const&
    ) {
        BOOST_ASSERT(state == disconnect);
        state = close;
        auto& a_ep{ep};
        a_ep.async_close(
            force_move(self)
        );
    }

    void send_publish_from_queue() {
        if (ep.status_ != connection_status::connected) return;
        while (!ep.publish_queue_.empty() &&
               ep.publish_send_count_ != ep.publish_send_max_) {
            ep.async_send(
                force_move(ep.publish_queue_.front()),
                true, // from queue
                [](system_error const&){}
            );
            ep.publish_queue_.pop_front();
        }
    }

    template <typename Self>
    bool process_qos2_publish(
        Self& self,
        protocol_version ver,
        typename basic_packet_id_type<PacketIdBytes>::type packet_id
    ) {
        bool already_handled = false;
        if (ep.qos2_publish_handled_.find(packet_id) == ep.qos2_publish_handled_.end()) {
            ep.qos2_publish_handled_.emplace(packet_id);
        }
        else {
            already_handled = true;
        }
        if (ep.status_ == connection_status::connected &&
            (ep.auto_pub_response_ ||
             already_handled) // already_handled is true only if the pubrec packet
        ) {                   // corresponding to the publish packet has already
                              // been sent as success
            switch (ver) {
            case protocol_version::v3_1_1:
                ep.async_send(
                    v3_1_1::basic_pubrec_packet<PacketIdBytes>(packet_id),
                    [](system_error const&){}
                );
                break;
            case protocol_version::v5:
                ep.async_send(
                    v5::basic_pubrec_packet<PacketIdBytes>(packet_id),
                    [](system_error const&){}
                );
                break;
            default:
                BOOST_ASSERT(false);
                break;
            }
        }
        if (already_handled) {
            // do the next read
            auto& a_ep{ep};
            a_ep.stream_->async_read_packet(
                force_move(self)
            );
            return false;
        }
        return true;
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(packet_variant_type)
)
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_recv(
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "recv";
    BOOST_ASSERT(!recv_processing_);
    recv_processing_ = true;
    return
        as::async_compose<
            CompletionToken,
            void(packet_variant_type)
        >(
            recv_op{
                *this
            },
            token
        );
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(packet_variant_type)
)
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_recv(
    std::set<control_packet_type> types,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "recv";
    BOOST_ASSERT(!recv_processing_);
    recv_processing_ = true;
    return
        as::async_compose<
            CompletionToken,
            void(packet_variant_type)
        >(
            recv_op{
                *this,
                filter::match,
                force_move(types)
            },
            token
        );
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(packet_variant_type)
)
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_recv(
    filter fil,
    std::set<control_packet_type> types,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "recv";
    BOOST_ASSERT(!recv_processing_);
    recv_processing_ = true;
    return
        as::async_compose<
            CompletionToken,
            void(packet_variant_type)
        >(
            recv_op{
                *this,
                fil,
                force_move(types)
            },
            token
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RECV_HPP
