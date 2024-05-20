// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_CONNACK_HPP)
#define ASYNC_MQTT_PACKET_V5_CONNACK_HPP

#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/exception.hpp>

#include <async_mqtt/packet/control_packet_type.hpp>
#include <async_mqtt/packet/reason_code.hpp>
#include <async_mqtt/packet/property_variant.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/static_vector.hpp>

/**
 * @defgroup connack_v5 CONNACK packet (v5.0)
 * @ingroup packet_v5
 */

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @ingroup connack_v5
 * @brief MQTT CONNACK packet (v5)
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 *
 * Only MQTT broker(sever) can send this packet.
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901074
 */
class connack_packet {
public:

    /**
     * @brief constructor
     * @param session_present If the broker stores the session, then true, otherwise false.
     *                        When the endpoint receives CONNACK packet with session_present is false,
     *                        then stored packets are erased.
     * @param reason_code ConnectReasonCode
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901079
     * @param props       properties.
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901080
     */
    connack_packet(
        bool session_present,
        connect_reason_code reason_code,
        properties props = {}
    );

    /**
     * @brief Get MQTT control packet type
     * @return control packet type
     */
    static constexpr control_packet_type type();

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
    std::size_t num_of_const_buffer_sequence() const;

    /**
     * @brief Get session present
     * @return true if the session is present, otherwise false
     */
    bool session_present() const;

    /**
     * @brief Get reason code
     * @return reason_code
     */
    connect_reason_code code() const;

    /**
     * @brief Get properties
     * @return properties
     */
    properties const& props() const;

private:

    template <std::size_t PacketIdBytesArg>
    friend basic_packet_variant<PacketIdBytesArg>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v5_connack;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    connack_packet(buffer buf);

private:
    std::uint8_t fixed_header_;

    std::size_t remaining_length_;
    boost::container::static_vector<char, 4> remaining_length_buf_;

    std::uint8_t connect_acknowledge_flags_;

    connect_reason_code reason_code_;

    std::size_t property_length_;
    boost::container::static_vector<char, 4> property_length_buf_;
    properties props_;
};

/**
 * @related connack_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 */
std::ostream& operator<<(std::ostream& o, connack_packet const& v);

} // namespace async_mqtt::v5

#include <async_mqtt/packet/impl/v5_connack.hpp>

#endif // ASYNC_MQTT_PACKET_V5_CONNACK_HPP
