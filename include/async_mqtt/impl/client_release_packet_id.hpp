// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_RELEASE_PACKET_ID_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_RELEASE_PACKET_ID_HPP

#include <async_mqtt/client.hpp>
#include <async_mqtt/impl/client_impl.hpp>

namespace async_mqtt {

namespace detail {

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
auto
client_impl<Version, NextLayer>::async_release_packet_id(
    packet_id_type pid,
    CompletionToken&& token
) {
    return ep_.async_release_packet_id(pid, std::forward<CompletionToken>(token));
}

// sync version

template <protocol_version Version, typename NextLayer>
inline
void
client_impl<Version, NextLayer>::release_packet_id(packet_id_type packet_id) {
    ep_.release_packet_id(packet_id);
}

} // namespace detail

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
auto
client<Version, NextLayer>::async_release_packet_id(
    packet_id_type pid,
    CompletionToken&& token
) {
    BOOST_ASSERT(impl_);
    return impl_->async_release_packet_id(pid, std::forward<CompletionToken>(token));
}

// sync version

template <protocol_version Version, typename NextLayer>
inline
void
client<Version, NextLayer>::release_packet_id(packet_id_type packet_id) {
    BOOST_ASSERT(impl_);
    impl_->release_packet_id(packet_id);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_RELEASE_PACKET_ID_HPP
