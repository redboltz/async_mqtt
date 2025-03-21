// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASIO_BIND_CLIENT_FWD_HPP)
#define ASYNC_MQTT_ASIO_BIND_CLIENT_FWD_HPP

#include <cstddef> // for std::size_t

#include <async_mqtt/protocol/role.hpp>
#include <async_mqtt/protocol/protocol_version.hpp>

namespace async_mqtt {

/**
 * @brief MQTT client for casual usecases
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### predefined next layer types for NextLayer:
 *    @li @ref protocol::mqtt
 *    @li @ref protocol::mqtts
 *    @li @ref protocol::ws
 *    @li @ref protocol::wss
 *
 * @tparam Version       MQTT protocol version.
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <protocol_version Version, typename NextLayer>
class client;

} // namespace async_mqtt

#endif // ASYNC_MQTT_ASIO_BIND_CLIENT_FWD_HPP
