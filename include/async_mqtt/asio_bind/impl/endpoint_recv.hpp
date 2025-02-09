// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_RECV_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_RECV_HPP

#include <async_mqtt/asio_bind/endpoint.hpp>
#include <async_mqtt/asio_bind/impl/endpoint_impl.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_recv(
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "recv";
    BOOST_ASSERT(impl_);
    return
        as::async_initiate<
            CompletionToken,
            void(error_code, std::optional<packet_variant_type>)
        >(
            [](
                auto handler,
                std::shared_ptr<impl_type> impl
            ) {
                impl_type::async_recv(
                    force_move(impl),
                    std::nullopt,
                    std::set<control_packet_type>{},
                    force_move(handler)
                );
            },
            token,
            impl_
        );
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_recv(
    std::set<control_packet_type> types,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "recv";
    BOOST_ASSERT(impl_);
    return
        as::async_initiate<
            CompletionToken,
            void(error_code, std::optional<packet_variant_type>)
        >(
            [](
                auto handler,
                std::shared_ptr<impl_type> impl,
                std::set<control_packet_type> types
            ) {
                impl_type::async_recv(
                    force_move(impl),
                    filter::match,
                    force_move(types),
                    force_move(handler)
                );
            },
            token,
            impl_,
            force_move(types)
        );
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_recv(
    filter fil,
    std::set<control_packet_type> types,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "recv";
    BOOST_ASSERT(impl_);
    return
        as::async_initiate<
            CompletionToken,
            void(error_code, std::optional<packet_variant_type>)
        >(
            [](
                auto handler,
                std::shared_ptr<impl_type> impl,
                filter fil,
                std::set<control_packet_type> types
            ) {
                impl_type::async_recv(
                    force_move(impl),
                    fil,
                    force_move(types),
                    force_move(handler)
                );
            },
            token,
            impl_,
            fil,
            force_move(types)
        );
}

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/asio_bind/impl/endpoint_recv.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RECV_HPP
