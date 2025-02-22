// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_CONNECTION_STATUS_HPP)
#define ASYNC_MQTT_PROTOCOL_CONNECTION_STATUS_HPP

namespace async_mqtt {

/**
 * @brief Connection status.
 */
enum class connection_status {
    connecting,   ///< The status after sending or receiving a CONNECT packet,
                  ///< but before receiving or sending a CONNACK packet.
    connected,    ///< The status after sending or receiving a CONNACK packet.
    disconnected, ///< The status after sending or receiving a DISCONNECT packet,
                  ///< detecting an error, or when the underlying layer is closed.
};

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/impl/connection_status_impl.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PROTOCOL_CONNECTION_STATUS_HPP
