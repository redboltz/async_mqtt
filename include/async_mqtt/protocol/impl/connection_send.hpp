// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_SEND_HPP)
#define ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_SEND_HPP

#include <async_mqtt/protocol/connection.hpp>
#include <async_mqtt/protocol/event_variant.hpp>
#include <async_mqtt/protocol/event_send.hpp>
#include <async_mqtt/protocol/event_timer.hpp>
#include <async_mqtt/protocol/event_packet_id_released.hpp>

namespace async_mqtt {

namespace detail {

template <role Role, std::size_t PacketIdBytes>
template <typename Packet>
inline
std::tuple<error_code, std::vector<basic_event_variant<PacketIdBytes>>>
basic_connection_impl<Role, PacketIdBytes>::
send(Packet packet) {
    std::vector<basic_event_variant<PacketIdBytes>> events;
    error_code ec;

    auto send_and_post_process =
        [&](auto&& actual_packet) {
            auto ec = process_send_packet(actual_packet, events);
            if (!ec) {
                if constexpr(is_connack<std::remove_reference_t<decltype(actual_packet)>>()) {
                    // server send stored packets after connack sent
                    send_stored(events);
                }
                if constexpr(Role == role::client) {
                    if (pingreq_send_interval_ms_) {
                        events.push_back(
                            event_timer{
                                event_timer::op_type::reset,
                                timer::pingreq_send,
                                *pingreq_send_interval_ms_
                            }
                        );
                    }
                }
            }
        };

    if constexpr(
        std::is_same_v<std::decay_t<Packet>, basic_packet_variant<PacketIdBytes>> ||
        std::is_same_v<std::decay_t<Packet>, basic_store_packet_variant<PacketIdBytes>>
    ) {
        force_move(packet).visit(
            overload{
                [&](auto actual_packet) {
                    send_and_post_process(force_move(actual_packet));
                },
                [&](std::monostate const&) {}
            }
        );
    }
    else {
        send_and_post_process(std::forward<Packet>(packet));
    }

    return {ec, force_move(events)};
}

template <role Role, std::size_t PacketIdBytes>
template <typename ActualPacket>
inline
error_code
basic_connection_impl<Role, PacketIdBytes>::
process_send_packet(
    ActualPacket actual_packet,
    std::vector<basic_event_variant<PacketIdBytes>>& events
) {
    bool release_packet_id_required_if_send_error = false;
    // MQTT protocol sendable packet check
    if (
        !(
            (can_send_as_client(Role) && is_client_sendable<std::decay_t<ActualPacket>>()) ||
            (can_send_as_server(Role) && is_server_sendable<std::decay_t<ActualPacket>>())
        )
    ) {
        return
            make_error_code(
                mqtt_error::packet_not_allowed_to_send
            );
    }

    auto version_check =
        [&] {
            if (protocol_version_ == protocol_version::v3_1_1 && is_v3_1_1<ActualPacket>()) {
                return true;
            }
            if (protocol_version_ == protocol_version::v5 && is_v5<ActualPacket>()) {
                return true;
            }
            return false;
        };

    // connection status check
    if constexpr(is_connect<ActualPacket>()) {
        if (status_ != connection_status::closed) {
            return
                make_error_code(
                    mqtt_error::packet_not_allowed_to_send
                );
        }
        if (!version_check()) {
            return
                make_error_code(
                    mqtt_error::packet_not_allowed_to_send
                );
        }
    }
    else if constexpr(is_connack<ActualPacket>()) {
        if (status_ != connection_status::connecting) {
            return
                make_error_code(
                    mqtt_error::packet_not_allowed_to_send
                );
        }
        if (!version_check()) {
            return
                make_error_code(
                    mqtt_error::packet_not_allowed_to_send
                );
        }
    }
    else if constexpr(std::is_same_v<v5::auth_packet, ActualPacket>) {
        if (status_ != connection_status::connected &&
            status_ != connection_status::connecting) {
            return
                make_error_code(
                    mqtt_error::packet_not_allowed_to_send
                );
        }
        if (!version_check()) {
            return
                make_error_code(
                    mqtt_error::packet_not_allowed_to_send
                );
        }
    }
    else {
        if (status_ != connection_status::connected) {
            if constexpr(!is_publish<std::decay_t<ActualPacket>>()) {
                return
                    make_error_code(
                        mqtt_error::packet_not_allowed_to_send
                    );
            }
        }
        if (!version_check()) {
            return
                make_error_code(
                    mqtt_error::packet_not_allowed_to_send
                );
        }
    }

    // sending process
    bool topic_alias_validated = false;

    if constexpr(std::is_same_v<v3_1_1::connect_packet, std::decay_t<ActualPacket>>) {
        initialize();
        status_ = connection_status::connecting;
        auto keep_alive = actual_packet.keep_alive();
        if (keep_alive != 0 && !pingreq_send_interval_ms_) {
            pingreq_send_interval_ms_.emplace(keep_alive * 1000);
        }
        if (actual_packet.clean_session()) {
            pid_man_.clear();
            store_.clear();
            need_store_ = false;
        }
        else {
            need_store_ = true;
        }
        topic_alias_send_ = std::nullopt;
    }

    if constexpr(std::is_same_v<v5::connect_packet, std::decay_t<ActualPacket>>) {
        initialize();
        status_ = connection_status::connecting;
        auto keep_alive = actual_packet.keep_alive();
        if (keep_alive != 0 && !pingreq_send_interval_ms_) {
            pingreq_send_interval_ms_.emplace(std::chrono::seconds{keep_alive});
        }
        if (actual_packet.clean_start()) {
            pid_man_.clear();
            store_.clear();
        }
        for (auto const& prop : actual_packet.props()) {
            prop.visit(
                overload {
                    [&](property::topic_alias_maximum const& p) {
                        if (p.val() != 0) {
                            topic_alias_recv_.emplace(p.val());
                        }
                    },
                    [&](property::receive_maximum const& p) {
                        BOOST_ASSERT(p.val() != 0);
                        publish_recv_max_ = p.val();
                    },
                    [&](property::maximum_packet_size const& p) {
                        BOOST_ASSERT(p.val() != 0);
                        maximum_packet_size_recv_ = p.val();
                    },
                    [&](property::session_expiry_interval const& p) {
                        if (p.val() != 0) {
                            need_store_ = true;
                        }
                    },
                    [](auto const&){}
                }
            );
        }
    }

    if constexpr(std::is_same_v<v3_1_1::connack_packet, std::decay_t<ActualPacket>>) {
        if (actual_packet.code() == connect_return_code::accepted) {
            status_ = connection_status::connected;
        }
        else {
            status_ = connection_status::disconnecting;
        }
    }

    if constexpr(std::is_same_v<v5::connack_packet, std::decay_t<ActualPacket>>) {
        if (actual_packet.code() == connect_reason_code::success) {
            status_ = connection_status::connected;
            for (auto const& prop : actual_packet.props()) {
                prop.visit(
                    overload {
                        [&](property::topic_alias_maximum const& p) {
                            if (p.val() != 0) {
                                topic_alias_recv_.emplace(p.val());
                            }
                        },
                        [&](property::receive_maximum const& p) {
                            BOOST_ASSERT(p.val() != 0);
                            publish_recv_max_ = p.val();
                        },
                        [&](property::maximum_packet_size const& p) {
                            BOOST_ASSERT(p.val() != 0);
                            maximum_packet_size_recv_ = p.val();
                        },
                        [](auto const&){}
                    }
                );
            }
        }
        else {
            status_ = connection_status::disconnecting;
        }
    }

    // store publish/pubrel packet
    if constexpr(is_publish<std::decay_t<ActualPacket>>()) {
        if (actual_packet.opts().get_qos() == qos::at_least_once ||
            actual_packet.opts().get_qos() == qos::exactly_once
        ) {
            auto packet_id = actual_packet.packet_id();
            BOOST_ASSERT(pid_man_.is_used_id(packet_id));
            if (need_store_) {
                if constexpr(is_instance_of<v5::basic_publish_packet, std::decay_t<ActualPacket>>::value) {
                    auto ta_opt = get_topic_alias(actual_packet.props());
                    if (actual_packet.topic().empty()) {
                        auto topic_opt = validate_topic_alias(ta_opt);
                        if (!topic_opt) {
                            if (packet_id != 0) {
                                release_pid(packet_id);
                            }
                            return
                                make_error_code(
                                    mqtt_error::packet_not_allowed_to_send
                                );
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
                            if (packet_id != 0) {
                                release_pid(packet_id);
                            }
                            return
                                make_error_code(
                                    mqtt_error::packet_not_allowed_to_send
                                );
                        }
                        // add new packet that doesn't have topic_aliass to store
                        // the original packet still use topic alias to send
                        store_packet.set_dup(true);
                        store_.add(force_move(store_packet));
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
                            if (packet_id != 0) {
                                release_pid(packet_id);
                            }
                            return
                                make_error_code(
                                    mqtt_error::packet_not_allowed_to_send
                                );
                        }
                        store_packet.set_dup(true);
                        store_.add(force_move(store_packet));
                    }
                }
                else {
                    // v3_1_1 publish_packet
                    auto store_packet{actual_packet};
                    store_packet.set_dup(true);
                    store_.add(force_move(store_packet));
                }
            }
            else {
                // QoS1, 2 but not stored
                release_packet_id_required_if_send_error = true;
            }
            if (actual_packet.opts().get_qos() == qos::exactly_once) {
                qos2_publish_processing_.insert(packet_id);
                pid_pubrec_.insert(packet_id);
            }
            else {
                pid_puback_.insert(packet_id);
            }
        }
    }

