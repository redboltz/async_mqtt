// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_CONNACK_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_CONNACK_HPP


#include <async_mqtt/error.hpp>
#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/util/buffer.hpp>

#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/packet/control_packet_type.hpp>

/**
 * @defgroup connack_v3_1_1 CONNACK packet (v3.1.1)
 * @ingroup packet_v3_1_1
 */

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

/**
 * @ingroup connack_v3_1_1
 * @brief MQTT CONNACK packet (v3.1.1)
 *
 * Only MQTT broker(sever) can send this packet.
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718033
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v3_1_1_connack.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class connack_packet {
public:
    /**
     * @brief constructor
     * @param session_present If the broker stores the session, then true, otherwise false.
     *                        When the endpoint receives CONNACK packet with session_present is false,
     *                        then stored packets are erased.
     * @param return_code ConnectReturnCode
     *                    See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349256
     */
    explicit connack_packet(
        bool session_present,
        connect_return_code return_code
    );

    /**
     * @brief Get MQTT control packet type
     * @return control packet type
     */
    static constexpr control_packet_type type() {
        return control_packet_type::connack;
    }

    /**
     * @brief Create const buffer sequence.
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
     * @brief Get number of element of const_buffer_sequence.
     * @return number of element of const_buffer_sequence
     */
    static std::size_t num_of_const_buffer_sequence() {
        return 1; // all
    }

    /**
     * @brief Get session_present.
     * @return session_present
     */
    bool session_present() const;

    /**
     * @brief Get connect_return_code.
     * @return connect_return_code
     */
    connect_return_code code() const;

private:

    template <std::size_t PacketIdBytesArg>
    friend std::optional<basic_packet_variant<PacketIdBytesArg>>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver, error_code& ec);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v311_connack;
    friend struct ::ut_packet::v311_connack_error;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    explicit connack_packet(buffer buf, error_code& ec);

private:
    static_vector<char, 4> all_;
};

/**
 * @related connack_packet
 * @brief less than operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs less than the rhs, otherwise false.
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v3_1_1_connack.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
bool operator<(connack_packet const& lhs, connack_packet const& rhs);

/**
 * @related connack_packet
 * @brief equal operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs equal to the rhs, otherwise false.
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v3_1_1_connack.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
bool operator==(connack_packet const& lhs, connack_packet const& rhs);

/**
 * @related connack_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v3_1_1_connack.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, connack_packet const& v);

} // namespace async_mqtt::v3_1_1

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/packet/impl/v3_1_1_connack.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PACKET_V3_1_1_CONNACK_HPP
