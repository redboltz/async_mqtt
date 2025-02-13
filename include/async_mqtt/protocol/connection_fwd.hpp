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
 * @brief MQTT connection.
 *
 * An I/O-independent MQTT protocol state machine.
 * @li Manages connection status.
 * @li Manages packet identifiers.
 * @li Manages Topic Aliases.
 * @li Manages packet resending on reconnection.
 * @li Manages PINGREQ/PINGRESP timers.
 * @li Manages automatic response packet sending.
 *
 * All requests and settings are implemented as synchronous functions.
 * When a caller action is required, the corresponding virtual function
 * is called **within the synchronous function before it returns**.
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * @tparam Role          The role used for checking whether packets can be sent.
 * @tparam PacketIdBytes The number of bytes used for the packet identifier.
 *                       According to the MQTT specification, this is 2.
 *                       You can use @ref connection for this.
 */
template <role Role, std::size_t PacketIdBytes>
class basic_connection;

/**
 * @brief Type alias for @ref basic_connection with PacketIdBytes set to 2.
 *        This is for typical use cases (e.g., MQTT client).
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * @tparam Role The role used for checking whether packets can be sent.
 */
template <role Role>
using connection = basic_connection<Role, 2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_CONNECTION_FWD_HPP