    if constexpr(is_instance_of<v5::basic_publish_packet, std::decay_t<ActualPacket>>::value) {
        // apply topic_alias
        auto ta_opt = get_topic_alias(actual_packet.props());
        if (actual_packet.topic().empty()) {
            if (!topic_alias_validated &&
                !validate_topic_alias(ta_opt)) {
                auto packet_id = actual_packet.packet_id();
                if (packet_id != 0) {
                    release_pid(packet_id);
                }
                return
                    make_error_code(
                        mqtt_error::packet_not_allowed_to_send
                    );
            }
            // use topic_alias set by user
        }
        else {
            if (ta_opt) {
                if (validate_topic_alias_range(*ta_opt)) {
                    ASYNC_MQTT_LOG("mqtt_impl", trace)
                        << "topia alias : "
                        << actual_packet.topic() << " - " << *ta_opt
                        << " is registered." ;
                    BOOST_ASSERT(topic_alias_send_);
                    topic_alias_send_->insert_or_update(actual_packet.topic(), *ta_opt);
                }
                else {
                    auto packet_id = actual_packet.packet_id();
                    if (packet_id != 0) {
                        release_pid(packet_id);
                    }
                    return
                        make_error_code(
                            mqtt_error::packet_not_allowed_to_send
                        );
                }
            }
            else if (auto_map_topic_alias_send_) {
                if (topic_alias_send_) {
                    if (auto ta_opt = topic_alias_send_->find(actual_packet.topic())) {
                        ASYNC_MQTT_LOG("mqtt_impl", trace)
                            << "topia alias : " << actual_packet.topic() << " - " << *ta_opt
                            << " is found." ;
                        actual_packet.remove_topic_add_topic_alias(*ta_opt);
                    }
                    else {
                        auto lru_ta = topic_alias_send_->get_lru_alias();
                        topic_alias_send_->insert_or_update(actual_packet.topic(), lru_ta); // remap topic alias
                        actual_packet.add_topic_alias(lru_ta);
                    }
                }
            }
            else if (auto_replace_topic_alias_send_) {
                if (topic_alias_send_) {
                    if (auto ta_opt = topic_alias_send_->find(actual_packet.topic())) {
                        ASYNC_MQTT_LOG("mqtt_impl", trace)
                            << "topia alias : " << actual_packet.topic() << " - " << *ta_opt
                            << " is found." ;
                        actual_packet.remove_topic_add_topic_alias(*ta_opt);
                    }
                }
            }
        }

        // receive_maximum for sending
        if (actual_packet.opts().get_qos() == qos::at_least_once ||
            actual_packet.opts().get_qos() == qos::exactly_once
        ) {
            if (publish_send_count_ == publish_send_max_) {
                return
                    make_error_code(
                        disconnect_reason_code::receive_maximum_exceeded
                    );
            }
            ++publish_send_count_;
        }
    }

