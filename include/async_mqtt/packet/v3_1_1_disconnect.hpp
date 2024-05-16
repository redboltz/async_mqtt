// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_DISCONNECT_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_DISCONNECT_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/packet/buffer_to_packet_variant_fwd.hpp>
#include <async_mqtt/exception.hpp>
#include <async_mqtt/util/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>

#include <async_mqtt/packet/fixed_header.hpp>

/**
 * @defgroup disconnect_v3_1_1
 * @ingroup packet_v3_1_1
 * @brief DISCONNECT packet (v3.1.1)
 */

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

/**
 * @ingroup disconnect_v3_1_1
 * @brief MQTT DISCONNECT packet (v3.1.1)
 *
 * Only MQTT client can send this packet.
 * When the endpoint sends DISCONNECT packet, then the endpoint become disconnecting status.
 * The endpoint can't send packets any more.
 * The underlying layer is not automatically closed from the client side.
 * If you want to close the underlying layer from the client side, you need to call basic_endpoint::close()
 * after sending DISCONNECT packet.
 * When the broker receives DISCONNECT packet, then close underlying layer from the broker.
 * In this case, Will is not published by the broker.
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718090
 */
class disconnect_packet {
public:
    /**
     * @brief constructor
     */
    disconnect_packet();

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

private:

    template <std::size_t PacketIdBytesArg>
    friend basic_packet_variant<PacketIdBytesArg>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v311_disconnect;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    disconnect_packet(buffer buf);

private:
    static_vector<char, 2> all_;
};

/**
 * @related disconnect_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 */
std::ostream& operator<<(std::ostream& o, disconnect_packet const& /*v*/);

} // namespace async_mqtt::v3_1_1

#include <async_mqtt/packet/impl/v3_1_1_disconnect.hpp>

#endif // ASYNC_MQTT_PACKET_V3_1_1_DISCONNECT_HPP
