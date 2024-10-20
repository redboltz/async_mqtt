// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_PUBREC_HPP)
#define ASYNC_MQTT_PACKET_V5_PUBREC_HPP

#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/error.hpp>

#include <async_mqtt/packet/control_packet_type.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/property_variant.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/static_vector.hpp>

/**
 * @defgroup pubrec_v5 PUBREC packet (v5.0)
 * @ingroup packet_v5
 */

/**
 * @defgroup pubrec_v5_detail implementation class
 * @ingroup pubrec_v5
 */

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @ingroup pubrec_v5_detail
 * @brief MQTT PUBREC packet (v5)
 * @tparam PacketIdBytes size of packet_id
 *
 * If basic_endpoint::set_auto_pub_response() is called with true, then this packet is
 * automatically sent when PUBLISH (QoS2) v5::basic_publish_packet is received.
 *
 * When the packet is received with error reason_code, the packet_id is automatically released and become reusable.
 * \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901131"></a>
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v5_pubrec.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
class basic_pubrec_packet {
public:

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBLISH(QoS2) packet
     * @param reason_code PubcompReasonCode
     *                    \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901134"></a>
     * @param props       properties.
     *                    \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901135"></a>
     */
    explicit basic_pubrec_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        pubrec_reason_code reason_code,
        properties props
    );

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBLISH(QoS2) packet
     */
    explicit basic_pubrec_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id
    );

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBLISH(QoS2) packet
     * @param reason_code PubcompReasonCode
     *                    \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901134"></a>
     */
    explicit basic_pubrec_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        pubrec_reason_code reason_code
    );

    /**
     * @brief Get MQTT control packet type
     * @return control packet type
     */
    static constexpr control_packet_type type() {
        return control_packet_type::pubrec;
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
    std::size_t num_of_const_buffer_sequence() const;

    /**
     * @brief Get packet_id.
     * @return packet_id
     */
    typename basic_packet_id_type<PacketIdBytes>::type packet_id() const;

    /**
     * @brief Get reason code
     * @return reason_code
     */
    pubrec_reason_code code() const;

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
    std::ostream& operator<<(std::ostream& o, basic_pubrec_packet<PacketIdBytes> const& v) {
        o <<
            "v5::pubrec{" <<
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
    explicit basic_pubrec_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        std::optional<pubrec_reason_code> reason_code,
        properties props
    );

private:

    template <std::size_t PacketIdBytesArg>
    friend basic_packet_variant<PacketIdBytesArg>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver, error_code& ec);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v5_pubrec;
    friend struct ::ut_packet::v5_pubrec_pid4;
    friend struct ::ut_packet::v5_pubrec_pid_only;
    friend struct ::ut_packet::v5_pubrec_pid_rc;
    friend struct ::ut_packet::v5_pubrec_prop_len_last;
    friend struct ::ut_packet::v5_pubrec_error;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    explicit basic_pubrec_packet(buffer buf, error_code& ec);

private:
    std::uint8_t fixed_header_;
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;
    static_vector<char, PacketIdBytes> packet_id_;

    std::optional<pubrec_reason_code> reason_code_;

    std::size_t property_length_ = 0;
    static_vector<char, 4> property_length_buf_;
    properties props_;
};

/**
 * @related basic_pubrec_packet
 * @brief less than operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs less than the rhs, otherwise false.
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v5_pubrec.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
bool operator<(basic_pubrec_packet<PacketIdBytes> const& lhs, basic_pubrec_packet<PacketIdBytes> const& rhs);

/**
 * @related basic_pubrec_packet
 * @brief equal operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs equal to the rhs, otherwise false.
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v5_pubrec.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
bool operator==(basic_pubrec_packet<PacketIdBytes> const& lhs, basic_pubrec_packet<PacketIdBytes> const& rhs);

/**
 * @ingroup pubrec_v5
 * @related basic_pubrec_packet
 * @brief Type alias of basic_pubrec_packet (PacketIdBytes=2).
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v5_pubrec.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
using pubrec_packet = basic_pubrec_packet<2>;

} // namespace async_mqtt::v5

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/packet/impl/v5_pubrec.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PACKET_V5_PUBREC_HPP
