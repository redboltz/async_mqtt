// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_PINGRESP_HPP)
#define ASYNC_MQTT_PACKET_V5_PINGRESP_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/error.hpp>

#include <async_mqtt/packet/control_packet_type.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>


/**
 * @defgroup pingresp_v5 PINGRESP packet (v5.0)
 * @ingroup packet_v5
 */

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @ingroup pingresp_v5
 * @brief MQTT PINGRESP packet (v5)
 *
 * Only MQTT broker(sever) can send this packet.
 * If basic_endpoint::set_auto_ping_response() is called with true, then this packet is
 * automatically sent when PINGREQ v5::pingreq_packet is received.
 * If the endpoint called basic_endpoint::set_pingresp_recv_timeout() with non 0 argument,
 * and PINGRESP packet isn't received until the timer fired, then send DISCONNECT packet with
 * the reason code disconnect_reason_code::keep_alive_timeout automatically then close underlying
 * layer automatically.
 * \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901200"></a>
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v5_pingresp.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class pingresp_packet {
public:

    /**
     * @brief constructor
     */
    explicit pingresp_packet();

    /**
     * @brief Get MQTT control packet type
     * @return control packet type
     */
    static constexpr control_packet_type type() {
        return control_packet_type::pingresp;
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
    friend struct ::ut_packet::v5_pingresp;
    friend struct ::ut_packet::v5_pingresp_error;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    explicit pingresp_packet(buffer buf, error_code& ec);

private:
    static_vector<char, 2> all_;
};

/**
 * @related pingresp_packet
 * @brief less than operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs less than the rhs, otherwise false.
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v5_pingresp.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
bool operator<(pingresp_packet const& lhs, pingresp_packet const& rhs);

/**
 * @related pingresp_packet
 * @brief equal operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs equal to the rhs, otherwise false.
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v5_pingresp.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
bool operator==(pingresp_packet const& lhs, pingresp_packet const& rhs);

/**
 * @related pingresp_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v5_pingresp.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, pingresp_packet const& v);

} // namespace async_mqtt::v5

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/packet/impl/v5_pingresp.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PACKET_V5_PINGRESP_HPP
