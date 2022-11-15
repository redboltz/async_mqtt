// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ENDPOINT_HPP)
#define ASYNC_MQTT_ENDPOINT_HPP

#include <set>
#include <deque>
#include <atomic>

#include <async_mqtt/packet/packet_variant.hpp>
#include <async_mqtt/util/value_allocator.hpp>
#include <async_mqtt/stream.hpp>
#include <async_mqtt/store.hpp>
#include <async_mqtt/log.hpp>
#include <async_mqtt/topic_alias_send.hpp>
#include <async_mqtt/topic_alias_recv.hpp>
#include <async_mqtt/packet_id_manager.hpp>
#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/packet/packet_traits.hpp>

namespace async_mqtt {

enum class role {
    client = 0b01,
    server = 0b10,
    both   = 0b11
};

enum class connection_status {
    connecting,
    connected,
    disconnecting,
    disconnected
};

constexpr bool is_client(role r) {
    return static_cast<int>(r) & static_cast<int>(role::client);
}
constexpr bool is_server(role r) {
    return static_cast<int>(r) & static_cast<int>(role::server);
}

inline optional<topic_alias_t> get_topic_alias(properties const& props) {
    optional<topic_alias_t> ta_opt;
    for (auto const& prop : props) {
        prop.visit(
            overload {
                [&](property::topic_alias const& p) {
                    ta_opt.emplace(p.val());
                },
                [](auto const&) {
                }
            }
        );
        if (ta_opt) return ta_opt;
    }
    return ta_opt;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
class basic_endpoint {
public:
    using this_type = basic_endpoint<Role, PacketIdBytes, NextLayer>;
    using stream_type = stream<NextLayer>;
    using strand_type = typename stream_type::strand_type;
    using packet_variant_type = basic_packet_variant<PacketIdBytes>;
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;

    template <typename... Args>
    basic_endpoint(
        protocol_version ver,
        Args&&... args
    ): protocol_version_{ver},
       stream_{std::forward<Args>(args)...}
    {
    }

    stream_type const& stream() const {
        return stream_;
    }
    stream_type& stream() {
        return stream_;
    }

    strand_type const& strand() const {
        return stream().strand();
    }
    strand_type& strand() {
        return stream().strand();
    }

    void set_auto_map_topic_alias_send(bool val) {
        auto_map_topic_alias_send_ = val;
    }

    void set_auto_replace_topic_alias_send(bool val) {
        auto_replace_topic_alias_send_ = val;
    }

    // async functions

    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(optional<packet_id_t>)>::return_type
    acquire_unique_packet_id(
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
                void(optional<packet_id_t>)
            >(
                acquire_unique_packet_id_impl{
                    *this
                },
                token
            );
    }

    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(bool)>::return_type
    register_packet_id(
        packet_id_t packet_id,
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
                void(bool)
            >(
                register_packet_id_impl{
                    *this,
                    packet_id
                },
                token
            );
    }

    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void()>::return_type
    release_packet_id(
        packet_id_t packet_id,
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
                void()
            >(
                release_packet_id_impl{
                    *this,
                    packet_id
                },
                token
            );
    }

    template <typename Packet, typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(system_error)>::return_type
    send(
        Packet packet,
        CompletionToken&& token
    ) {
        if constexpr(!std::is_same_v<Packet, basic_packet_variant<PacketIdBytes>>) {
            static_assert(
                (is_client(Role) && is_client_sendable<std::decay_t<Packet>>()) ||
                (is_server(Role) && is_server_sendable<std::decay_t<Packet>>()),
                "Packet cannot be send by MQTT protocol"
            );
        }

        return
            as::async_compose<
                CompletionToken,
                void(system_error)
            >(
                send_impl<Packet>{
                    *this,
                    force_move(packet)
                },
                token
            );
    }

    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(packet_variant_type)>::return_type
     recv(
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
                void(packet_variant_type)
            >(
                recv_impl{
                    *this
                },
                token
            );
    }

    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void()>::return_type
    restore(
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs,
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
                void()
            >(
                restore_impl{
                    *this,
                    force_move(pvs)
                },
                token
            );
    }

    template <typename CompletionToken>
    typename as::async_result<
        std::decay_t<CompletionToken>,
        void(std::vector<basic_store_packet_variant<PacketIdBytes>>)
    >::return_type
     get_stored(
        CompletionToken&& token
    ) const {
        return
            as::async_compose<
                CompletionToken,
                void(std::vector<basic_store_packet_variant<PacketIdBytes>>)
            >(
                get_stored_impl{
                    *this
                },
                token
            );
    }

private: // compose operation impl

    struct acquire_unique_packet_id_impl {
        this_type& ep;
        enum { dispatch, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch:
                state = complete;
                as::dispatch(
                    ep.strand(),
                    force_move(self)
                );
                break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                self.complete(ep.pid_man_.acquire_unique_id());
                break;
            }
        }
    };

    struct register_packet_id_impl {
        this_type& ep;
        packet_id_t packet_id;
        enum { dispatch, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch:
                state = complete;
                as::dispatch(
                    ep.strand(),
                    force_move(self)
                );
                break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                self.complete(ep.pid_man_.register_id(packet_id));
                break;
            }
        }
    };

