// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_CLOSE_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_CLOSE_HPP

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/asio_bind/impl/endpoint_impl.hpp>

namespace async_mqtt {

namespace detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::async_close(
    this_type_sp impl,
    CompletionToken&& token
) {
    BOOST_ASSERT(impl);
    return
        as::async_initiate<
            CompletionToken,
            void()
        >(
            [](
                auto handler,
                this_type_sp impl
            ) {
                async_close_impl(
                    force_move(impl),
                    force_move(handler)
                );
            },
            token,
            force_move(impl)
        );
}

} // namespace detail

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_close(CompletionToken&& token) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "close";
    BOOST_ASSERT(impl_);
    return
        as::async_initiate<
            CompletionToken,
            void()
        >(
            [](
                auto handler,
                std::shared_ptr<impl_type> impl
            ) {
                impl_type::async_close_impl(
                    force_move(impl),
                    force_move(handler)
                );
            },
            token,
            impl_
        );
}

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/asio_bind/impl/endpoint_close.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_ENDPOINT_CLOSE_HPP
