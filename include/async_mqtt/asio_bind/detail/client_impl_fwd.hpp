// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_DETAIL_CLIENT_IMPL_FWD_HPP)
#define ASYNC_MQTT_DETAIL_CLIENT_IMPL_FWD_HPP

#include <async_mqtt/protocol/protocol_version.hpp>

namespace async_mqtt::detail {

template <protocol_version Version, typename NextLayer>
class client_impl;

} // namespace async_mqtt::detail

#endif // ASYNC_MQTT_DETAIL_CLIENT_IMPL_FWD_HPP
