// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_PUBREL_HPP)
#define ASYNC_MQTT_PACKET_V5_PUBREL_HPP

#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/error.hpp>

#include <async_mqtt/packet/control_packet_type.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/property_variant.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/static_vector.hpp>

/**
 * @defgroup pubrel_v5 PUBREL packet (v5.0)
 * @ingroup packet_v5
 */

/**
 * @defgroup pubrel_v5_detail implementation class
 * @ingroup pubrel_v5
 */

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @ingroup pubrel_v5_detail
 * @brief MQTT PUBREL packet (v5)
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 * @tparam PacketIdBytes size of packet_id
 *
 * If basic_endpoint::set_auto_pub_response() is called with true, then this packet is
 * automatically sent when PUBREC v5::basic_pubrec_packet is received.
 * If both the client and the broker keeping the session, this packet is
 * stored in the endpoint for resending if disconnect/reconnect happens.
 * If the session doesn' exist or lost, then the stored packets are erased.
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901141
 */
template <std::size_t PacketIdBytes>
class basic_pubrel_packet {
public:

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBREC packet
     * @param reason_code PubrelReasonCode
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901144
     * @param props       properties.
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901145
     */
    explicit basic_pubrel_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        pubrel_reason_code reason_code,
        properties props
    );

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBREC packet
     */
    explicit basic_pubrel_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id
    );

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBREC packet
     * @param reason_code PubrelReasonCode
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901144
     */
    explicit basic_pubrel_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        pubrel_reason_code reason_code
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
    constexpr std::size_t num_of_const_buffer_sequence() const;

    /**
     * @brief Get packet_id.
     * @return packet_id
     */
    typename basic_packet_id_type<PacketIdBytes>::type packet_id() const;

    /**
     * @brief Get reason code
     * @return reason_code
     */
    pubrel_reason_code code() const;

    /**
     * @brief Get properties
     * @return properties
     */
    properties const& props() const;

    /**
     * @brief stream output operator
     * @param o output stream
     * @param v target
     * @return  output stream
     */
    friend
    inline std::ostream& operator<<(std::ostream& o, basic_pubrel_packet<PacketIdBytes> const& v) {
        o <<
            "v5::pubrel{" <<
            "pid:" << v.packet_id();
        if (v.reason_code_) {
            o << ",rc:" << *v.reason_code_;
        }
        if (!v.props().empty()) {
            o << ",ps:" << v.props();
        };
        o << "}";
        return o;
    }

private:
    explicit basic_pubrel_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        std::optional<pubrel_reason_code> reason_code,
        properties props
    );

private:

    template <std::size_t PacketIdBytesArg>
    friend basic_packet_variant<PacketIdBytesArg>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver, error_code& ec);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v5_pubrel;
    friend struct ::ut_packet::v5_pubrel_pid4;
    friend struct ::ut_packet::v5_pubrel_pid_only;
    friend struct ::ut_packet::v5_pubrel_pid_rc;
    friend struct ::ut_packet::v5_pubrel_prop_len_last;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    explicit basic_pubrel_packet(buffer buf, error_code& ec);

private:
    std::uint8_t fixed_header_;
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;
    static_vector<char, PacketIdBytes> packet_id_;

    std::optional<pubrel_reason_code> reason_code_;

    std::size_t property_length_ = 0;
    static_vector<char, 4> property_length_buf_;
    properties props_;
};

/**
 * @related basic_pubrel_packet
 * @brief less than operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs less than the rhs, otherwise false.
 */
template <std::size_t PacketIdBytes>
bool operator<(basic_pubrel_packet<PacketIdBytes> const& lhs, basic_pubrel_packet<PacketIdBytes> const& rhs);

/**
 * @related basic_pubrel_packet
 * @brief equal operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs equal to the rhs, otherwise false.
 */
template <std::size_t PacketIdBytes>
bool operator==(basic_pubrel_packet<PacketIdBytes> const& lhs, basic_pubrel_packet<PacketIdBytes> const& rhs);

/**
 * @ingroup pubrel_v5
 * @related basic_pubrel_packet
 * @brief Type alias of basic_pubrel_packet (PacketIdBytes=2).
 */
using pubrel_packet = basic_pubrel_packet<2>;

} // namespace async_mqtt::v5

#include <async_mqtt/packet/impl/v5_pubrel.hpp>

#endif // ASYNC_MQTT_PACKET_V5_PUBREL_HPP
