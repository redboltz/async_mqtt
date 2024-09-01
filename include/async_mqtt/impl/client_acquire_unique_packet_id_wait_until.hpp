// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_WAIT_UNTIL_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_WAIT_UNTIL_HPP

#include <async_mqtt/client.hpp>

namespace async_mqtt {

namespace detail {

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code, packet_id_type)
)
client_impl<Version, NextLayer>::async_acquire_unique_packet_id_wait_until(
    CompletionToken&& token
) {
    return ep_.async_acquire_unique_packet_id_wait_until(std::forward<CompletionToken>(token));
}

} // namespace detail

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code, packet_id_type)
)
client<Version, NextLayer>::async_acquire_unique_packet_id_wait_until(
    CompletionToken&& token
) {
    BOOST_ASSERT(impl_);
    return impl_->async_acquire_unique_packet_id_wait_until(std::forward<CompletionToken>(token));
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_WAIT_UNTIL_HPP
