// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_GET_STORED_PACKETS_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_GET_STORED_PACKETS_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint<Role, PacketIdBytes, NextLayer>::
get_stored_packets_op {
    this_type const& ep;
    std::vector<basic_store_packet_variant<PacketIdBytes>> packets = {};

    template <typename Self>
    void operator()(
        Self& self
    ) {
        packets = ep.get_stored_packets();
        self.complete(force_move(packets));
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(std::vector<basic_store_packet_variant<PacketIdBytes>>)
)
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_get_stored_packets(
    CompletionToken&& token
) const {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "get_stored_packets";
    return
        as::async_compose<
            CompletionToken,
            void(std::vector<basic_store_packet_variant<PacketIdBytes>>)
        >(
            get_stored_packets_op{
                *this
            },
            token
        );
}

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
std::vector<basic_store_packet_variant<PacketIdBytes>>
basic_endpoint<Role, PacketIdBytes, NextLayer>::get_stored_packets() const {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "get_stored_packets";
    return store_.get_stored();
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_GET_STORED_PACKETS_HPP
