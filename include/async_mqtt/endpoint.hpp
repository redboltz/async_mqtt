// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ENDPOINT_HPP)
#define ASYNC_MQTT_ENDPOINT_HPP

#include <deque>

#include <async_mqtt/packet/packet_variant.hpp>
#include <async_mqtt/util/value_allocator.hpp>
#include <async_mqtt/stream.hpp>
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
            std::is_invocable<CompletionToken, error_code>::value
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
                void(error_code)
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


private:

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
            case write:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                state = complete;
                ep.stream_.write_packet(
                    force_move(packet),
                    force_move(self)
                );
                break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                self.complete(ec);
                break;
            }
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
                                ep.pid_man_.clear();
                                ep.store_.clear();
                            }
                        },
                        [&](v3_1_1::basic_publish_packet<PacketIdBytes> const& p) {
                            if (ep.auto_pub_response_) {
                                switch (p.opts().qos()) {
                                case qos::at_least_once: {
                                    ep.send(
                                        v3_1_1::basic_puback_packet<PacketIdBytes>(p.packet_id()),
                                        [](error_code const&){}
                                    );
                                } break;
                                case qos::exactly_once: {
                                    ep.send(
                                        v3_1_1::basic_pubrec_packet<PacketIdBytes>(p.packet_id()),
                                        [](error_code const&){}
                                    );
                                } break;
                                default:
                                    break;
                                }
                            }
                        },
                        [&](v3_1_1::basic_puback_packet<PacketIdBytes> const& p) {
                            ep.pid_man_.release_id(p.packet_id());
                        },
                        [&](v3_1_1::basic_pubrec_packet<PacketIdBytes> const& p) {
                            if (ep.auto_pub_response_) {
                                ep.send(
                                    v3_1_1::basic_pubrel_packet<PacketIdBytes>(p.packet_id()),
                                    [](error_code const&){}
                                );
                            }
                        },
                        [&](v3_1_1::basic_pubrel_packet<PacketIdBytes> const& p) {
                            if (ep.auto_pub_response_) {
                                ep.send(
                                    v3_1_1::basic_pubcomp_packet<PacketIdBytes>(p.packet_id()),
                                    [](error_code const&){}
                                );
                            }
                        },
                        [&](v3_1_1::basic_pubcomp_packet<PacketIdBytes> const& p) {
                            ep.pid_man_.release_id(p.packet_id());
                        },
                        [&](v3_1_1::basic_subscribe_packet<PacketIdBytes> const& p) {
                        },
                        [&](v5::connect_packet const& p) {
                        },
                        [&](v5::basic_publish_packet<PacketIdBytes> const& p) {
                            // if auto
                            //    if qos 1
                            //       send puback
                            //    if qos2
                            //       send pubrec
                        },
                        [&](v5::basic_pubrel_packet<PacketIdBytes> const& p) {
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
    std::deque<packet_variant> store_;
    bool auto_pub_response_ = false;
};

template <typename NextLayer, role Role>
using endpoint = basic_endpoint<NextLayer, Role, 2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_ENDPOINT_HPP
