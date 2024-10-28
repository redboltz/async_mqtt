// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_V5_PUBACK_HPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_V5_PUBACK_HPP

#include <async_mqtt/protocol/buffer_to_packet_variant.hpp>
#include <async_mqtt/protocol/error.hpp>

#include <async_mqtt/protocol/packet/control_packet_type.hpp>
#include <async_mqtt/protocol/packet/packet_id_type.hpp>
#include <async_mqtt/protocol/packet/property_variant.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/static_vector.hpp>

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @brief MQTT PUBACK packet (v5)
 * @tparam PacketIdBytes size of packet_id
 *
 * If basic_endpoint::set_auto_pub_response() is called with true, then this packet is
 * automatically sent when PUBLISH (QoS1) v5::basic_publish_packet is received.
 *
 * When the packet is received, the packet_id is automatically released and become reusable.
 * \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901121"></a>
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 */
template <std::size_t PacketIdBytes>
class basic_puback_packet {
public:

    /**
     * @brief constructor
     *
     * @param packet_id   MQTT PacketIdentifier that is corresponding to the PUBLISH(QoS1) packet
     * @param reason_code PubackReasonCode
     *                    \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901124"></a>
     * @param props       properties.
     *                    \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901125"></a>
     */
    explicit basic_puback_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        puback_reason_code reason_code,
        properties props
    );

    /**
     * @brief constructor
     *
     * @param packet_id   MQTT PacketIdentifier that is corresponding to the PUBLISH(QoS1) packet
     */
    explicit basic_puback_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id
    );

    /**
     * @brief constructor
     *
     * @param packet_id   MQTT PacketIdentifier that is corresponding to the PUBLISH(QoS1) packet
     * @param reason_code PubackReasonCode
     *                    \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901124"></a>
     */
    explicit basic_puback_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        puback_reason_code reason_code
    );

    /**
     * @brief Get MQTT control packet type
     * @return control packet type
     */
    static constexpr control_packet_type type() {
        return control_packet_type::puback;
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
    puback_reason_code code() const;

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
    std::ostream& operator<<(std::ostream& o, basic_puback_packet<PacketIdBytes> const& v) {
        o <<
            "v5::puback{" <<
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

    template <std::size_t PacketIdBytesArg>
    friend std::optional<basic_packet_variant<PacketIdBytesArg>>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver, error_code& ec);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v5_puback;
    friend struct ::ut_packet::v5_puback_pid4;
    friend struct ::ut_packet::v5_puback_pid_only;
    friend struct ::ut_packet::v5_puback_pid_rc;
    friend struct ::ut_packet::v5_puback_prop_len_last;
    friend struct ::ut_packet::v5_puback_error;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    explicit basic_puback_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        std::optional<puback_reason_code> reason_code,
        properties props
    );

    explicit basic_puback_packet(buffer buf, error_code& ec);

private:
    std::uint8_t fixed_header_;
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;
    static_vector<char, PacketIdBytes> packet_id_;

    std::optional<puback_reason_code> reason_code_;

    std::size_t property_length_ = 0;
    static_vector<char, 4> property_length_buf_;
    properties props_;
};

/**
 * @related basic_puback_packet
 * @brief less than operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs less than the rhs, otherwise false.
 *
 */
template <std::size_t PacketIdBytes>
bool operator<(basic_puback_packet<PacketIdBytes> const& lhs, basic_puback_packet<PacketIdBytes> const& rhs);

/**
 * @related basic_puback_packet
 * @brief equal operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs equal to the rhs, otherwise false.
 *
 */
template <std::size_t PacketIdBytes>
bool operator==(basic_puback_packet<PacketIdBytes> const& lhs, basic_puback_packet<PacketIdBytes> const& rhs);

/**
 * @related basic_puback_packet
 * @brief Type alias of basic_puback_packet (PacketIdBytes=2).
 *
 */
using puback_packet = basic_puback_packet<2>;

} // namespace async_mqtt::v5

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/packet/impl/v5_puback.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PROTOCOL_PACKET_V5_PUBACK_HPP
