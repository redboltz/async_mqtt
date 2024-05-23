// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_HPP

#include <async_mqtt/client.hpp>

namespace async_mqtt {

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code, packet_id_type)
)
client<Version, NextLayer>::async_acquire_unique_packet_id(
    CompletionToken&& token
) {
    return ep_->async_acquire_unique_packet_id(std::forward<CompletionToken>(token));
}

// sync version

template <protocol_version Version, typename NextLayer>
inline
std::optional<packet_id_type>
client<Version, NextLayer>::acquire_unique_packet_id() {
    return ep_->acquire_unique_packet_id();
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_HPP
