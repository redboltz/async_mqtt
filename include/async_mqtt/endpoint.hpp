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
        Args&&... args
    ): mode_{mode},
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
            std::is_invocable<CompletionToken, sys::error_code, std::size_t>::value
        >* = nullptr
    >
    auto send_packet(
        Packet packet,
        CompletionToken&& token
    ) {
        stream_.write_packet(
            std::forward<Packet>(packet),
            std::forward<CompletionToken>(token)
        );
    }

    // recv and check protocol and call CompToken with variant_type
    void recv(variant_type const& pv) {
        pv.visit(
            overload {
                [&](async_mqtt::v3_1_1::publish_packet const& p) {
                    // if auto
                    //    if qos 1
                    //       send puback
                    //    if qos2
                    //       send pubrec
                }
            }
        );
    }

private:
    connection_mode mode_;
    stream_type stream_;
    packet_id_manager<packet_id_type> pid_man_;
};

template <typename NextLayer>
using endpoint = basic_endpoint<NextLayer, 2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_ENDPOINT_HPP
