// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_SUBACK_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_SUBACK_HPP

#include <async_mqtt/protocol/buffer_to_packet_variant.hpp>
#include <async_mqtt/protocol/error.hpp>

#include <async_mqtt/protocol/packet/control_packet_type.hpp>
#include <async_mqtt/protocol/packet/topic_subopts.hpp>
#include <async_mqtt/protocol/packet/packet_id_type.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/static_vector.hpp>


/**
 * @defgroup suback_v3_1_1 SUBACK packet (v3.1.1)
 * @ingroup packet_v3_1_1
 */

/**
 * @defgroup suback_v3_1_1_detail implementation class
 * @ingroup suback_v3_1_1
 */

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

/**
 * @ingroup suback_v3_1_1_detail
 * @brief MQTT SUBACK packet (v3.1.1)
 * @tparam PacketIdBytes size of packet_id
 *
 * MQTT SUBACK packet.
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718068
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/v3_1_1_suback.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
class basic_suback_packet {
public:

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the SUBSCRIBE packet
     * @param params    suback entries.
     */
    explicit basic_suback_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        std::vector<suback_return_code> params
    );

    /**
     * @brief Get MQTT control packet type
     * @return control packet type
     */
    static constexpr control_packet_type type() {
        return control_packet_type::suback;
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
        return
            1 +                   // fixed header
            1 +                   // remaining length
            1 +                   // packet id
            1;                    // suback_return_code vector
    }

    /**
     * @brief Get packet_id.
     * @return packet_id
     */
    typename basic_packet_id_type<PacketIdBytes>::type packet_id() const;

    /**
     * @brief Get entries
     * @return entries
     */
    std::vector<suback_return_code> const& entries() const;

private:

    template <std::size_t PacketIdBytesArg>
    friend std::optional<basic_packet_variant<PacketIdBytesArg>>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver, error_code& ec);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v311_suback;
    friend struct ::ut_packet::v311_suback_pid4;
    friend struct ::ut_packet::v311_suback_error;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    explicit basic_suback_packet(buffer buf, error_code& ec);

private:
    std::uint8_t fixed_header_;
    std::vector<suback_return_code> entries_;
    static_vector<char, PacketIdBytes> packet_id_ = static_vector<char, PacketIdBytes>(PacketIdBytes);
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;
};

/**
 * @related basic_suback_packet
 * @brief less than operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs less than the rhs, otherwise false.
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/v3_1_1_suback.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
bool operator<(basic_suback_packet<PacketIdBytes> const& lhs, basic_suback_packet<PacketIdBytes> const& rhs);

/**
 * @related basic_suback_packet
 * @brief equal operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs equal to the rhs, otherwise false.
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/v3_1_1_suback.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
bool operator==(basic_suback_packet<PacketIdBytes> const& lhs, basic_suback_packet<PacketIdBytes> const& rhs);

/**
 * @related basic_suback_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/v3_1_1_suback.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
std::ostream& operator<<(std::ostream& o, basic_suback_packet<PacketIdBytes> const& v);

/**
 * @ingroup suback_v3_1_1
 * @related basic_suback_packet
 * @brief Type alias of basic_suback_packet (PacketIdBytes=2).
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/v3_1_1_suback.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
using suback_packet = basic_suback_packet<2>;

} // namespace async_mqtt::v3_1_1

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/packet/impl/v3_1_1_suback.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PACKET_V3_1_1_SUBACK_HPP
