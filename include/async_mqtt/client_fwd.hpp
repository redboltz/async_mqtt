// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_CLIENT_FWD_HPP)
#define ASYNC_MQTT_CLIENT_FWD_HPP

#include <cstddef> // for std::size_t

#include <async_mqtt/role.hpp>
#include <async_mqtt/protocol/protocol_version.hpp>

namespace async_mqtt {

/**
 * @ingroup client
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
 * #### Requirements
 * @li Header: async_mqtt/client.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 * @tparam Version       MQTT protocol version.
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <protocol_version Version, typename NextLayer>
class client;

} // namespace async_mqtt

#endif // ASYNC_MQTT_CLIENT_FWD_HPP
