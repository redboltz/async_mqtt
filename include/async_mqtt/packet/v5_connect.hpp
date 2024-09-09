// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_CONNECT_HPP)
#define ASYNC_MQTT_PACKET_V5_CONNECT_HPP

#include <utility>
#include <numeric>

#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/error.hpp>

#include <async_mqtt/packet/control_packet_type.hpp>
#include <async_mqtt/packet/will.hpp>
#include <async_mqtt/packet/property_variant.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/variable_bytes.hpp>
#include <async_mqtt/util/static_vector.hpp>

/**
 * @defgroup connect_v5 CONNECT packet (v5.0)
 * @ingroup packet_v5
 */

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @ingroup connect_v5
 * @brief MQTT CONNECT packet (v5)
 *
 * Only MQTT client can send this packet.
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901033
 *
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 * #### Requirements
 * - Header: async_mqtt/packet/v5_connect.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
class connect_packet {
public:
    /**
     * @brief constructor
     * @param clean_start  When the endpoint sends CONNECT packet with clean_start is true,
     *                       then stored packets are erased.
     *                       When the endpoint receives CONNECT packet with clean_start is false,
     *                       then the endpoint start storing PUBLISH packet (QoS1 and QoS2) and PUBREL packet
     *                       that would send by the endpoint until the corresponding response would be received.
     *                       \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901039
     * @param keep_alive_sec When the endpoint sends CONNECT packet with keep_alive_sec,
     *                       then the endpoint start sending PINGREQ packet keep_alive_sec after the last
     *                       packet is sent.
     *                       When the endpoint receives CONNECT packet with keep_alive_sec,
     *                       then start keep_alive_sec * 1.5 timer.
     *                       The timer is reset if any packet is received. If the timer is fired, then
     *                       the endpoint close the underlying layer automatically.
     *                       At that time, if the endpoint recv() is called, then the CompletionToken is
     *                       invoked with system_error.
     *                       \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901045
     * @param client_id      MQTT ClientIdentifier. It is the request to the broker for generating ClientIdentifier
     *                       if it is empty string.
     *                       \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901059
     * @param user_name      MQTT UserName. It is often used for authentication.
     *                       \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901071
     * @param password       MQTT Password. It is often used for authentication.
     *                       \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901072
     * @param props          Connect properties.
     *                       \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901046
     */
    explicit connect_packet(
        bool clean_start,
        std::uint16_t keep_alive_sec,
        std::string client_id,
        std::optional<std::string> user_name = std::nullopt,
        std::optional<std::string> password = std::nullopt,
        properties props = {}
    );

    /**
     * @brief constructor
     * @param clean_start  When the endpoint sends CONNECT packet with clean_start is true,
     *                       then stored packets are erased.
     *                       When the endpoint receives CONNECT packet with clean_start is false,
     *                       then the endpoint start storing PUBLISH packet (QoS1 and QoS2) and PUBREL packet
     *                       that would send by the endpoint until the corresponding response would be received.
     *                       \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901039
     * @param keep_alive_sec When the endpoint sends CONNECT packet with keep_alive_sec,
     *                       then the endpoint start sending PINGREQ packet keep_alive_sec after the last
     *                       packet is sent.
     *                       When the endpoint receives CONNECT packet with keep_alive_sec,
     *                       then start keep_alive_sec * 1.5 timer.
     *                       The timer is reset if any packet is received. If the timer is fired, then
     *                       the endpoint close the underlying layer automatically.
     *                       At that time, if the endpoint recv() is called, then the CompletionToken is
     *                       invoked with system_error.
     *                       \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901045
     * @param client_id      MQTT ClientIdentifier. It is the request to the broker for generating ClientIdentifier
     *                       if it is empty string.
     *                       \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901059
     * @param w              MQTT Will
     *                       \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901060
     * @param user_name      MQTT UserName. It is often used for authentication.
     *                       \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901071
     * @param password       MQTT Password. It is often used for authentication.
     *                       \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901072
     * @param props          Connect properties.
     *                       \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901046
     */
    explicit connect_packet(
        bool clean_start,
        std::uint16_t keep_alive_sec,
        std::string client_id,
        std::optional<will> w,
        std::optional<std::string> user_name = std::nullopt,
        std::optional<std::string> password = std::nullopt,
        properties props = {}
    );

    /**
     * @brief Get MQTT control packet type
     * @return control packet type
     */
    static constexpr control_packet_type type() {
        return control_packet_type::connect;
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
     * @brief Get clean_start.
     * @return clean_start
     */
    bool clean_start() const;

    /**
     * @brief Get keep_alive.
     * @return keep_alive
     */
    std::uint16_t keep_alive() const;

    /**
     * @brief Get client_id
     * @return client_id
     */
    std::string client_id() const;

    /**
     * @brief Get user_name.
     * @return user_name
     */
    std::optional<std::string> user_name() const;

    /**
     * @brief Get password.
     * @return password
     */
    std::optional<std::string> password() const;

    /**
     * @brief Get will.
     * @return will
     */
    std::optional<will> get_will() const;

    /**
     * @brief Get properties
     * @return properties
     */
    properties const& props() const;

private:

    template <std::size_t PacketIdBytesArg>
    friend basic_packet_variant<PacketIdBytesArg>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver, error_code& ec);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v5_connect;
    friend struct ::ut_packet::v5_connect_error;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    explicit connect_packet(buffer buf, error_code& ec);

private:
    std::uint8_t fixed_header_;
    char connect_flags_;

    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;

    static_vector<char, 7> protocol_name_and_level_;
    buffer client_id_;
    static_vector<char, 2> client_id_length_buf_;

    buffer will_topic_;
    static_vector<char, 2> will_topic_length_buf_;
    buffer will_message_;
    static_vector<char, 2> will_message_length_buf_;
    std::size_t will_property_length_;
    static_vector<char, 4> will_property_length_buf_;
    properties will_props_;

    buffer user_name_;
    static_vector<char, 2> user_name_length_buf_;
    buffer password_;
    static_vector<char, 2> password_length_buf_;

    static_vector<char, 2> keep_alive_buf_;

    std::size_t property_length_;
    static_vector<char, 4> property_length_buf_;
    properties props_;
};

/**
 * @related connect_packet
 * @brief less than operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs less than the rhs, otherwise false.
 *
 * #### Requirements
 * - Header: async_mqtt/packet/v5_connect.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
bool operator<(connect_packet const& lhs, connect_packet const& rhs);

/**
 * @related connect_packet
 * @brief equal operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs equal to the rhs, otherwise false.
 *
 * #### Requirements
 * - Header: async_mqtt/packet/v5_connect.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
bool operator==(connect_packet const& lhs, connect_packet const& rhs);

/**
 * @related connect_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * - Header: async_mqtt/packet/v5_connect.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, connect_packet const& v);

} // namespace async_mqtt::v5

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/packet/impl/v5_connect.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PACKET_V5_CONNECT_HPP
