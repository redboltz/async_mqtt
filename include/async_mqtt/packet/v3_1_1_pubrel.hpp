// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_PUBREL_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_PUBREL_HPP

#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/exception.hpp>

#include <async_mqtt/packet/control_packet_type.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/static_vector.hpp>


/**
 * @defgroup pubrel_v3_1_1 PUBREL packet (v3.1.1)
 * @ingroup packet_v3_1_1
 */

/**
 * @defgroup pubrel_v3_1_1_detail implementation class
 * @ingroup pubrel_v3_1_1
 */

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

/**
 * @ingroup pubrel_v3_1_1_detail
 * @brief MQTT PUBREL packet (v3.1.1)
 * @tparam PacketIdBytes size of packet_id
 *
 * If basic_endpoint::set_auto_pub_response() is called with true, then this packet is
 * automatically sent when PUBREC v3_1_1::basic_pubrec_packet is received.
 * If both the client and the broker keeping the session, this packet is
 * stored in the endpoint for resending if disconnect/reconnect happens.
 * If the session doesn' exist or lost, then the stored packets are erased.
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718053
 */
template <std::size_t PacketIdBytes>
class basic_pubrel_packet {
public:

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBREC packet
     */
    basic_pubrel_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id
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
    static constexpr std::size_t num_of_const_buffer_sequence();

    /**
     * @brief Get packet_id.
     * @return packet_id
     */
    typename basic_packet_id_type<PacketIdBytes>::type packet_id() const;

private:

    template <std::size_t PacketIdBytesArg>
    friend basic_packet_variant<PacketIdBytesArg>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v311_pubrel;
    friend struct ::ut_packet::v311_pubrel_pid4;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    basic_pubrel_packet(buffer buf);

private:
    static_vector<char, 2 + PacketIdBytes> all_;
};

/**
 * @related basic_pubrel_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 */
template <std::size_t PacketIdBytes>
std::ostream& operator<<(std::ostream& o, basic_pubrel_packet<PacketIdBytes> const& v);

/**
 * @ingroup pubrel_v3_1_1
 * @related basic_pubrel_packet
 * @brief Type alias of basic_pubrel_packet (PacketIdBytes=2).
 */
using pubrel_packet = basic_pubrel_packet<2>;

} // namespace async_mqtt::v3_1_1

#include <async_mqtt/packet/impl/v3_1_1_pubrel.hpp>

#endif // ASYNC_MQTT_PACKET_V3_1_1_PUBREL_HPP
