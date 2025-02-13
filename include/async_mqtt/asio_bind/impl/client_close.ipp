// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_CLOSE_IPP)
#define ASYNC_MQTT_IMPL_CLIENT_CLOSE_IPP

#include <async_mqtt/asio_bind/client.hpp>
#include <async_mqtt/asio_bind/impl/client_impl.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

template <protocol_version Version, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
client_impl<Version, NextLayer>::async_close(
    this_type_sp impl,
    as::any_completion_handler<
        void()
    > handler
) {
    impl->ep_.async_close(force_move(handler));
}

} // namespace async_mqtt::detail

#include <async_mqtt/asio_bind/impl/client_instantiate.hpp>

#endif // ASYNC_MQTT_IMPL_CLIENT_CLOSE_IPP