    if constexpr(is_instance_of<v5::basic_puback_packet, std::decay_t<ActualPacket>>::value) {
        publish_recv_.erase(actual_packet.packet_id());
    }

    if constexpr(is_instance_of<v5::basic_pubrec_packet, std::decay_t<ActualPacket>>::value) {
        if (make_error_code(actual_packet.code())) {
            publish_recv_.erase(actual_packet.packet_id());
            qos2_publish_handled_.erase(actual_packet.packet_id());
        }
    }

    if constexpr(is_pubrel<std::decay_t<ActualPacket>>()) {
        auto packet_id = actual_packet.packet_id();
        BOOST_ASSERT(pid_man_.is_used_id(packet_id));
        if (need_store_) store_.add(actual_packet);
        pid_pubcomp_.insert(packet_id);
    }

    if constexpr(is_instance_of<v5::basic_pubcomp_packet, std::decay_t<ActualPacket>>::value) {
        publish_recv_.erase(actual_packet.packet_id());
    }

    if constexpr(is_subscribe<std::decay_t<ActualPacket>>()) {
        auto packet_id = actual_packet.packet_id();
        BOOST_ASSERT(pid_man_.is_used_id(packet_id));
        pid_suback_.insert(packet_id);
        release_packet_id_required_if_send_error = true;
    }

