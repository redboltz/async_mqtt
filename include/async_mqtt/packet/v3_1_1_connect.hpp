// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_CONNECT_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_CONNECT_HPP

#include <async_mqtt/buffer_to_packet_variant_fwd.hpp>
#include <async_mqtt/buffer.hpp>

#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/connect_flags.hpp>
#include <async_mqtt/packet/will.hpp>

/**
 * @defgroup connect_v3_1_1
 * @ingroup packet_v3_1_1
 */

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

/**
 * @ingroup connect_v3_1_1
 * @brief MQTT CONNECT packet (v3.1.1)
 *
 * Only MQTT client can send this packet.
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718028
 */
class connect_packet {
public:
    /**
     * @brief constructor
     * @param clean_session  When the endpoint sends CONNECT packet with clean_session is true,
     *                       then stored packets are erased.
     *                       When the endpoint receives CONNECT packet with clean_session is false,
     *                       then the endpoint start storing PUBLISH packet (QoS1 and QoS2) and PUBREL packet
     *                       that would send by the endpoint until the corresponding response would be received.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349231
     * @param keep_alive_sec When the endpoint sends CONNECT packet with keep_alive_sec,
     *                       then the endpoint start sending PINGREQ packet keep_alive_sec after the last
     *                       packet is sent.
     *                       When the endpoint receives CONNECT packet with keep_alive_sec,
     *                       then start keep_alive_sec * 1.5 timer.
     *                       The timer is reset if any packet is received. If the timer is fired, then
     *                       the endpoint close the underlying layer automatically.
     *                       At that time, if the endpoint recv() is called, then the CompletionToken is
     *                       invoked with system_error.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349237
     * @param client_id      MQTT ClientIdentifier. It is the request to the broker for generating ClientIdentifier
     *                       if it is empty string and clean_session is true, If false then protocol error.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349242
     * @param user_name      MQTT UserName. It is often used for authentication.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349245
     * @param password       MQTT Password. It is often used for authentication.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349246
     */
    connect_packet(
        bool clean_session,
        std::uint16_t keep_alive_sec,
        std::string client_id,
        std::optional<std::string> user_name = std::nullopt,
        std::optional<std::string> password = std::nullopt
    );

    /**
     * @brief constructor
     * @param clean_session  When the endpoint sends CONNECT packet with clean_session is true,
     *                       then stored packets are erased.
     *                       When the endpoint receives CONNECT packet with clean_session is false,
     *                       then the endpoint start storing PUBLISH packet (QoS1 and QoS2) and PUBREL packet
     *                       that would send by the endpoint until the corresponding response would be received.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349231
     * @param keep_alive_sec When the endpoint sends CONNECT packet with keep_alive_sec,
     *                       then the endpoint start sending PINGREQ packet keep_alive_sec after the last
     *                       packet is sent.
     *                       When the endpoint receives CONNECT packet with keep_alive_sec,
     *                       then start keep_alive_sec * 1.5 timer.
     *                       The timer is reset if any packet is received. If the timer is fired, then
     *                       the endpoint close the underlying layer automatically.
     *                       At that time, if the endpoint recv() is called, then the CompletionToken is
     *                       invoked with system_error.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349237
     * @param client_id      MQTT ClientIdentifier. It is the request to the broker for generating ClientIdentifier
     *                       if it is empty string and clean_session is true, If false then protocol error.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349242
     * @param will           MQTT Will
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349243
     * @param user_name      MQTT UserName. It is often used for authentication.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349245
     * @param password       MQTT Password. It is often used for authentication.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349246
     */
    connect_packet(
        bool clean_session,
        std::uint16_t keep_alive_sec,
        std::string client_id,
        std::optional<will> w,
        std::optional<std::string> user_name = std::nullopt,
        std::optional<std::string> password = std::nullopt
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
     * @brief Get clean_session.
     * @return clean_session
     */
    bool clean_session() const;

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

private:

    template <std::size_t PacketIdBytesArg>
    friend basic_packet_variant<PacketIdBytesArg>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v311_connect;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    connect_packet(buffer buf);

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

    buffer user_name_;
    static_vector<char, 2> user_name_length_buf_;
    buffer password_;
    static_vector<char, 2> password_length_buf_;

    static_vector<char, 2> keep_alive_buf_;
};

/**
 * @related connect_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 */
std::ostream& operator<<(std::ostream& o, connect_packet const& v);

} // namespace async_mqtt::v3_1_1

#include <async_mqtt/packet/impl/v3_1_1_connect.hpp>

#endif // ASYNC_MQTT_PACKET_V3_1_1_CONNECT_HPP
