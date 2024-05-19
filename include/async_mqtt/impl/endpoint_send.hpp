// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_SEND_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_SEND_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename Packet>
struct basic_endpoint<Role, PacketIdBytes, NextLayer>::
send_op {
    this_type& ep;
    Packet packet;
    bool from_queue = false;
    enum { write, complete } state = write;

    template <typename Self>
    void operator()(
        Self& self,
        error_code ec = error_code{},
        std::size_t /*bytes_transferred*/ = 0
    ) {
        if (ec) {
            ASYNC_MQTT_LOG("mqtt_impl", info)
                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                << "send error:" << ec.message();
            self.complete(ec);
            return;
        }

        switch (state) {
        case write: {
            state = complete;
            if constexpr(
                std::is_same_v<std::decay_t<Packet>, basic_packet_variant<PacketIdBytes>> ||
                std::is_same_v<std::decay_t<Packet>, basic_store_packet_variant<PacketIdBytes>>
            ) {
                packet.visit(
                    overload {
                        [&](auto actual_packet) {
                            if (process_send_packet(self, actual_packet)) {
                                auto& a_ep{ep};
                                a_ep.stream_->async_write_packet(
                                    actual_packet,
                                        force_move(self)
                                );
                                if constexpr(is_connack<std::remove_reference_t<decltype(actual_packet)>>()) {
                                    // server send stored packets after connack sent
                                    a_ep.send_stored();
                                }
                                if constexpr(Role == role::client) {
                                    a_ep.reset_pingreq_send_timer();
                                }
                            }
                        },
                        [&](system_error&) {}
                    }
                );
            }
            else {
                if (process_send_packet(self, packet)) {
                    auto& a_ep{ep};
                    auto a_packet{packet};
                    a_ep.stream_->async_write_packet(
                        force_move(a_packet),
                        force_move(self)
                    );
                    if constexpr(is_connack<Packet>()) {
                        // server send stored packets after connack sent
                        a_ep.send_stored();
                    }
                    if constexpr(Role == role::client) {
                        a_ep.reset_pingreq_send_timer();
                    }
                }
            }
        } break;
        case complete:
            self.complete(ec);
            break;
        }
    }

    template <typename Self, typename ActualPacket>
    bool process_send_packet(Self& self, ActualPacket& actual_packet) {
        // MQTT protocol sendable packet check
        if (
            !(
                (can_send_as_client(Role) && is_client_sendable<std::decay_t<ActualPacket>>()) ||
                (can_send_as_server(Role) && is_server_sendable<std::decay_t<ActualPacket>>())
            )
        ) {
            self.complete(
                make_error(
                    errc::protocol_error,
                    "packet cannot be send by MQTT protocol"
                )
            );
            return false;
        }

        auto version_check =
            [&] {
                if (ep.protocol_version_ == protocol_version::v3_1_1 && is_v3_1_1<ActualPacket>()) {
                    return true;
                }
                if (ep.protocol_version_ == protocol_version::v5 && is_v5<ActualPacket>()) {
                    return true;
                }
                return false;
            };

        // connection status check
        if constexpr(is_connect<ActualPacket>()) {
            if (ep.status_ != connection_status::closed) {
                self.complete(
                    make_error(
                        errc::protocol_error,
                        "connect_packet can only be send on connection_status::closed"
                    )
                );
                return false;
            }
            if (!version_check()) {
                self.complete(
                    make_error(
                        errc::protocol_error,
                        "protocol version mismatch"
                    )
                );
                return false;
            }
        }
        else if constexpr(is_connack<ActualPacket>()) {
            if (ep.status_ != connection_status::connecting) {
                self.complete(
                    make_error(
                        errc::protocol_error,
                        "connack_packet can only be send on connection_status::connecting"
                    )
                );
                return false;
            }
            if (!version_check()) {
                self.complete(
                    make_error(
                        errc::protocol_error,
                        "protocol version mismatch"
                    )
                );
                return false;
            }
        }
        else if constexpr(std::is_same_v<v5::auth_packet, Packet>) {
            if (ep.status_ != connection_status::connected &&
                ep.status_ != connection_status::connecting) {
                self.complete(
                    make_error(
                        errc::protocol_error,
                        "auth packet can only be send on connection_status::connecting or status::connected"
                    )
                );
                return false;
            }
            if (!version_check()) {
                self.complete(
                    make_error(
                        errc::protocol_error,
                        "protocol version mismatch"
                    )
                );
                return false;
            }
        }
        else {
            if (ep.status_ != connection_status::connected) {
                if constexpr(!is_publish<std::decay_t<ActualPacket>>()) {
                    self.complete(
                        make_error(
                            errc::protocol_error,
                            "packet can only be send on connection_status::connected"
                        )
                    );
                    return false;
                }
            }
            if (!version_check()) {
                self.complete(
                    make_error(
                        errc::protocol_error,
                        "protocol version mismatch"
                    )
                );
                return false;
            }
        }

        // sending process
        bool topic_alias_validated = false;

        if constexpr(std::is_same_v<v3_1_1::connect_packet, std::decay_t<ActualPacket>>) {
            ep.initialize();
            ep.status_ = connection_status::connecting;
            auto keep_alive = actual_packet.keep_alive();
            if (keep_alive != 0 && !ep.pingreq_send_interval_ms_) {
                ep.pingreq_send_interval_ms_.emplace(keep_alive * 1000);
            }
            if (actual_packet.clean_session()) {
                ep.clear_pid_man();
                ep.store_.clear();
                ep.need_store_ = false;
            }
            else {
                ep.need_store_ = true;
            }
            ep.topic_alias_send_ = std::nullopt;
        }

        if constexpr(std::is_same_v<v5::connect_packet, std::decay_t<ActualPacket>>) {
            ep.initialize();
            ep.status_ = connection_status::connecting;
            auto keep_alive = actual_packet.keep_alive();
            if (keep_alive != 0 && !ep.pingreq_send_interval_ms_) {
                ep.pingreq_send_interval_ms_.emplace(keep_alive * 1000);
            }
            if (actual_packet.clean_start()) {
                ep.clear_pid_man();
                ep.store_.clear();
            }
            for (auto const& prop : actual_packet.props()) {
                prop.visit(
                    overload {
                        [&](property::topic_alias_maximum const& p) {
                            if (p.val() != 0) {
                                ep.topic_alias_recv_.emplace(p.val());
                            }
                        },
                        [&](property::receive_maximum const& p) {
                            BOOST_ASSERT(p.val() != 0);
                            ep.publish_recv_max_ = p.val();
                        },
                        [&](property::maximum_packet_size const& p) {
                            BOOST_ASSERT(p.val() != 0);
                            ep.maximum_packet_size_recv_ = p.val();
                        },
                        [&](property::session_expiry_interval const& p) {
                            if (p.val() != 0) {
                                ep.need_store_ = true;
                            }
                        },
                        [](auto const&){}
                    }
                );
            }
        }

        if constexpr(std::is_same_v<v3_1_1::connack_packet, std::decay_t<ActualPacket>>) {
            if (actual_packet.code() == connect_return_code::accepted) {
                ep.status_ = connection_status::connected;
            }
            else {
                ep.status_ = connection_status::disconnecting;
            }
        }

        if constexpr(std::is_same_v<v5::connack_packet, std::decay_t<ActualPacket>>) {
            if (actual_packet.code() == connect_reason_code::success) {
                ep.status_ = connection_status::connected;
                for (auto const& prop : actual_packet.props()) {
                    prop.visit(
                        overload {
                            [&](property::topic_alias_maximum const& p) {
                                if (p.val() != 0) {
                                    ep.topic_alias_recv_.emplace(p.val());
                                }
                            },
                            [&](property::receive_maximum const& p) {
                                BOOST_ASSERT(p.val() != 0);
                                ep.publish_recv_max_ = p.val();
                            },
                            [&](property::maximum_packet_size const& p) {
                                BOOST_ASSERT(p.val() != 0);
                                ep.maximum_packet_size_recv_ = p.val();
                            },
                            [](auto const&){}
                        }
                    );
                }
            }
            else {
                ep.status_ = connection_status::disconnecting;
            }
        }

        // store publish/pubrel packet
        if constexpr(is_publish<std::decay_t<ActualPacket>>()) {
            if (actual_packet.opts().get_qos() == qos::at_least_once ||
                actual_packet.opts().get_qos() == qos::exactly_once
            ) {
                BOOST_ASSERT(ep.pid_man_.is_used_id(actual_packet.packet_id()));
                if (ep.need_store_) {
                    if constexpr(is_instance_of<v5::basic_publish_packet, std::decay_t<ActualPacket>>::value) {
                        auto ta_opt = get_topic_alias(actual_packet.props());
                        if (actual_packet.topic().empty()) {
                            auto topic_opt = validate_topic_alias(self, ta_opt);
                            if (!topic_opt) {
                                auto packet_id = actual_packet.packet_id();
                                if (packet_id != 0) {
                                    ep.release_pid(packet_id);
                                }
                                return false;
                            }
                            topic_alias_validated = true;
                            auto props = actual_packet.props();
                            auto it = props.cbegin();
                            auto end = props.cend();
                            for (; it != end; ++it) {
                                if (it->id() == property::id::topic_alias) {
                                    props.erase(it);
                                    break;
                                }
                            }

                            auto store_packet =
                                ActualPacket(
                                    actual_packet.packet_id(),
                                    force_move(*topic_opt),
                                    actual_packet.payload_as_buffer(),
                                    actual_packet.opts(),
                                    force_move(props)
                                );
                            if (!validate_maximum_packet_size(self, store_packet)) {
                                auto packet_id = actual_packet.packet_id();
                                if (packet_id != 0) {
                                    ep.release_pid(packet_id);
                                }
                                return false;
                            }
                            // add new packet that doesn't have topic_aliass to store
                            // the original packet still use topic alias to send
                            store_packet.set_dup(true);
                            ep.store_.add(force_move(store_packet));
                        }
                        else {
                            auto props = actual_packet.props();
                            auto it = props.cbegin();
                            auto end = props.cend();
                            for (; it != end; ++it) {
                                if (it->id() == property::id::topic_alias) {
                                    props.erase(it);
                                    break;
                                }
                            }

                            auto store_packet =
                                ActualPacket(
                                    actual_packet.packet_id(),
                                    actual_packet.topic(),
                                    actual_packet.payload_as_buffer(),
                                    actual_packet.opts(),
                                    force_move(props)
                                );
                            if (!validate_maximum_packet_size(self, store_packet)) {
                                auto packet_id = actual_packet.packet_id();
                                if (packet_id != 0) {
                                    ep.release_pid(packet_id);
                                }
                                return false;
                            }
                            store_packet.set_dup(true);
                            ep.store_.add(force_move(store_packet));
                        }
                    }
                    else {
                        if (!validate_maximum_packet_size(self, actual_packet)) {
                            auto packet_id = actual_packet.packet_id();
                            if (packet_id != 0) {
                                ep.release_pid(packet_id);
                            }
                            return false;
                        }
                        auto store_packet{actual_packet};
                        store_packet.set_dup(true);
                        ep.store_.add(force_move(store_packet));
                    }
                }
                if (actual_packet.opts().get_qos() == qos::exactly_once) {
                    ep.qos2_publish_processing_.insert(actual_packet.packet_id());
                    ep.pid_pubrec_.insert(actual_packet.packet_id());
                }
                else {
                    ep.pid_puback_.insert(actual_packet.packet_id());
                }
            }
        }

        if constexpr(is_instance_of<v5::basic_publish_packet, std::decay_t<ActualPacket>>::value) {
            // apply topic_alias
            auto ta_opt = get_topic_alias(actual_packet.props());
            if (actual_packet.topic().empty()) {
                if (!topic_alias_validated &&
                    !validate_topic_alias(self, ta_opt)) {
                    auto packet_id = actual_packet.packet_id();
                    if (packet_id != 0) {
                        ep.release_pid(packet_id);
                    }
                    return false;
                }
                // use topic_alias set by user
            }
            else {
                if (ta_opt) {
                    if (validate_topic_alias_range(self, *ta_opt)) {
                        ASYNC_MQTT_LOG("mqtt_impl", trace)
                            << ASYNC_MQTT_ADD_VALUE(address, &ep)
                            << "topia alias : "
                            << actual_packet.topic() << " - " << *ta_opt
                            << " is registered." ;
                        ep.topic_alias_send_->insert_or_update(actual_packet.topic(), *ta_opt);
                    }
                    else {
                        auto packet_id = actual_packet.packet_id();
                        if (packet_id != 0) {
                            ep.release_pid(packet_id);
                        }
                        return false;
                    }
                }
                else if (ep.auto_map_topic_alias_send_) {
                    if (ep.topic_alias_send_) {
                        if (auto ta_opt = ep.topic_alias_send_->find(actual_packet.topic())) {
                            ASYNC_MQTT_LOG("mqtt_impl", trace)
                                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                << "topia alias : " << actual_packet.topic() << " - " << *ta_opt
                                << " is found." ;
                            actual_packet.remove_topic_add_topic_alias(*ta_opt);
                        }
                        else {
                            auto lru_ta = ep.topic_alias_send_->get_lru_alias();
                            ep.topic_alias_send_->insert_or_update(actual_packet.topic(), lru_ta); // remap topic alias
                            actual_packet.add_topic_alias(lru_ta);
                        }
                    }
                }
                else if (ep.auto_replace_topic_alias_send_) {
                    if (ep.topic_alias_send_) {
                        if (auto ta_opt = ep.topic_alias_send_->find(actual_packet.topic())) {
                            ASYNC_MQTT_LOG("mqtt_impl", trace)
                                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                << "topia alias : " << actual_packet.topic() << " - " << *ta_opt
                                << " is found." ;
                            actual_packet.remove_topic_add_topic_alias(*ta_opt);
                        }
                    }
                }
            }

            // receive_maximum for sending
            if (!from_queue && ep.enqueue_publish(actual_packet)) {
                self.complete(
                    make_error(
                        errc::success,
                        "publish_packet is enqueued due to receive_maximum for sending"
                    )
                );
                return false;
            }
        }

        if constexpr(is_instance_of<v5::basic_puback_packet, std::decay_t<ActualPacket>>::value) {
            ep.publish_recv_.erase(actual_packet.packet_id());
        }

        if constexpr(is_instance_of<v5::basic_pubrec_packet, std::decay_t<ActualPacket>>::value) {
            if (is_error(actual_packet.code())) {
                ep.publish_recv_.erase(actual_packet.packet_id());
                ep.qos2_publish_handled_.erase(actual_packet.packet_id());
            }
        }

        if constexpr(is_pubrel<std::decay_t<ActualPacket>>()) {
            BOOST_ASSERT(ep.pid_man_.is_used_id(actual_packet.packet_id()));
            if (ep.need_store_) ep.store_.add(actual_packet);
            ep.pid_pubcomp_.insert(actual_packet.packet_id());
        }

        if constexpr(is_instance_of<v5::basic_pubcomp_packet, std::decay_t<ActualPacket>>::value) {
            ep.publish_recv_.erase(actual_packet.packet_id());
        }

        if constexpr(is_subscribe<std::decay_t<ActualPacket>>()) {
            BOOST_ASSERT(ep.pid_man_.is_used_id(actual_packet.packet_id()));
            ep.pid_suback_.insert(actual_packet.packet_id());
        }

        if constexpr(is_unsubscribe<std::decay_t<ActualPacket>>()) {
            BOOST_ASSERT(ep.pid_man_.is_used_id(actual_packet.packet_id()));
            ep.pid_unsuback_.insert(actual_packet.packet_id());
        }

        if constexpr(is_pingreq<std::decay_t<ActualPacket>>()) {
            ep.reset_pingresp_recv_timer();
        }

        if constexpr(is_disconnect<std::decay_t<ActualPacket>>()) {
            ep.status_ = connection_status::disconnecting;
        }

        if (!validate_maximum_packet_size(self, actual_packet)) {
            if constexpr(own_packet_id<std::decay_t<ActualPacket>>()) {
                auto packet_id = actual_packet.packet_id();
                if (packet_id != 0) {
                    ep.release_pid(packet_id);
                }
            }
            return false;
        }

        if constexpr(is_publish<std::decay_t<ActualPacket>>()) {
            if (ep.status_ != connection_status::connected) {
                // offline publish
                self.complete(
                    make_error(
                        errc::success,
                        "packet is stored but not sent"
                    )
                );
                return false;
            }
        }
        return true;
    }

    template <typename Self>
    bool validate_topic_alias_range(Self& self, topic_alias_type ta) {
        if (!ep.topic_alias_send_) {
            self.complete(
                make_error(
                    errc::bad_message,
                    "topic_alias is set but topic_alias_maximum is 0"
                )
            );
            return false;
        }
        if (ta == 0 || ta > ep.topic_alias_send_->max()) {
            self.complete(
                make_error(
                    errc::bad_message,
                    "topic_alias is set but out of range"
                )
            );
            return false;
        }
        return true;
    }

    template <typename Self>
    std::optional<std::string> validate_topic_alias(Self& self, std::optional<topic_alias_type> ta_opt) {
        if (!ta_opt) {
            self.complete(
                make_error(
                    errc::bad_message,
                    "topic is empty but topic_alias isn't set"
                )
            );
            return std::nullopt;
        }

        if (!validate_topic_alias_range(self, *ta_opt)) {
            return std::nullopt;
        }

        auto topic = ep.topic_alias_send_->find(*ta_opt);
        if (topic.empty()) {
            self.complete(
                make_error(
                    errc::bad_message,
                    "topic is empty but topic_alias is not registered"
                )
            );
            return std::nullopt;
        }
        return topic;
    }

    template <typename Self, typename PacketArg>
    bool validate_maximum_packet_size(Self& self, PacketArg const& packet_arg) {
        if (packet_arg.size() > ep.maximum_packet_size_send_) {
            ASYNC_MQTT_LOG("mqtt_impl", error)
                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                << "packet size over maximum_packet_size for sending";
            self.complete(
                make_error(
                    errc::bad_message,
                    "packet size is over maximum_packet_size for sending"
                )
            );
            return false;
        }
        return true;
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename Packet, typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(system_error)
)
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_send(
    Packet packet,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "send:" << packet;
    if constexpr(!std::is_same_v<Packet, basic_packet_variant<PacketIdBytes>>) {
        static_assert(
            (can_send_as_client(Role) && is_client_sendable<std::decay_t<Packet>>()) ||
            (can_send_as_server(Role) && is_server_sendable<std::decay_t<Packet>>()),
            "Packet cannot be send by MQTT protocol"
        );
    }

    return
        async_send(
            force_move(packet),
            false, // not from queue
            std::forward<CompletionToken>(token)
        );
}



template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename Packet, typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void()
)
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_send(
    Packet packet,
    bool from_queue,
    CompletionToken&& token
) {
    return
        as::async_compose<
            CompletionToken,
            void(system_error)
        >(
            send_op<Packet>{
                *this,
                force_move(packet),
                from_queue
            },
            token
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_SEND_HPP
