// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_QOS_UTIL_HPP)
#define ASYNC_MQTT_PACKET_QOS_UTIL_HPP

#include <async_mqtt/error.hpp>
#include <async_mqtt/protocol/packet/qos.hpp>

namespace async_mqtt {

/**
 * @ingroup suback_v3_1_1
 * @brief create suback_return_code corresponding to the QoS
 * @param q QoS
 * @return suback_retun_code
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/qos_util.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr suback_return_code qos_to_suback_return_code(qos q) {
    return static_cast<suback_return_code>(q);
}

/**
 * @ingroup suback_v5
 * @brief create suback_reason_code corresponding to the QoS
 * @param q QoS
 * @return suback_reason_code
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/qos_util.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr suback_reason_code qos_to_suback_reason_code(qos q) {
    return static_cast<suback_reason_code>(q);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_QOS_UTIL_HPP
