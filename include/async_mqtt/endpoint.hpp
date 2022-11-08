// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ENDPOINT_HPP)
#define ASYNC_MQTT_ENDPOINT_HPP


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

constexpr bool is_client(role r) {
    return static_cast<int>(r) & static_cast<int>(role::client);
}
constexpr bool is_server(role r) {
    return static_cast<int>(r) & static_cast<int>(role::server);
}

template <typename NextLayer, role Role, std::size_t PacketIdBytes>
class basic_endpoint {
public:
    using this_type = basic_endpoint<NextLayer, Role, PacketIdBytes>;
    using stream_type = stream<NextLayer>;
    using strand_type = typename stream_type::strand_type;
    using variant_type = basic_packet_variant<PacketIdBytes>;
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

    template <
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken, optional<packet_id_t>>::value
        >* = nullptr
    >
    auto acquire_unique_packet_id(
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

    template <
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken, bool>::value
        >* = nullptr
    >
    auto register_packet_id(
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

    template <
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken>::value
        >* = nullptr
    >
    auto release_packet_id(
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

    template <
        typename Packet,
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken, system_error>::value
        >* = nullptr
    >
    auto send(
        Packet&& packet,
        CompletionToken&& token
    ) {
        static_assert(
            (is_client(Role) && is_client_sendable<Packet>()) ||
            (is_server(Role) && is_server_sendable<Packet>()),
            "Packet cannot be send by MQTT protocol"
        );
        return
            as::async_compose<
                CompletionToken,
                void(system_error)
            >(
                send_impl<Packet>{
                    *this,
                    std::forward<Packet>(packet)
                },
                token
            );
    }

    template <
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken, variant_type>::value
        >* = nullptr
    >
    auto recv(
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
                void(variant_type)
            >(
                recv_impl{
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
            std::size_t bytes_transferred = 0
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
                bool topic_alias_validated = false;
                if constexpr(std::is_same_v<v3_1_1::connect_packet, Packet>) {
                    ep.need_store_ = !packet.clean_session();
                }
                else if constexpr(std::is_same_v<v3_1_1::connect_packet, Packet>) {
                    ep.need_store_ = !packet.clean_start();
                }
                // store publish/pubrel packet
                else if constexpr(is_publish<Packet>()) {
                    if (ep.need_store_ &&
                        (packet.opts().qos() == qos::at_least_once ||
                         packet.opts().qos() == qos::exactly_once)
                    ) {
                        if constexpr(is_instance_of<v5::basic_publish_packet, Packet>::value) {
                            auto ta_opt = get_topic_alias();
                            if (packet.topic().empty()) {
                                auto topic_opt = validate_topic_alias(self, ta_opt);
                                if (!topic_opt) return;
                                topic_alias_validated = true;
                                auto props = packet.props();
                                auto it = props.cbegin();
                                auto end = props.cend();
                                for (; it != end; ++it) {
                                    if (it->id() == property::id::topic_alias) {
                                        props.erase(it);
                                        break;
                                    }
                                }
                                // add new packet that doesn't have topic_aliass to store
                                // the original packet still use topic alias to send
                                ep.store_.add(
                                    Packet(
                                        packet.packet_id(),
                                        allocate_buffer(*topic_opt),
                                        packet.payload(),
                                        packet.opts(),
                                        force_move(props)
                                    )
                                );
                            }
                            else {
                                ep.store_.add(packet);
                            }
                        }
                        else {
                            ep.store_.add(packet);
                        }
                    }
                }
                else if constexpr(is_pubrel<Packet>()) {
                    if (ep.need_store_) ep.store_.add(packet);
                }

                // apply topic_alias
                if constexpr(is_instance_of<v5::basic_publish_packet, Packet>::value) {
                    auto ta_opt = get_topic_alias();
                    if (packet.topic().empty()) {
                        if (!topic_alias_validated &&
                            !validate_topic_alias(self, ta_opt)) return;
                        // use topic_alias set by user
                    }
                    else {
                        if (ta_opt) {
                            ASYNC_MQTT_LOG("mqtt_impl", trace)
                                << ASYNC_MQTT_ADD_VALUE(address, this)
                                << "topia alias : "
                                << packet.topic() << " - " << *ta_opt
                                << " is registered." ;
                            ep.topic_alias_send_->insert_or_update(packet.topic(), *ta_opt);
                        }
                        else if (ep.auto_map_topic_alias_send_) {
                            if (ep.topic_alias_send_) {
                                if (auto ta_opt = ep.topic_alias_send_->find(packet.topic())) {
                                    ASYNC_MQTT_LOG("mqtt_impl", trace)
                                        << ASYNC_MQTT_ADD_VALUE(address, this)
                                        << "topia alias : " << packet.topic() << " - " << ta_opt.value()
                                        << " is found." ;
                                    ep.topic_alias_send_->insert_or_update(packet.topic(), *ta_opt); // update
                                    packet.remove_topic_add_topic_alias(*ta_opt);
                                }
                                else {
                                    auto lru_ta = ep.topic_alias_send_->get_lru_alias();
                                    ep.topic_alias_send_->insert_or_update(packet.topic(), lru_ta); // remap topic alias
                                    packet.add_topic_alias(*ta_opt);
                                }
                            }
                        }
                        else if (ep.auto_replace_topic_alias_send_) {
                            if (ep.topic_alias_send_) {
                                if (auto ta_opt = ep.topic_alias_send_->find(packet.topic())) {
                                    ASYNC_MQTT_LOG("mqtt_impl", trace)
                                        << ASYNC_MQTT_ADD_VALUE(address, this)
                                        << "topia alias : " << packet.topic() << " - " << ta_opt.value()
                                        << " is found." ;
                                    ep.topic_alias_send_->insert_or_update(packet.topic(), *ta_opt); // update
                                    packet.remove_topic_add_topic_alias(*ta_opt);
                                }
                            }
                        }
                    }
                }

                ep.stream_.write_packet(
                    force_move(packet),
                    force_move(self)
                );
            } break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                self.complete(ec);
                break;
            }
        }

        optional<topic_alias_t> get_topic_alias() const {
            for (auto const& prop : packet.props()) {
                prop.visit(
                    overload {
                        [&](property::topic_alias const& p) -> optional<topic_alias_t> {
                            return p.val();
                        },
                        [](auto const&) -> optional<topic_alias_t> {
                            return nullopt;
                        }
                    }
                );
            }
            return nullopt;
        }

        template <typename Self>
        std::optional<std::string> validate_topic_alias(Self& self, optional<topic_alias_t> ta_opt) const {
            if (!ta_opt) {
                self.complete(
                    make_error(
                        errc::bad_message,
                        "topic is empty but topic_alias doesn't set"
                    )
                );
                return nullopt;
            }

            if (!ep.topic_alias_send_) {
                self.complete(
                    make_error(
                        errc::bad_message,
                        "topic is empty but topic_alias_maximum is 0"
                    )
                );
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
                auto v = buffer_to_basic_packet_variant<PacketIdBytes>(buf, ep.protocol_version_);
                v.visit(
                    // do internal protocol processing
                    overload {
                        [&](v3_1_1::connect_packet const& p) {
                        },
                        [&](v3_1_1::connack_packet const& p) {
                            if (p.session_present()) {
                            }
                            else {
                                if (!ep.need_store_) {
                                    ep.pid_man_.clear();
                                    ep.store_.clear();
                                }
                            }
                        },
                        [&](v3_1_1::basic_publish_packet<PacketIdBytes> const& p) {
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
                        [&](v3_1_1::basic_puback_packet<PacketIdBytes> const& p) {
                            ep.store_.erase(response_packet::v3_1_1_puback, p.packet_id());
                            ep.pid_man_.release_id(p.packet_id());
                        },
                        [&](v3_1_1::basic_pubrec_packet<PacketIdBytes> const& p) {
                            ep.store_.erase(response_packet::v3_1_1_pubrec, p.packet_id());
                            if (ep.auto_pub_response_) {
                                ep.send(
                                    v3_1_1::basic_pubrel_packet<PacketIdBytes>(p.packet_id()),
                                    [](system_error const&){}
                                );
                            }
                        },
                        [&](v3_1_1::basic_pubrel_packet<PacketIdBytes> const& p) {
                            if (ep.auto_pub_response_) {
                                ep.send(
                                    v3_1_1::basic_pubcomp_packet<PacketIdBytes>(p.packet_id()),
                                    [](system_error const&){}
                                );
                            }
                        },
                        [&](v3_1_1::basic_pubcomp_packet<PacketIdBytes> const& p) {
                            ep.store_.erase(response_packet::v3_1_1_pubcomp, p.packet_id());
                            ep.pid_man_.release_id(p.packet_id());
                        },
                        [&](v3_1_1::basic_subscribe_packet<PacketIdBytes> const& p) {
                        },
                        [&](v3_1_1::basic_suback_packet<PacketIdBytes> const& p) {
                        },
                        [&](v3_1_1::basic_unsubscribe_packet<PacketIdBytes> const& p) {
                        },
                        [&](v3_1_1::basic_unsuback_packet<PacketIdBytes> const& p) {
                        },
                        [&](v3_1_1::pingreq_packet const& p) {
                        },
                        [&](v3_1_1::pingresp_packet const& p) {
                        },
                        [&](v3_1_1::disconnect_packet const& p) {
                        },
                        [&](v5::connect_packet const& p) {
                        },
                        [&](v5::connack_packet const& p) {
                            for (auto const& prop : p.props()) {
                                prop.visit(
                                    overload {
                                        [&](property::topic_alias_maximum const& p) {
                                            if (p.val() > 0) {
                                                ep.topic_alias_send_.emplace(p.val());
                                            }
                                        },
                                        [](auto const&) {
                                        }
                                    }
                                );
                            }
                            if (p.session_present()) {
                            }
                            else {
                                if (!ep.need_store_) {
                                    ep.pid_man_.clear();
                                    ep.store_.clear();
                                }
                            }
                        },
                        [&](v5::basic_publish_packet<PacketIdBytes> const& p) {
                            if (ep.auto_pub_response_) {
                                switch (p.opts().qos()) {
                                case qos::at_least_once: {
                                    ep.send(
                                        v5::basic_puback_packet<PacketIdBytes>(p.packet_id()),
                                        [](system_error const&){}
                                    );
                                } break;
                                case qos::exactly_once: {
                                    ep.send(
                                        v5::basic_pubrec_packet<PacketIdBytes>(p.packet_id()),
                                        [](system_error const&){}
                                    );
                                } break;
                                default:
                                    break;
                                }
                            }
                        },
                        [&](v5::basic_puback_packet<PacketIdBytes> const& p) {
                            ep.store_.erase(response_packet::v5_puback, p.packet_id());
                            ep.pid_man_.release_id(p.packet_id());
                        },
                        [&](v5::basic_pubrec_packet<PacketIdBytes> const& p) {
                            ep.store_.erase(response_packet::v5_pubrec, p.packet_id());
                            if (ep.auto_pub_response_) {
                                ep.send(
                                    v5::basic_pubrel_packet<PacketIdBytes>(p.packet_id()),
                                    [](system_error const&){}
                                );
                            }
                        },
                        [&](v5::basic_pubrel_packet<PacketIdBytes> const& p) {
                            if (ep.auto_pub_response_) {
                                ep.send(
                                    v5::basic_pubcomp_packet<PacketIdBytes>(p.packet_id()),
                                    [](system_error const&){}
                                );
                            }
                        },
                        [&](v5::basic_pubcomp_packet<PacketIdBytes> const& p) {
                            ep.store_.erase(response_packet::v5_pubcomp, p.packet_id());
                            ep.pid_man_.release_id(p.packet_id());
                        },
                        [&](v5::basic_subscribe_packet<PacketIdBytes> const& p) {
                        },
                        [&](v5::basic_suback_packet<PacketIdBytes> const& p) {
                        },
                        [&](v5::basic_unsubscribe_packet<PacketIdBytes> const& p) {
                        },
                        [&](v5::basic_unsuback_packet<PacketIdBytes> const& p) {
                        },
                        [&](v5::pingreq_packet const& p) {
                        },
                        [&](v5::pingresp_packet const& p) {
                        },
                        [&](v5::disconnect_packet const& p) {
                        },
                        [&](v5::auth_packet const& p) {
                        },
                        [&](system_error const& e) {
                        }
                    }
                );
                self.complete(force_move(v));
            } break;
            }
        }
    };

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
};

template <typename NextLayer, role Role>
using endpoint = basic_endpoint<NextLayer, Role, 2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_ENDPOINT_HPP
