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
    std::optional<typename basic_packet_id_type<PacketIdBytes>::type> release_pid_opt = std::nullopt;
    enum { dispatch, write, complete } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self,
        error_code ec = error_code{},
        std::size_t /*bytes_transferred*/ = 0
    ) {
        auto& a_ep{*ep};
        if (ec) {
            ASYNC_MQTT_LOG("mqtt_impl", info)
                << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                << "send error:" << ec.message();
            if (release_pid_opt) {
                a_ep.release_pid(*release_pid_opt);
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
            state = complete;
            if constexpr(
                std::is_same_v<std::decay_t<Packet>, basic_packet_variant<PacketIdBytes>> ||
                std::is_same_v<std::decay_t<Packet>, basic_store_packet_variant<PacketIdBytes>>
            ) {
                packet.visit(
                    overload {
                        [&](auto actual_packet) {
                            if (process_send_packet(self, actual_packet)) {
                                auto ep_copy = ep;
                                a_ep.stream_.async_write_packet(
                                    actual_packet,
                                    force_move(self)
                                );
                                if constexpr(is_connack<std::remove_reference_t<decltype(actual_packet)>>()) {
                                    // server send stored packets after connack sent
                                    send_stored(ep_copy);
                                }
                                if constexpr(Role == role::client) {
                                    reset_pingreq_send_timer(force_move(ep_copy));
                                }
                            }
                        },
                        [&](std::monostate const&) {}
                    }
                );
            }
            else {
                if (process_send_packet(self, packet)) {
                    auto a_packet{packet};
                    auto ep_copy = ep;
                    a_ep.stream_.async_write_packet(
                        force_move(a_packet),
                        force_move(self)
                    );
                    if constexpr(is_connack<Packet>()) {
                        // server send stored packets after connack sent
                        send_stored(ep_copy);
                    }
                    if constexpr(Role == role::client) {
                        reset_pingreq_send_timer(force_move(ep_copy));
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
                make_error_code(
                    mqtt_error::packet_not_allowed_to_send
                )
            );
            return false;
        }

        auto version_check =
            [&] {
                if (ep->protocol_version_ == protocol_version::v3_1_1 && is_v3_1_1<ActualPacket>()) {
                    return true;
                }
                if (ep->protocol_version_ == protocol_version::v5 && is_v5<ActualPacket>()) {
                    return true;
                }
                return false;
            };

        // connection status check
        if constexpr(is_connect<ActualPacket>()) {
            if (ep->status_ != connection_status::closed) {
                self.complete(
                    make_error_code(
                        mqtt_error::packet_not_allowed_to_send
                    )
                );
                return false;
            }
            if (!version_check()) {
                self.complete(
                    make_error_code(
                        mqtt_error::packet_not_allowed_to_send
                    )
                );
                return false;
            }
        }
        else if constexpr(is_connack<ActualPacket>()) {
            if (ep->status_ != connection_status::connecting) {
                self.complete(
                    make_error_code(
                        mqtt_error::packet_not_allowed_to_send
                    )
                );
                return false;
            }
            if (!version_check()) {
                self.complete(
                    make_error_code(
                        mqtt_error::packet_not_allowed_to_send
                    )
                );
                return false;
            }
        }
        else if constexpr(std::is_same_v<v5::auth_packet, ActualPacket>) {
            if (ep->status_ != connection_status::connected &&
                ep->status_ != connection_status::connecting) {
                self.complete(
                    make_error_code(
                        mqtt_error::packet_not_allowed_to_send
                    )
                );
                return false;
            }
            if (!version_check()) {
                self.complete(
                    make_error_code(
                        mqtt_error::packet_not_allowed_to_send
                    )
                );
                return false;
            }
        }
        else {
            if (ep->status_ != connection_status::connected) {
                if constexpr(!is_publish<std::decay_t<ActualPacket>>()) {
                    self.complete(
                        make_error_code(
                            mqtt_error::packet_not_allowed_to_send
                        )
                    );
                    return false;
                }
            }
            if (!version_check()) {
                self.complete(
                    make_error_code(
                        mqtt_error::packet_not_allowed_to_send
                    )
                );
                return false;
            }
        }

        // sending process
        bool topic_alias_validated = false;

        if constexpr(std::is_same_v<v3_1_1::connect_packet, std::decay_t<ActualPacket>>) {
            ep->initialize();
            ep->status_ = connection_status::connecting;
            auto keep_alive = actual_packet.keep_alive();
            if (keep_alive != 0 && !ep->pingreq_send_interval_ms_) {
                ep->pingreq_send_interval_ms_.emplace(keep_alive * 1000);
            }
            if (actual_packet.clean_session()) {
                ep->clear_pid_man();
                ep->store_.clear();
                ep->need_store_ = false;
            }
            else {
                ep->need_store_ = true;
            }
            ep->topic_alias_send_ = std::nullopt;
        }

        if constexpr(std::is_same_v<v5::connect_packet, std::decay_t<ActualPacket>>) {
            ep->initialize();
            ep->status_ = connection_status::connecting;
            auto keep_alive = actual_packet.keep_alive();
            if (keep_alive != 0 && !ep->pingreq_send_interval_ms_) {
                ep->pingreq_send_interval_ms_.emplace(std::chrono::seconds{keep_alive});
            }
            if (actual_packet.clean_start()) {
                ep->clear_pid_man();
                ep->store_.clear();
            }
            for (auto const& prop : actual_packet.props()) {
                prop.visit(
                    overload {
                        [&](property::topic_alias_maximum const& p) {
                            if (p.val() != 0) {
                                ep->topic_alias_recv_.emplace(p.val());
                            }
                        },
                        [&](property::receive_maximum const& p) {
                            BOOST_ASSERT(p.val() != 0);
                            ep->publish_recv_max_ = p.val();
                        },
                        [&](property::maximum_packet_size const& p) {
                            BOOST_ASSERT(p.val() != 0);
                            ep->maximum_packet_size_recv_ = p.val();
                        },
                        [&](property::session_expiry_interval const& p) {
                            if (p.val() != 0) {
                                ep->need_store_ = true;
                            }
                        },
                        [](auto const&){}
                    }
                );
            }
        }

        if constexpr(std::is_same_v<v3_1_1::connack_packet, std::decay_t<ActualPacket>>) {
            if (actual_packet.code() == connect_return_code::accepted) {
                ep->status_ = connection_status::connected;
            }
            else {
                ep->status_ = connection_status::disconnecting;
            }
        }

        if constexpr(std::is_same_v<v5::connack_packet, std::decay_t<ActualPacket>>) {
            if (actual_packet.code() == connect_reason_code::success) {
                ep->status_ = connection_status::connected;
                for (auto const& prop : actual_packet.props()) {
                    prop.visit(
                        overload {
                            [&](property::topic_alias_maximum const& p) {
                                if (p.val() != 0) {
                                    ep->topic_alias_recv_.emplace(p.val());
                                }
                            },
                            [&](property::receive_maximum const& p) {
                                BOOST_ASSERT(p.val() != 0);
                                ep->publish_recv_max_ = p.val();
                            },
                            [&](property::maximum_packet_size const& p) {
                                BOOST_ASSERT(p.val() != 0);
                                ep->maximum_packet_size_recv_ = p.val();
                            },
                            [](auto const&){}
                        }
                    );
                }
            }
            else {
                ep->status_ = connection_status::disconnecting;
            }
        }

        // store publish/pubrel packet
        if constexpr(is_publish<std::decay_t<ActualPacket>>()) {
            if (actual_packet.opts().get_qos() == qos::at_least_once ||
                actual_packet.opts().get_qos() == qos::exactly_once
            ) {
                auto packet_id = actual_packet.packet_id();
                BOOST_ASSERT(ep->pid_man_.is_used_id(packet_id));
                release_pid_opt.emplace(packet_id);
                if (ep->need_store_) {
                    if constexpr(is_instance_of<v5::basic_publish_packet, std::decay_t<ActualPacket>>::value) {
                        auto ta_opt = get_topic_alias(actual_packet.props());
                        if (actual_packet.topic().empty()) {
                            auto topic_opt = validate_topic_alias(ta_opt);
                            if (!topic_opt) {
                                self.complete(
                                    make_error_code(
                                        mqtt_error::packet_not_allowed_to_send
                                    )
                                );
                                if (packet_id != 0) {
                                    ep->release_pid(packet_id);
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
                                    packet_id,
                                    force_move(*topic_opt),
                                    actual_packet.payload_as_buffer(),
                                    actual_packet.opts(),
                                    force_move(props)
                                );
                            if (!validate_maximum_packet_size(store_packet.size())) {
                                self.complete(
                                    make_error_code(
                                        mqtt_error::packet_not_allowed_to_send
                                    )
                                );
                                if (packet_id != 0) {
                                    ep->release_pid(packet_id);
                                }
                                return false;
                            }
                            // add new packet that doesn't have topic_aliass to store
                            // the original packet still use topic alias to send
                            store_packet.set_dup(true);
                            ep->store_.add(force_move(store_packet));
                            // even if send error would happens
                            // packet_id should be remained
                            release_pid_opt.reset();
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
                                    packet_id,
                                    actual_packet.topic(),
                                    actual_packet.payload_as_buffer(),
                                    actual_packet.opts(),
                                    force_move(props)
                                );
                            if (!validate_maximum_packet_size(store_packet.size())) {
                                self.complete(
                                    make_error_code(
                                        mqtt_error::packet_not_allowed_to_send
                                    )
                                );
                                if (packet_id != 0) {
                                    ep->release_pid(packet_id);
                                }
                                return false;
                            }
                            store_packet.set_dup(true);
                            ep->store_.add(force_move(store_packet));
                            // even if send error would happens
                            // packet_id should be remained
                            release_pid_opt.reset();
                        }
                    }
                    else {
                        if (!validate_maximum_packet_size(actual_packet.size())) {
                            self.complete(
                                make_error_code(
                                    mqtt_error::packet_not_allowed_to_send
                                )
                            );
                            if (packet_id != 0) {
                                ep->release_pid(packet_id);
                            }
                            return false;
                        }
                        auto store_packet{actual_packet};
                        store_packet.set_dup(true);
                        ep->store_.add(force_move(store_packet));
                        // even if send error would happens
                        // packet_id should be remained
                        release_pid_opt.reset();
                    }
                }
                if (actual_packet.opts().get_qos() == qos::exactly_once) {
                    ep->qos2_publish_processing_.insert(packet_id);
                    ep->pid_pubrec_.insert(packet_id);
                }
                else {
                    ep->pid_puback_.insert(packet_id);
                }
            }
        }

        if constexpr(is_instance_of<v5::basic_publish_packet, std::decay_t<ActualPacket>>::value) {
            // apply topic_alias
            auto ta_opt = get_topic_alias(actual_packet.props());
            if (actual_packet.topic().empty()) {
                if (!topic_alias_validated &&
                    !validate_topic_alias(ta_opt)) {
                    self.complete(
                        make_error_code(
                            mqtt_error::packet_not_allowed_to_send
                        )
                    );
                    auto packet_id = actual_packet.packet_id();
                    if (packet_id != 0) {
                        ep->release_pid(packet_id);
                    }
                    return false;
                }
                // use topic_alias set by user
            }
            else {
                if (ta_opt) {
                    if (validate_topic_alias_range(*ta_opt)) {
                        ASYNC_MQTT_LOG("mqtt_impl", trace)
                            << ASYNC_MQTT_ADD_VALUE(address, &ep)
                            << "topia alias : "
                            << actual_packet.topic() << " - " << *ta_opt
                            << " is registered." ;
                        BOOST_ASSERT(ep->topic_alias_send_);
                        ep->topic_alias_send_->insert_or_update(actual_packet.topic(), *ta_opt);
                    }
                    else {
                        self.complete(
                            make_error_code(
                                mqtt_error::packet_not_allowed_to_send
                            )
                        );
                        auto packet_id = actual_packet.packet_id();
                        if (packet_id != 0) {
                            ep->release_pid(packet_id);
                        }
                        return false;
                    }
                }
                else if (ep->auto_map_topic_alias_send_) {
                    if (ep->topic_alias_send_) {
                        if (auto ta_opt = ep->topic_alias_send_->find(actual_packet.topic())) {
                            ASYNC_MQTT_LOG("mqtt_impl", trace)
                                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                << "topia alias : " << actual_packet.topic() << " - " << *ta_opt
                                << " is found." ;
                            actual_packet.remove_topic_add_topic_alias(*ta_opt);
                        }
                        else {
                            auto lru_ta = ep->topic_alias_send_->get_lru_alias();
                            ep->topic_alias_send_->insert_or_update(actual_packet.topic(), lru_ta); // remap topic alias
                            actual_packet.add_topic_alias(lru_ta);
                        }
                    }
                }
                else if (ep->auto_replace_topic_alias_send_) {
                    if (ep->topic_alias_send_) {
                        if (auto ta_opt = ep->topic_alias_send_->find(actual_packet.topic())) {
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
            if (!from_queue && ep->enqueue_publish(actual_packet)) {
                self.complete(
                    error_code{}
                );
                return false;
            }
        }

        if constexpr(is_instance_of<v5::basic_puback_packet, std::decay_t<ActualPacket>>::value) {
            ep->publish_recv_.erase(actual_packet.packet_id());
        }

        if constexpr(is_instance_of<v5::basic_pubrec_packet, std::decay_t<ActualPacket>>::value) {
            if (make_error_code(actual_packet.code())) {
                ep->publish_recv_.erase(actual_packet.packet_id());
                ep->qos2_publish_handled_.erase(actual_packet.packet_id());
            }
        }

        if constexpr(is_pubrel<std::decay_t<ActualPacket>>()) {
            auto packet_id = actual_packet.packet_id();
            BOOST_ASSERT(ep->pid_man_.is_used_id(packet_id));
            if (ep->need_store_) ep->store_.add(actual_packet);
            ep->pid_pubcomp_.insert(packet_id);
        }

        if constexpr(is_instance_of<v5::basic_pubcomp_packet, std::decay_t<ActualPacket>>::value) {
            ep->publish_recv_.erase(actual_packet.packet_id());
        }

        if constexpr(is_subscribe<std::decay_t<ActualPacket>>()) {
            auto packet_id = actual_packet.packet_id();
            BOOST_ASSERT(ep->pid_man_.is_used_id(packet_id));
            ep->pid_suback_.insert(packet_id);
            release_pid_opt.emplace(packet_id);
        }

        if constexpr(is_unsubscribe<std::decay_t<ActualPacket>>()) {
            auto packet_id = actual_packet.packet_id();
            BOOST_ASSERT(ep->pid_man_.is_used_id(packet_id));
            ep->pid_unsuback_.insert(packet_id);
            release_pid_opt.emplace(packet_id);
        }

        if constexpr(is_pingreq<std::decay_t<ActualPacket>>()) {
            reset_pingresp_recv_timer(ep);
        }

        if constexpr(is_disconnect<std::decay_t<ActualPacket>>()) {
            ep->status_ = connection_status::disconnecting;
        }

        if (!validate_maximum_packet_size(actual_packet.size())) {
            self.complete(
                make_error_code(
                    mqtt_error::packet_not_allowed_to_send
                )
            );
            if constexpr(own_packet_id<std::decay_t<ActualPacket>>()) {
                auto packet_id = actual_packet.packet_id();
                if (packet_id != 0) {
                    ep->release_pid(packet_id);
                }
            }
            return false;
        }

        if constexpr(is_publish<std::decay_t<ActualPacket>>()) {
            if (ep->status_ != connection_status::connected) {
                // offline publish
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                    << "publish message is not sent but stored";
                self.complete(
                    error_code{}
                );
                return false;
            }
        }
        return true;
    }

    bool validate_topic_alias_range(topic_alias_type ta);
    std::optional<std::string> validate_topic_alias(std::optional<topic_alias_type> ta_opt);
    bool validate_maximum_packet_size(std::size_t size);
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

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/impl/endpoint_send.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_ENDPOINT_SEND_HPP
