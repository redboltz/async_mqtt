// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_PINGREQ_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_PINGREQ_HPP

#include <async_mqtt/buffer_to_packet_variant.hpp>

#include <async_mqtt/packet/control_packet_type.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/static_vector.hpp>


/**
 * @defgroup pingreq_v3_1_1 PINGREQ packet (v3.1.1)
 * @ingroup packet_v3_1_1
 */

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

/**
 * @ingroup pingreq_v3_1_1
 * @brief MQTT PINGREQ packet (v3.1.1)
 *
 * Only MQTT client can send this packet.
 * This packet is to notify the client is living.
 * When you set keep_alive_sec > 0 on CONNECT packet (v3_1_1::connect_packet),
 * then PINGREQ packet is automatically sent after keep_alive_sec passed after the last packet sent.
 * When the broker receives this packet, KeepAlive timeout (keep_alive_sec * 1.5) is reset and
 * start counting from 0.
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718081
 *
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 * #### Requirements
 * - Header: async_mqtt/packet/v3_1_1_pingreq.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
class pingreq_packet {
public:

    /**
     * @brief constructor
     */
    explicit pingreq_packet();

    /**
     * @brief Get MQTT control packet type
     * @return control packet type
     */
    static constexpr control_packet_type type() {
        return control_packet_type::pingreq;
    }

    /**
     * @brief Create const buffer sequence
     *        it is for boost asio APIs
     * @return const buffer sequence
     */
    std::vector<as::const_buffer> const_buffer_sequence() const;

    /**
     * @brief Get packet size.
     * @return packet size
     */
    std::size_t size() const;

    /**
     * @brief Get number of element of const_buffer_sequence
     * @return number of element of const_buffer_sequence
     */
    static constexpr std::size_t num_of_const_buffer_sequence() {
        return 1; // all
    }

private:

    template <std::size_t PacketIdBytesArg>
    friend basic_packet_variant<PacketIdBytesArg>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver, error_code& ec);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v311_pingreq;
    friend struct ::ut_packet::v311_pingreq_error;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    explicit pingreq_packet(buffer buf, error_code& ec);

private:
    static_vector<char, 2> all_;
};

/**
 * @related pingreq_packet
 * @brief less than operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs less than the rhs, otherwise false.
 *
 * #### Requirements
 * - Header: async_mqtt/packet/v3_1_1_pingreq.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
bool operator<(pingreq_packet const& lhs, pingreq_packet const& rhs);

/**
 * @related pingreq_packet
 * @brief equal operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs equal to the rhs, otherwise false.
 *
 * #### Requirements
 * - Header: async_mqtt/packet/v3_1_1_pingreq.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
bool operator==(pingreq_packet const& lhs, pingreq_packet const& rhs);

/**
 * @related pingreq_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * - Header: async_mqtt/packet/v3_1_1_pingreq.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, pingreq_packet const& v);

} // namespace async_mqtt::v3_1_1

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/packet/impl/v3_1_1_pingreq.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PACKET_V3_1_1_PINGREQ_HPP