    if constexpr(is_unsubscribe<std::decay_t<ActualPacket>>()) {
        auto packet_id = actual_packet.packet_id();
        BOOST_ASSERT(pid_man_.is_used_id(packet_id));
        pid_unsuback_.insert(packet_id);
        release_packet_id_required_if_send_error = true;
    }

    if constexpr(is_pingreq<std::decay_t<ActualPacket>>()) {
        // events order
        // pingreq packet send
        // if success, pingresp timer set
        events.emplace_back(
            event_send{
                force_move(actual_packet),
                release_packet_id_required_if_send_error
            }
        );
        if (pingresp_recv_timeout_ms_) {
            events.emplace_back(
                event_timer{
                    event_timer::op_type::reset,
                    timer::pingresp_recv,
                    *pingresp_recv_timeout_ms_
                }
            );
        }
        return error_code{};
    }

    if constexpr(is_disconnect<std::decay_t<ActualPacket>>()) {
        status_ = connection_status::disconnecting;
    }

    if (!validate_maximum_packet_size(actual_packet.size())) {
        if constexpr(own_packet_id<std::decay_t<ActualPacket>>()) {
            auto packet_id = actual_packet.packet_id();
            if (packet_id != 0) {
                release_pid(packet_id);
            }
        }
        return
            make_error_code(
                mqtt_error::packet_not_allowed_to_send
            );
    }

    if constexpr(is_publish<std::decay_t<ActualPacket>>()) {
        if (status_ != connection_status::connected) {
            ASYNC_MQTT_LOG("mqtt_impl", error)
                << "publish message try to send but not connected";
            return
                make_error_code(
                    mqtt_error::packet_not_allowed_to_send
                );
        }
    }

    events.emplace_back(
        basic_event_send<PacketIdBytes>{
            force_move(actual_packet),
            release_packet_id_required_if_send_error
        }
    );
    return error_code{};
}

template <role Role, std::size_t PacketIdBytes>
inline
void
basic_connection_impl<Role, PacketIdBytes>::
send_stored(std::vector<basic_event_variant<PacketIdBytes>>& events) const {
    store_.for_each(
        [&](basic_store_packet_variant<PacketIdBytes> const& pv) {
            if (pv.size() > maximum_packet_size_send_) {
                pid_man_.release_id(pv.packet_id());
                // TBD some event should be pushed (size error not send id reusable)
                // Or perhaps nothing is required
                return false;
            }
            pv.visit(
                // copy packet because the stored packets need to be preserved
                // until receiving puback/pubrec/pubcomp
                [&](auto const& p) {
                    events.push_back(event_send{p});
                }
            );
            return true;
        }
   );
}

template <role Role, std::size_t PacketIdBytes>
inline
constexpr bool
basic_connection_impl<Role, PacketIdBytes>::can_send_as_client(role r) {
    return
        static_cast<int>(r) &
        static_cast<int>(role::client);
}

template <role Role, std::size_t PacketIdBytes>
inline
constexpr bool
basic_connection_impl<Role, PacketIdBytes>::can_send_as_server(role r) {
    return
        static_cast<int>(r) &
        static_cast<int>(role::server);
}

} // namespace detail

// connection public member functions

template <role Role, std::size_t PacketIdBytes>
template <typename Packet>
inline
std::tuple<error_code, std::vector<basic_event_variant<PacketIdBytes>>>
basic_connection<Role, PacketIdBytes>::
send(Packet packet) {
    BOOST_ASSERT(impl_);
    return impl_->send(std::forward<Packet>(packet));
}

} // namespace async_mqtt


#endif // ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_SEND_HPP
