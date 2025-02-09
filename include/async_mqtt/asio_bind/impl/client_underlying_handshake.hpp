// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_UNDERLYING_HANDSHAKE_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_UNDERLYING_HANDSHAKE_HPP

#include <async_mqtt/asio_bind/client.hpp>
#include <async_mqtt/asio_bind/impl/client_impl.hpp>

namespace async_mqtt {

namespace detail {

template <protocol_version Version, typename NextLayer>
template <typename... Args>
auto
client_impl<Version, NextLayer>::async_underlying_handshake(
    Args&&... args
) {
    return ep_.async_underlying_handshake(std::forward<Args>(args)...);
}

} // namespace detail

template <protocol_version Version, typename NextLayer>
template <typename... Args>
auto
client<Version, NextLayer>::async_underlying_handshake(
    Args&&... args
) {
    BOOST_ASSERT(impl_);
    return impl_->async_underlying_handshake(std::forward<Args>(args)...);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_UNDERLYING_HANDSHAKE_HPP
