// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_SUBACK_RETURN_CODE_HPP)
#define ASYNC_MQTT_PACKET_SUBACK_RETURN_CODE_HPP

#include <cstdint>
#include <ostream>

#include <async_mqtt/packet/qos.hpp>

/// @file

namespace async_mqtt {

/**
 * @brief MQTT suback_return_code
 *
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718071
 */
enum class suback_return_code : std::uint8_t {
    success_maximum_qos_0                  = 0x00, ///< Success with QoS0
    success_maximum_qos_1                  = 0x01, ///< Success with QoS1
    success_maximum_qos_2                  = 0x02, ///< Success with QoS2
    failure                                = 0x80, ///< Fail
};

/**
 * @brief create suback_return_code from qos
 * @param q qos
 * @return suback_retun_code
 */
constexpr suback_return_code qos_to_suback_return_code(qos q) {
    return static_cast<suback_return_code>(q);
}

/**
 * @brief stringize suback_return_code
 */
constexpr
char const* suback_return_code_to_str(suback_return_code v) {
    switch(v)
    {
    case suback_return_code::success_maximum_qos_0: return "success_maximum_qos_0";
    case suback_return_code::success_maximum_qos_1: return "success_maximum_qos_1";
    case suback_return_code::success_maximum_qos_2: return "success_maximum_qos_2";
    case suback_return_code::failure:               return "failure";
    default:                                        return "unknown_suback_return_code";
    }
}

/**
 * @related opts
 * @brief output to the stream suback_return_code
 */
inline
std::ostream& operator<<(std::ostream& os, suback_return_code val)
{
    os << suback_return_code_to_str(val);
    return os;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_SUBACK_RETURN_CODE_HPP
