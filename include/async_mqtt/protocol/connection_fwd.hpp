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
 * @ingroup connection
 * @brief MQTT connection
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/connection.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 * @tparam Role          role for packet sendable checking
 * @tparam PacketIdBytes MQTT spec is 2. You can use `connection` for that.
 */
template <role Role, std::size_t PacketIdBytes>
class basic_connection;

/**
 * @ingroup connection
 * @related basic_connection
 * @brief Type alias of basic_connection (PacketIdBytes=2).
 *        This is for typical usecase.
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/connection.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 * @tparam Role          role for packet sendable checking
 */
template <role Role>
using connection = basic_connection<Role, 2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_CONNECTION_FWD_HPP