    struct release_packet_id_impl {
        this_type& ep;
        packet_id_t packet_id;
        enum { dispatch, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch:
                state = complete;
                as::dispatch(
                    ep.strand(),
                    force_move(self)
                );
                break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                ep.pid_man_.release_id(packet_id);
                self.complete();
                break;
            }
        }
    };

    template <typename Packet>
    struct send_impl {
        this_type& ep;
        Packet packet;
        enum { dispatch, write, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec = error_code{},
            std::size_t /*bytes_transferred*/ = 0
        ) {
            if (ec) {
                self.complete(ec);
                return;
            }

            switch (state) {
            case dispatch:
                state = write;
                as::dispatch(
                    ep.strand(),
                    force_move(self)
                );
                break;
            case write: {
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                state = complete;
                if constexpr(std::is_same_v<std::decay_t<Packet>, basic_packet_variant<PacketIdBytes>>) {
                    packet.visit(
                        overload {
                            [&](auto& actual_packet) {
                                if (process_send_packet(self, actual_packet)) {
                                    ep.stream_.write_packet(
                                        force_move(actual_packet),
                                        force_move(self)
                                    );
                                    if constexpr(is_connack<std::remove_reference_t<decltype(actual_packet)>>()) {
                                        // server send connack after connack sent
                                        ep.send_stored();
                                    }
                                }
                            },
                            [&](system_error&) {}
                        }
                    );
                }
                else {
                    if (process_send_packet(self, packet)) {
                        ep.stream_.write_packet(
                            force_move(packet),
                            force_move(self)
                        );
                        if constexpr(is_connack<Packet>()) {
                            // server send connack after connack sent
                            ep.send_stored();
                        }
                    }
                }
            } break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                self.complete(ec);
                break;
            }
        }

        template <typename Self, typename ActualPacket>
        bool process_send_packet(Self& self, ActualPacket& actual_packet) {
            // MQTT protocol sendable packet check
            if ((is_client(Role) && !is_client_sendable<std::decay_t<ActualPacket>>()) ||
                (is_server(Role) && !is_server_sendable<std::decay_t<ActualPacket>>())
            ) {
                self.complete(
                    make_error(
                        errc::protocol_error,
                        "Packet cannot be send by MQTT protocol"
                    )
                );
                return false;
            }

            // connection status check
            if constexpr(is_connect<ActualPacket>()) {
                if (ep.status_ != connection_status::disconnected) {
                    self.complete(
                        make_error(
                            errc::protocol_error,
                            "connect_packet can only be send on connection_status::disconnected"
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
                            "connsck_packet can only be send on connection_status::connecting"
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
                            "packet can only be send on connection_status::connected"
                        )
                    );
                    return false;
                }
            }
            else {
                if (ep.status_ != connection_status::connected) {
                    self.complete(
                        make_error(
                            errc::protocol_error,
                            "packet can only be send on connection_status::connected"
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
                if (actual_packet.clean_session()) {
                    ep.pid_man_.clear();
                    ep.store_.clear();
                    ep.need_store_ = false;
                }
                else {
                    ep.need_store_ = true;
                }
                ep.topic_alias_send_ = nullopt;
            }

            if constexpr(std::is_same_v<v5::connect_packet, std::decay_t<ActualPacket>>) {
                ep.initialize();
                ep.status_ = connection_status::connecting;
                if (actual_packet.clean_start()) {
                    ep.pid_man_.clear();
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
                if (ep.need_store_ &&
                    (actual_packet.opts().qos() == qos::at_least_once ||
                     actual_packet.opts().qos() == qos::exactly_once)
                ) {
                    if constexpr(is_instance_of<v5::basic_publish_packet, std::decay_t<ActualPacket>>::value) {
                        auto ta_opt = get_topic_alias(actual_packet.props());
                        if (actual_packet.topic().empty()) {
                            auto topic_opt = validate_topic_alias(self, ta_opt);
                            if (!topic_opt) return false;
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
                                    allocate_buffer(*topic_opt),
                                    actual_packet.payload(),
                                    actual_packet.opts(),
                                    force_move(props)
                                );
                            if (!validate_maximum_packet_size(self, store_packet)) return false;
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
                                    actual_packet.payload(),
                                    actual_packet.opts(),
                                    force_move(props)
                                );
                            if (!validate_maximum_packet_size(self, store_packet)) return false;
                            store_packet.set_dup(true);
                            ep.store_.add(force_move(store_packet));
                        }
                    }
                    else {
                        if (!validate_maximum_packet_size(self, actual_packet)) return false;
                        auto store_packet{actual_packet};
                        store_packet.set_dup(true);
                        ep.store_.add(force_move(store_packet));
                    }
                }
            }

            if constexpr(is_instance_of<v5::basic_publish_packet, std::decay_t<ActualPacket>>::value) {
                // apply topic_alias
                auto ta_opt = get_topic_alias(actual_packet.props());
                if (actual_packet.topic().empty()) {
                    if (!topic_alias_validated &&
                        !validate_topic_alias(self, ta_opt)) return false;
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
                            return false;
                        }
                    }
                    else if (ep.auto_map_topic_alias_send_) {
                        if (ep.topic_alias_send_) {
                            if (auto ta_opt = ep.topic_alias_send_->find(actual_packet.topic())) {
                                ASYNC_MQTT_LOG("mqtt_impl", trace)
                                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                    << "topia alias : " << actual_packet.topic() << " - " << ta_opt.value()
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
                                    << "topia alias : " << actual_packet.topic() << " - " << ta_opt.value()
                                    << " is found." ;
                                actual_packet.remove_topic_add_topic_alias(*ta_opt);
                            }
                        }
                    }
                }

                // receive_maximum for sending
                if (ep.enqueue_publish(actual_packet)) {
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
                }
            }

            if constexpr(is_pubrel<std::decay_t<ActualPacket>>()) {
                if (ep.need_store_) ep.store_.add(actual_packet);
            }

            if constexpr(is_instance_of<v5::basic_pubcomp_packet, std::decay_t<ActualPacket>>::value) {
                ep.publish_recv_.erase(actual_packet.packet_id());
            }

            if constexpr(is_disconnect<std::decay_t<ActualPacket>>()) {
                ep.status_ = connection_status::disconnecting;
            }

            if (!validate_maximum_packet_size(self, actual_packet)) {
                return false;
            }

            return true;
        }

        template <typename Self>
        bool validate_topic_alias_range(Self& self, topic_alias_t ta) const {
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
        std::optional<std::string> validate_topic_alias(Self& self, optional<topic_alias_t> ta_opt) const {
            BOOST_ASSERT(ep.strand().running_in_this_thread());
            if (!ta_opt) {
                self.complete(
                    make_error(
                        errc::bad_message,
                        "topic is empty but topic_alias isn't set"
                    )
                );
                return nullopt;
            }

            if (!validate_topic_alias_range(self, *ta_opt)) {
                return nullopt;
            }

            auto topic = ep.topic_alias_send_->find(*ta_opt);
            if (topic.empty()) {
                self.complete(
                    make_error(
                        errc::bad_message,
                        "topic is empty but topic_alias is not registered"
                    )
                );
                return nullopt;
            }
            return topic;
        }

        template <typename Self, typename PacketArg>
        bool validate_maximum_packet_size(Self& self, PacketArg const& packet_arg) const {
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

    struct recv_impl {
        this_type& ep;
        enum { initiate, complete } state = initiate;

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec = error_code{},
            buffer buf = buffer{}
        ) {
            if (ec) {
                ep.status_ = connection_status::disconnected;
                self.complete(system_error{ec});
                return;
            }

            switch (state) {
            case initiate:
                state = complete;
                ep.stream_.read_packet(force_move(self));
                break;
            case complete: {
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                if (buf.size() > ep.maximum_packet_size_recv_) {
                    if (ep.protocol_version_ == protocol_version::v5) {
                        ep.send(
                            v5::disconnect_packet{
                                disconnect_reason_code::packet_too_large
                            },
                            [](system_error const&){}
                        );
                    }
                    self.complete(
                        make_error(
                            errc::bad_message,
                            "too large packet received"
                        )
                    );
                    return;
                }

                bool complete_called = false;
                auto v = buffer_to_basic_packet_variant<PacketIdBytes>(buf, ep.protocol_version_);
                v.visit(
                    // do internal protocol processing
                    overload {
                        [&](v3_1_1::connect_packet& p) {
                            ep.initialize();
                            ep.status_ = connection_status::connecting;
                            if (p.clean_session()) {
                                ep.need_store_ = false;
                            }
                            else {
                                ep.need_store_ = true;
                            }
                        },
                        [&](v5::connect_packet& p) {
                            ep.initialize();
                            ep.status_ = connection_status::connecting;
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
                                    if (!ep.need_store_) {
                                        ep.pid_man_.clear();
                                        ep.store_.clear();
                                    }
                                }
                            }
                        },
                        [&](v5::connack_packet& p) {
                            if (p.code() == connect_reason_code::success) {
                                ep.status_ = connection_status::connected;
                                if (p.session_present()) {
                                    ep.send_stored();
                                }
                                else {
                                    ep.pid_man_.clear();
                                    ep.store_.clear();
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
                                            [](auto const&) {
                                            }
                                        }
                                    );
                                }
                            }
                        },
                        [&](v3_1_1::basic_publish_packet<PacketIdBytes>& p) {
                            if (ep.auto_pub_response_) {
                                switch (p.opts().qos()) {
                                case qos::at_least_once: {
                                    ep.send(
                                        v3_1_1::basic_puback_packet<PacketIdBytes>(p.packet_id()),
                                        [](system_error const&){}
                                    );
                                } break;
                                case qos::exactly_once: {
                                    ep.send(
                                        v3_1_1::basic_pubrec_packet<PacketIdBytes>(p.packet_id()),
                                        [](system_error const&){}
                                    );
                                } break;
                                default:
                                    break;
                                }
                            }
                        },
                        [&](v5::basic_publish_packet<PacketIdBytes>& p) {
                            switch (p.opts().qos()) {
                            case qos::at_least_once: {
                                if (ep.publish_recv_.size() == ep.publish_recv_max_) {
                                    ep.send(
                                        v5::disconnect_packet{
                                            disconnect_reason_code::receive_maximum_exceeded
                                        },
                                        [](system_error const&){}
                                    );
                                    self.complete(
                                        make_error(
                                            errc::bad_message,
                                            "receive maximum exceeded"
                                        )
                                    );
                                    complete_called = true;
                                    return;
                                }
                                auto packet_id = p.packet_id();
                                ep.publish_recv_.insert(packet_id);
                                if (ep.auto_pub_response_) {
                                    ep.send(
                                        v5::basic_puback_packet<PacketIdBytes>{packet_id},
                                        [](system_error const&){}
                                    );
                                }
                            } break;
                            case qos::exactly_once: {
                                if (ep.publish_recv_.size() == ep.publish_recv_max_) {
                                    ep.send(
                                        v5::disconnect_packet{
                                            disconnect_reason_code::receive_maximum_exceeded
                                        },
                                        [](system_error const&){}
                                    );
                                    self.complete(
                                        make_error(
                                            errc::bad_message,
                                            "receive maximum exceeded"
                                        )
                                    );
                                    complete_called = true;
                                    return;
                                }
                                auto packet_id = p.packet_id();
                                ep.publish_recv_.insert(packet_id);
                                if (ep.auto_pub_response_) {
                                    ep.send(
                                        v5::basic_pubrec_packet<PacketIdBytes>{packet_id},
                                        [](system_error const&){}
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
                                        *ta_opt > ep.topic_alias_recv_->max()) {
                                        ep.send(
                                            v5::disconnect_packet{
                                                disconnect_reason_code::topic_alias_invalid
                                            },
                                            [](system_error const&){}
                                        );
                                        self.complete(
                                            make_error(
                                                errc::bad_message,
                                                "topic alias invalid"
                                            )
                                        );
                                    complete_called = true;
                                    }
                                    else {
                                        auto topic = ep.topic_alias_recv_->find(*ta_opt);
                                        if (topic.empty()) {
                                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                                << "no matching topic alias: "
                                                << *ta_opt;
                                            ep.send(
                                                v5::disconnect_packet{
                                                    disconnect_reason_code::topic_alias_invalid
                                                },
                                                [](system_error const&){}
                                            );
                                            self.complete(
                                                make_error(
                                                    errc::bad_message,
                                                    "topic alias invalid"
                                                )
                                            );
                                            complete_called = true;
                                        }
                                        else {
                                            p.add_topic(allocate_buffer(topic));
                                        }
                                    }
                                }
                                else {
                                    ASYNC_MQTT_LOG("mqtt_impl", error)
                                        << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                        << "topic is empty but topic_alias isn't set";
                                    ep.send(
                                        v5::disconnect_packet{
                                            disconnect_reason_code::topic_alias_invalid
                                        },
                                        [](system_error const&){}
                                    );
                                    self.complete(
                                        make_error(
                                            errc::bad_message,
                                            "topic alias invalid"
                                        )
                                    );
                                    complete_called = true;
                                }
                            }
                            else {
                                if (auto ta_opt = get_topic_alias(p.props())) {
                                    if (*ta_opt == 0 ||
                                        *ta_opt > ep.topic_alias_recv_->max()) {
                                        ep.send(
                                            v5::disconnect_packet{
                                                disconnect_reason_code::topic_alias_invalid
                                            },
                                            [](system_error const&){}
                                        );
                                        self.complete(
                                            make_error(
                                                errc::bad_message,
                                                "topic alias invalid"
                                            )
                                        );
                                        complete_called = true;
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
                            ep.store_.erase(response_packet::v3_1_1_puback, packet_id);
                            ep.pid_man_.release_id(packet_id);
                        },
                        [&](v5::basic_puback_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            ep.store_.erase(response_packet::v5_puback, packet_id);
                            ep.pid_man_.release_id(packet_id);
                            --ep.publish_send_count_;
                            send_publish_from_queue();
                        },
                        [&](v3_1_1::basic_pubrec_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            ep.store_.erase(response_packet::v3_1_1_pubrec, packet_id);
                            if (ep.auto_pub_response_) {
                                ep.send(
                                    v3_1_1::basic_pubrel_packet<PacketIdBytes>(packet_id),
                                    [](system_error const&){}
                                );
                            }
                        },
                        [&](v5::basic_pubrec_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            ep.store_.erase(response_packet::v5_pubrec, packet_id);
                            if (is_error(p.code())) {
                                ep.pid_man_.release_id(packet_id);
                                --ep.publish_send_count_;
                                send_publish_from_queue();
                            }
                            else if (ep.auto_pub_response_) {
                                ep.send(
                                    v5::basic_pubrel_packet<PacketIdBytes>(packet_id),
                                    [](system_error const&){}
                                );
                            }
                        },
                        [&](v3_1_1::basic_pubrel_packet<PacketIdBytes>& p) {
                            if (ep.auto_pub_response_) {
                                ep.send(
                                    v3_1_1::basic_pubcomp_packet<PacketIdBytes>(p.packet_id()),
                                    [](system_error const&){}
                                );
                            }
                        },
                        [&](v5::basic_pubrel_packet<PacketIdBytes>& p) {
                            if (ep.auto_pub_response_) {
                                ep.send(
                                    v5::basic_pubcomp_packet<PacketIdBytes>(p.packet_id()),
                                    [](system_error const&){}
                                );
                            }
                        },
                        [&](v3_1_1::basic_pubcomp_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            ep.store_.erase(response_packet::v3_1_1_pubcomp, packet_id);
                            ep.pid_man_.release_id(packet_id);
                            --ep.publish_send_count_;
                            send_publish_from_queue();
                        },
                        [&](v5::basic_pubcomp_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            ep.store_.erase(response_packet::v5_pubcomp, packet_id);
                            ep.pid_man_.release_id(packet_id);
                        },
                        [&](v3_1_1::basic_subscribe_packet<PacketIdBytes>&) {
                        },
                        [&](v5::basic_subscribe_packet<PacketIdBytes>&) {
                        },
                        [&](v3_1_1::basic_suback_packet<PacketIdBytes>&) {
                        },
                        [&](v5::basic_suback_packet<PacketIdBytes>&) {
                        },
                        [&](v3_1_1::basic_unsubscribe_packet<PacketIdBytes>&) {
                        },
                        [&](v5::basic_unsubscribe_packet<PacketIdBytes>&) {
                        },
                        [&](v3_1_1::basic_unsuback_packet<PacketIdBytes>&) {
                        },
                        [&](v5::basic_unsuback_packet<PacketIdBytes>&) {
                        },
                        [&](v3_1_1::pingreq_packet&) {
                        },
                        [&](v5::pingreq_packet&) {
                        },
                        [&](v3_1_1::pingresp_packet&) {
                        },
                        [&](v5::pingresp_packet&) {
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
                            ep.status_ = connection_status::disconnected;
                        }
                    }
                );
                if (!complete_called) self.complete(force_move(v));
            } break;
            }
        }

        void send_publish_from_queue() {
            BOOST_ASSERT(ep.strand().running_in_this_thread());
            while (!ep.publish_queue_.empty() &&
                   ep.publish_send_count_ != ep.publish_send_max_) {
                ++ep.publish_send_count_;
                ep.send(
                    force_move(ep.publish_queue_.front()),
                    [](system_error const&){}
                );
                ep.publish_queue_.pop_front();
            }
        }
    };

    struct restore_impl {
        this_type& ep;
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs;
        enum { dispatch, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch:
                state = complete;
                as::dispatch(
                    ep.strand(),
                    force_move(self)
                );
                break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                for (auto& pv : pvs) {
                    pv.visit(
                        [&](auto& p) {
                            ep.store_.add(force_move(p));
                        }
                    );
                }
                self.complete();
                break;
            }
        }
    };

    struct get_stored_impl {
        this_type const& ep;
        enum { dispatch, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch:
                state = complete;
                as::dispatch(
                    ep.strand(),
                    force_move(self)
                );
                break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                self.complete(ep.store_.get_stored());
                break;
            }
        }
    };

