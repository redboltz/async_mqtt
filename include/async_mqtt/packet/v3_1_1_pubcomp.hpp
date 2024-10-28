// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_PUBCOMP_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_PUBCOMP_HPP

#include <async_mqtt/buffer_to_packet_variant.hpp>

#include <async_mqtt/packet/control_packet_type.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/static_vector.hpp>


/**
 * @defgroup pubcomp_v3_1_1 PUBCOMP packet (v3.1.1)
 * @ingroup packet_v3_1_1
 */

/**
 * @defgroup pubcomp_v3_1_1_detail implementation class
 * @ingroup pubcomp_v3_1_1
 */

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

/**
 * @ingroup pubcomp_v3_1_1_detail
 * @brief MQTT PUBCOMP packet (v3.1.1)
 * @tparam PacketIdBytes size of packet_id
 *
 * If basic_endpoint::set_auto_pub_response() is called with true, then this packet is
 * automatically sent when PUBREL v3_1_1::basic_pubrel_packet is received.
 *
 * When the packet is received, the packet_id is automatically released and become reusable.
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718058
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v3_1_1_pubcomp.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
class basic_pubcomp_packet {
public:

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBREL packet
     */
    explicit basic_pubcomp_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id
    );

    /**
     * @brief Get MQTT control packet type
     * @return control packet type
     */
    static constexpr control_packet_type type() {
        return control_packet_type::pubcomp;
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

    /**
     * @brief Get packet_id.
     * @return packet_id
     */
    typename basic_packet_id_type<PacketIdBytes>::type packet_id() const;

private:

    template <std::size_t PacketIdBytesArg>
    friend std::optional<basic_packet_variant<PacketIdBytesArg>>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver, error_code& ec);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v311_pubcomp;
    friend struct ::ut_packet::v311_pubcomp_pid4;
    friend struct ::ut_packet::v311_pubcomp_error;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    explicit basic_pubcomp_packet(buffer buf, error_code& ec);

private:
    boost::container::static_vector<char, 2 + PacketIdBytes> all_;
};

/**
 * @related basic_pubcomp_packet
 * @brief less than operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs less than the rhs, otherwise false.
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v3_1_1_pubcomp.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
bool operator<(basic_pubcomp_packet<PacketIdBytes> const& lhs, basic_pubcomp_packet<PacketIdBytes> const& rhs);

/**
 * @related basic_pubcomp_packet
 * @brief equal operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs equal to the rhs, otherwise false.
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v3_1_1_pubcomp.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
bool operator==(basic_pubcomp_packet<PacketIdBytes> const& lhs, basic_pubcomp_packet<PacketIdBytes> const& rhs);

/**
 * @related basic_pubcomp_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v3_1_1_pubcomp.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
std::ostream& operator<<(std::ostream& o, basic_pubcomp_packet<PacketIdBytes> const& v);

/**
 * @ingroup pubcomp_v3_1_1
 * @related basic_pubcomp_packet
 * @brief Type alias of basic_pubcomp_packet (PacketIdBytes=2).
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v3_1_1_pubcomp.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
using pubcomp_packet = basic_pubcomp_packet<2>;

} // namespace async_mqtt::v3_1_1

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/packet/impl/v3_1_1_pubcomp.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PACKET_V3_1_1_PUBCOMP_HPP
