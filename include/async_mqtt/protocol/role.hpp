// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASYNC_MQTT_ROLE_HPP)
#define ASYNC_MQTT_ASYNC_MQTT_ROLE_HPP

namespace async_mqtt {

/**
 * @brief MQTT endpoint connection role
 *
 */
enum class role {
    client = 0b01, ///< as client. Can't send CONNACK, SUBACK, UNSUBACK, PINGRESP. Can send Other packets.
    server = 0b10, ///< as server. Can't send CONNECT, SUBSCRIBE, UNSUBSCRIBE, PINGREQ, DISCONNECT(only on v3.1.1).
                   ///  Can send Other packets.
    any    = 0b11, ///< can send all packets. (no check)
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_ASYNC_MQTT_ROLE_HPP
