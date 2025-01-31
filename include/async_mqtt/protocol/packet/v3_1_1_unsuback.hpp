// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_V3_1_1_UNSUBACK_HPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_V3_1_1_UNSUBACK_HPP

#include <async_mqtt/protocol/buffer_to_packet_variant.hpp>

#include <async_mqtt/protocol/packet/control_packet_type.hpp>
#include <async_mqtt/protocol/packet/packet_id_type.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/static_vector.hpp>

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

/**
 * @brief MQTT UNSUBACK packet (v3.1.1)
 * @tparam PacketIdBytes size of packet_id
 *
 * MQTT UNSUBACK packet.
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718072
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 */
template <std::size_t PacketIdBytes>
class basic_unsuback_packet {
public:

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the UNSUBSCRIBE packet
     */
    explicit basic_unsuback_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id
    );

    /**
     * @brief Get MQTT control packet type
     * @return control packet type
     */
    static constexpr control_packet_type type() {
        return control_packet_type::unsuback;
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
    friend struct ::ut_packet::v311_unsuback;
    friend struct ::ut_packet::v311_unsuback_pid4;
    friend struct ::ut_packet::v311_unsuback_error;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    explicit basic_unsuback_packet(buffer buf, error_code& ec);

private:
    static_vector<char, 2 + PacketIdBytes> all_;
};

/**
 * @related basic_unsuback_packet
 * @brief less than operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs less than the rhs, otherwise false.
 *
 */
template <std::size_t PacketIdBytes>
bool operator<(basic_unsuback_packet<PacketIdBytes> const& lhs, basic_unsuback_packet<PacketIdBytes> const& rhs);

/**
 * @related basic_unsuback_packet
 * @brief equal operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs equal to the rhs, otherwise false.
 *
 */
template <std::size_t PacketIdBytes>
bool operator==(basic_unsuback_packet<PacketIdBytes> const& lhs, basic_unsuback_packet<PacketIdBytes> const& rhs);

/**
 * @related basic_unsuback_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 */
template <std::size_t PacketIdBytes>
std::ostream& operator<<(std::ostream& o, basic_unsuback_packet<PacketIdBytes> const& v);

/**
 * @related basic_unsuback_packet
 * @brief Type alias of basic_unsuback_packet (PacketIdBytes=2).
 *
 */
using unsuback_packet = basic_unsuback_packet<2>;

} // namespace async_mqtt::v3_1_1

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/packet/impl/v3_1_1_unsuback.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PROTOCOL_PACKET_V3_1_1_UNSUBACK_HPP
