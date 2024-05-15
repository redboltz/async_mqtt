// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_UNSUBSCRIBE_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_UNSUBSCRIBE_HPP


#include <async_mqtt/buffer_to_packet_variant_fwd.hpp>
#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>

#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/topic_sharename.hpp>
#include <async_mqtt/variable_bytes.hpp>
#include <async_mqtt/packet/copy_to_static_vector.hpp>

/**
 * @defgroup unsubscribe_v3_1_1
 * @ingroup packet_v3_1_1
 */

/**
 * @defgroup unsubscribe_v3_1_1_detail
 * @ingroup unsubscribe_v3_1_1
 * @brief packet internal detailes (e.g. type-aliased API's actual type information)
 */

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

/**
 * @ingroup unsubscribe_v3_1_1_detaied
 * @brief MQTT UNSUBSCRIBE packet (v3.1.1)
 * @tparam PacketIdBytes size of packet_id
 *
 * MQTT UNSUBSCRIBE packet.
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718072
 */
template <std::size_t PacketIdBytes>
class basic_unsubscribe_packet {
public:
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier.the packet_id must be acquired by
     *                  basic_endpoint::acquire_unique_packet_id().
     * @param params    unsubscribe entries.
     */
    basic_unsubscribe_packet(
        packet_id_t packet_id,
        std::vector<topic_sharename> params
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
    std::size_t num_of_const_buffer_sequence() const;

    /**
     * @brief Get packet_id.
     * @return packet_id
     */
    packet_id_t packet_id() const;

    /**
     * @brief Get entries
     * @return entries
     */
    std::vector<topic_sharename> const& entries() const;

private:

    template <std::size_t PacketIdBytesArg>
    friend basic_packet_variant<PacketIdBytesArg>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v311_unsubscribe;
    friend struct ::ut_packet::v311_unsubscribe_pid4;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    basic_unsubscribe_packet(buffer buf);

private:
    std::uint8_t fixed_header_;
    std::vector<static_vector<char, 2>> topic_length_buf_entries_;
    std::vector<topic_sharename> entries_;
    static_vector<char, PacketIdBytes> packet_id_ = static_vector<char, PacketIdBytes>(PacketIdBytes);
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;
};

/**
 * @related basic_unsubscribe_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 */
template <std::size_t PacketIdBytes>
std::ostream& operator<<(std::ostream& o, basic_unsubscribe_packet<PacketIdBytes> const& v);

/**
 * @ingroup unsubscribe_v3_1_1
 * @related basic_unsubscribe_packet
 * @brief Type alias of basic_unsubscribe_packet (PacketIdBytes=2).
 */
using unsubscribe_packet = basic_unsubscribe_packet<2>;

} // namespace async_mqtt::v3_1_1

#include <async_mqtt/packet/impl/v3_1_1_unsubscribe.hpp>

#endif // ASYNC_MQTT_PACKET_V3_1_1_UNSUBSCRIBE_HPP
