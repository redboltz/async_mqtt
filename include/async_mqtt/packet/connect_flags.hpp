// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_CONNECT_FLAGS_HPP)
#define ASYNC_MQTT_CONNECT_FLAGS_HPP

#include <cstdint>
#include <async_mqtt/packet/pubopts.hpp>

namespace async_mqtt {

namespace connect_flags {

constexpr char const mask_clean_session  = 0b00000010;
constexpr char const mask_clean_start    = 0b00000010;
constexpr char const mask_will_flag      = 0b00000100;
constexpr char const mask_will_retain    = 0b00100000;
constexpr char const mask_password_flag  = 0b01000000;
constexpr char const mask_user_name_flag = static_cast<char>(0b10000000u);

constexpr bool has_clean_session(char v) {
    return (v & mask_clean_session) != 0;
}

constexpr bool has_clean_start(char v) {
    return (v & mask_clean_start) != 0;
}

constexpr bool has_will_flag(char v) {
    return (v & mask_will_flag) != 0;
}

constexpr pub::retain will_retain(char v) {
    return
        [&] {
            if (v & mask_will_retain) return pub::retain::yes;
            return pub::retain::no;
        }();
}

constexpr bool has_password_flag(char v) {
    return (v & mask_password_flag) != 0;
}

constexpr bool has_user_name_flag(char v) {
    return (v & mask_user_name_flag) != 0;
}

constexpr void set_will_qos(char& v, qos qos_value) {
    v |= static_cast<char>(static_cast<std::uint8_t>(qos_value) << 3);
}

constexpr qos will_qos(char v) {
    return static_cast<qos>((v & 0b00011000) >> 3);
}

} // namespace connect_flags

} // namespace async_mqtt

#endif // MQTT_CONNECT_FLAGS_HPP
