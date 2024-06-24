// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ENDPOINT_FWD_HPP)
#define ASYNC_MQTT_ENDPOINT_FWD_HPP

#include <cstddef> // for std::size_t

#include <async_mqtt/role.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
class basic_endpoint;

/**
 * @ingroup endpoint
 * @related basic_endpoint
 * @brief Type alias of basic_endpoint (PacketIdBytes=2).
 *        This is for typical usecase.
 * @tparam Role          role for packet sendable checking
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <role Role, typename NextLayer>
using endpoint = basic_endpoint<Role, 2, NextLayer>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_ENDPOINT_FWD_HPP
