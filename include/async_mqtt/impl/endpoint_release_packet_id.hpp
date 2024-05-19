// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_RELEASE_PACKET_ID_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_RELEASE_PACKET_ID_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint<Role, PacketIdBytes, NextLayer>::
release_packet_id_op {
    this_type& ep;
    typename basic_packet_id_type<PacketIdBytes>::type packet_id;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        ep.release_pid(packet_id);
        self.complete();
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void()
)
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_release_packet_id(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "release_packet_id pid:" << packet_id;
    return
        as::async_compose<
            CompletionToken,
            void()
        >(
            release_packet_id_op{
                *this,
                packet_id
            },
            token
        );
}

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::
release_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "release_packet_id:" << packet_id;
    release_pid(packet_id);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RELEASE_PACKET_ID_HPP
