// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_STORE_PACKET_VARIANT_FWD_HPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_STORE_PACKET_VARIANT_FWD_HPP

#include <cstddef>

/**
 * @defgroup store_packet_variant variant class for stored packets (PUBLISH, PUBREL)
 * @ingroup packet_variant
 */

/**
 * @defgroup store_packet_variant_detail implementation class
 * @ingroup store_packet_variant
 */

namespace async_mqtt {

/**
 * @ingroup store_packet_variant_detail
 * @brief MQTT packet variant for store
 * @tparam PacketIdBytes MQTT spec is 2. You can use `store_packet_variant` for that.
 *
 * #### variants
 * @li @ref v3_1_1::basic_publish_packet<PacketIdBytes>
 * @li @ref v3_1_1::basic_pubrel_packet<PacketIdBytes>
 * @li @ref v5::basic_publish_packet<PacketIdBytes>
 * @li @ref v5::basic_pubrel_packet<PacketIdBytes>
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/store_packet_variant.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
class basic_store_packet_variant;

/**
 * @ingroup store_packet_variant
 * @brief Type alias of basic_store_packet_variant (PacketIdBytes=2).
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/store_packet_variant.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
using store_packet_variant = basic_store_packet_variant<2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_PACKET_STORE_PACKET_VARIANT_FWD_HPP
