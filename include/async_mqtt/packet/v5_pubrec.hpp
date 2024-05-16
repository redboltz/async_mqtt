// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_PUBREC_HPP)
#define ASYNC_MQTT_PACKET_V5_PUBREC_HPP

#include <async_mqtt/packet/buffer_to_packet_variant_fwd.hpp>
#include <async_mqtt/exception.hpp>
#include <async_mqtt/util/buffer.hpp>

#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/reason_code.hpp>
#include <async_mqtt/packet/property_variant.hpp>

/**
 * @defgroup pubrec_v5
 * @ingroup packet_v5
 */

/**
 * @defgroup pubrec_v5_detail
 * @ingroup pubrec_v5
 * @brief packet internal detailes (e.g. type-aliased API's actual type information)
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
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901131
 */
template <std::size_t PacketIdBytes>
class basic_pubrec_packet {
public:

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBLISH(QoS2) packet
     * @param reason_code PubcompReasonCode
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901134
     * @param props       properties.
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901135
     */
    basic_pubrec_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        pubrec_reason_code reason_code,
        properties props
    );

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBLISH(QoS2) packet
     */
    basic_pubrec_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id
    );

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBLISH(QoS2) packet
     * @param reason_code PubcompReasonCode
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901134
     */
    basic_pubrec_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        pubrec_reason_code reason_code
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
    basic_pubrec_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        std::optional<pubrec_reason_code> reason_code,
        properties props
    );

private:

    template <std::size_t PacketIdBytesArg>
    friend basic_packet_variant<PacketIdBytesArg>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v5_pubrec;
    friend struct ::ut_packet::v5_pubrec_pid4;
    friend struct ::ut_packet::v5_pubrec_pid_only;
    friend struct ::ut_packet::v5_pubrec_pid_rc;
    friend struct ::ut_packet::v5_pubrec_prop_len_last;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    basic_pubrec_packet(buffer buf);

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
 * @ingroup pubrec_v5
 * @related basic_pubrec_packet
 * @brief Type alias of basic_pubrec_packet (PacketIdBytes=2).
 */
using pubrec_packet = basic_pubrec_packet<2>;

} // namespace async_mqtt::v5

#include <async_mqtt/packet/impl/v5_pubrec.hpp>

#endif // ASYNC_MQTT_PACKET_V5_PUBREC_HPP
