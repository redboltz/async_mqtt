// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASIO_BIND_IMPL_ENDPOINT_GET_STORED_PACKETS_HPP)
#define ASYNC_MQTT_ASIO_BIND_IMPL_ENDPOINT_GET_STORED_PACKETS_HPP

#include <async_mqtt/asio_bind/endpoint.hpp>
#include <async_mqtt/asio_bind/impl/endpoint_impl.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_get_stored_packets(
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "get_stored_packets";
    BOOST_ASSERT(impl_);
    return
        as::async_initiate<
            CompletionToken,
            void(error_code, std::vector<basic_store_packet_variant<PacketIdBytes>>)
        >(
            [](
                auto handler,
                std::shared_ptr<impl_type> impl
            ) {
                impl_type::async_get_stored_packets(
                    force_move(impl),
                    force_move(handler)
                );
            },
            token,
            impl_
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
    BOOST_ASSERT(impl_);
    return impl_->get_stored_packets();
}

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/asio_bind/impl/endpoint_get_stored_packets.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_ASIO_BIND_IMPL_ENDPOINT_GET_STORED_PACKETS_HPP
