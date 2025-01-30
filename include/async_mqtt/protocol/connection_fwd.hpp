// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_CONNECTION_FWD_HPP)
#define ASYNC_MQTT_PROTOCOL_CONNECTION_FWD_HPP

#include <cstddef> // for std::size_t

#include <async_mqtt/protocol/role.hpp>

namespace async_mqtt {

/**
 * @brief MQTT connection
 *
 * I/O independent MQTT protocol state machine.
 * @li Manage connection status.
 * @li Manage packet identifier.
 * @li Manage Topic Alias.
 * @li Manage resending packets on reconnection.
 * @li Manage PINGREQ/PINGRESP timers.
 * @li Manage automatic sending response packets.
 *
 * All requests and settings are implemented as synchronous functions.
 * When caller's action is required, virtual function that is corresponding to
 * the action is called **in the synchronous function before the function
 * returns**.
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * @tparam Role          role for packet sendable checking
 * @tparam PacketIdBytes MQTT spec is 2. You can use `connection` for that.
 */
template <role Role, std::size_t PacketIdBytes>
class basic_connection;

/**
 * @related basic_connection
 * @brief Type alias of basic_connection (PacketIdBytes=2).
 *        This is for typical usecase (e.g. MQTT client).
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * @tparam Role          role for packet sendable checking
 */
template <role Role>
using connection = basic_connection<Role, 2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_CONNECTION_FWD_HPP
