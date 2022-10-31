// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_SESSION_PRESENT_HPP)
#define ASYNC_MQTT_PACKET_SESSION_PRESENT_HPP


namespace async_mqtt {

constexpr bool is_session_present(char v) {
    return v & 0b00000001;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_SESSION_PRESENT_HPP