private:

    bool enqueue_publish(v5::basic_publish_packet<PacketIdBytes>& packet) {
        BOOST_ASSERT(strand().running_in_this_thread());
        if (packet.opts().qos() == qos::at_least_once ||
            packet.opts().qos() == qos::exactly_once
        ) {
            if (publish_send_count_ == publish_send_max_) {
                publish_queue_.push_back(force_move(packet));
                return true;
            }
            else {
                ++publish_send_count_;
                if (!publish_queue_.empty()) {
                    publish_queue_.push_back(force_move(packet));
                    return true;
                }
            }
        }
        return false;
    }

    void send_stored() {
        BOOST_ASSERT(strand().running_in_this_thread());
        store_.for_each(
            [&](basic_store_packet_variant<PacketIdBytes> const& pv) {
                pv.visit(
                    // copy packet because the stored packets need to be preserved
                    // until receiving puback/pubrec/pubcomp
                    overload {
                        [&](v3_1_1::basic_publish_packet<PacketIdBytes> p) {
                            send(
                                p,
                                [](system_error const&){}
                            );
                        },
                        [&](v5::basic_publish_packet<PacketIdBytes> p) {
                            if (enqueue_publish(p)) return;
                            send(
                                p,
                                [](system_error const&){}
                            );
                        },
                        [&](v3_1_1::basic_pubrel_packet<PacketIdBytes> p) {
                            send(
                                p,
                                [](system_error const&){}
                            );
                        },
                        [&](v5::basic_pubrel_packet<PacketIdBytes> p) {
                            send(
                                p,
                                [](system_error const&){}
                            );
                        }
                    }
                );
            }
        );
    }

    void initialize() {
        BOOST_ASSERT(strand().running_in_this_thread());
        publish_send_count_ = 0;
        publish_queue_.clear();
        topic_alias_send_ = nullopt;
        topic_alias_recv_ = nullopt;
        need_store_ = false;
    }

private:
    protocol_version protocol_version_;
    stream_type stream_;
    packet_id_manager<packet_id_t> pid_man_;

    bool need_store_ = false;
    store<PacketIdBytes> store_;

    bool auto_pub_response_ = false;

    bool auto_map_topic_alias_send_ = false;
    bool auto_replace_topic_alias_send_ = false;
    optional<topic_alias_send> topic_alias_send_;
    optional<topic_alias_recv> topic_alias_recv_;

    receive_maximum_t publish_send_max_{receive_maximum_max};
    receive_maximum_t publish_recv_max_{receive_maximum_max};
    receive_maximum_t publish_send_count_{0};

    std::set<packet_id_t> publish_recv_;
    std::deque<v5::basic_publish_packet<PacketIdBytes>> publish_queue_;

    std::uint32_t maximum_packet_size_send_{packet_size_no_limit};
    std::uint32_t maximum_packet_size_recv_{packet_size_no_limit};

    std::atomic<connection_status> status_{connection_status::disconnected};
};

template <role Role, typename NextLayer>
using endpoint = basic_endpoint<Role, 2, NextLayer>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_ENDPOINT_HPP
