// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_WAIT_UNTIL_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_WAIT_UNTIL_HPP

#include <async_mqtt/client.hpp>
#include <async_mqtt/impl/client_impl.hpp>

namespace async_mqtt {

namespace detail {

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
auto
client_impl<Version, NextLayer>::async_acquire_unique_packet_id_wait_until(
    CompletionToken&& token
) {
    return ep_.async_acquire_unique_packet_id_wait_until(std::forward<CompletionToken>(token));
}

} // namespace detail

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
auto
client<Version, NextLayer>::async_acquire_unique_packet_id_wait_until(
    CompletionToken&& token
) {
    BOOST_ASSERT(impl_);
    return impl_->async_acquire_unique_packet_id_wait_until(std::forward<CompletionToken>(token));
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_WAIT_UNTIL_HPP
