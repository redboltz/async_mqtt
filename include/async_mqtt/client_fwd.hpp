// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_CLIENT_FWD_HPP)
#define ASYNC_MQTT_CLIENT_FWD_HPP

#include <cstddef> // for std::size_t

#include <async_mqtt/role.hpp>
#include <async_mqtt/protocol_version.hpp>

namespace async_mqtt {

/**
 * @ingroup client
 * @brief MQTT client for casual usecases
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 * #### Internal type aliases
 * - connect_packet
 *   - connect_packet of the Version
 *     - v3_1_1::connect_packet
 *     - v5::connect_packet
 * - connack_packet
 *   - connack_packet of the Version
 *     - v3_1_1::connack_packet
 *     - v5::connack_packet
 * - subscribe_packet
 *   - subscribe_packet of the Version
 *     - @ref v3_1_1::basic_subscribe_packet "v3_1_1::subscribe_packet"
 *     - @ref v5::basic_subscribe_packet "v5::subscribe_packet"
 * - suback_packet
 *   - suback_packet of the Version
 *     - @ref v3_1_1::basic_suback_packet "v3_1_1::suback_packet"
 *     - @ref v5::basic_suback_packet "v5::suback_packet"
 * - unsubscribe_packet
 *   - unsubscribe_packet of the Version
 *     - @ref v3_1_1::basic_unsubscribe_packet "v3_1_1::unsubscribe_packet"
 *     - @ref v5::basic_unsubscribe_packet "v5::unsubscribe_packet"
 * - unsuback_packet
 *   - unsuback_packet of the Version
 *     - @ref v3_1_1::basic_unsuback_packet "v3_1_1::unsuback_packet"
 *     - @ref v5::basic_unsuback_packet "v5::unsuback_packet"
 * - publish_packet
 *   - publish_packet of the Version
 *     - @ref v3_1_1::basic_publish_packet "v3_1_1::publish_packet"
 *     - @ref v5::basic_publish_packet "v5::publish_packet"
 * - puback_packet
 *   - puback_packet of the Version
 *     - @ref v3_1_1::basic_puback_packet "v3_1_1::puback_packet"
 *     - @ref v5::basic_puback_packet "v5::puback_packet"
 * - pubrec_packet
 *   - pubrec_packet of the Version
 *     - @ref v3_1_1::basic_pubrec_packet "v3_1_1::pubrec_packet"
 *     - @ref v5::basic_pubrec_packet "v5::pubrec_packet"
 * - pubrel_packet
 *   - pubrel_packet of the Version
 *     - @ref v3_1_1::basic_pubrel_packet "v3_1_1::pubrel_packet"
 *     - @ref v5::basic_pubrel_packet "v5::pubrel_packet"
 * - pubcomp_packet
 *   - pubcomp_packet of the Version
 *     - @ref v3_1_1::basic_pubcomp_packet "v3_1_1::pubcomp_packet"
 *     - @ref v5::basic_pubcomp_packet "v5::pubcomp_packet"
 * - disconnect_packet
 *   - disconnect_packet of the Version
 *     - v3_1_1::disconnect_packet
 *     - v5::disconnect_packet
 *
 * #### Requirements
 * - Header: async_mqtt/client.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 * @tparam Version       MQTT protocol version.
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <protocol_version Version, typename NextLayer>
class client;

} // namespace async_mqtt

#endif // ASYNC_MQTT_CLIENT_FWD_HPP
