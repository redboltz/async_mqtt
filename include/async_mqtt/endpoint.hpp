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
#include <async_mqtt/packet_id_manager.hpp>
#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/buffer_to_packet_variant.hpp>

namespace async_mqtt {

enum class connection_mode { client, server };

template <typename NextLayer, std::size_t PacketIdBytes>
class basic_endpoint {
public:
    using this_type = basic_endpoint<NextLayer, PacketIdBytes>;
    using packet_id_type = typename packet_id_type<PacketIdBytes>::type;
    using stream_type = stream<NextLayer>;
    using strand_type = typename stream_type::strand_type;
    using variant_type = basic_packet_variant<PacketIdBytes>;
    template <typename... Args>
    basic_endpoint(
        connection_mode mode,
        protocol_version ver,
        Args&&... args
    ): mode_{mode},
       protocol_version_{ver},
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
    template <typename Packet>
    struct send_impl {
        this_type& ep;
        Packet packet;
        enum { initiate, complete } state = initiate;

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
            case initiate: {
                state = complete;
                auto cbs = packet.const_buffer_sequence();
                ep.stream_.write_packet(
                    force_move(cbs),
                    force_move(self)
                );
            } break;
            case complete:
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
                auto v = buffer_to_basic_packet_variant<PacketIdBytes>(buf, ep.protocol_version_);
                v.visit(
                    // do internal protocol processing
                    overload {
                        [&](async_mqtt::v3_1_1::connect_packet const& p) {
                        },
                        [&](async_mqtt::v3_1_1::publish_packet const& p) {
                            // if auto
                            //    if qos 1
                            //       send puback
                            //    if qos2
                            //       send pubrec
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
    optional<connection_mode> mode_;
    protocol_version protocol_version_;
    stream_type stream_;
    packet_id_manager<packet_id_type> pid_man_;
};

template <typename NextLayer>
using endpoint = basic_endpoint<NextLayer, 2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_ENDPOINT_HPP
