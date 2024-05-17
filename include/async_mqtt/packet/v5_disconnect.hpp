// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_DISCONNECT_HPP)
#define ASYNC_MQTT_PACKET_V5_DISCONNECT_HPP

#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/exception.hpp>

#include <async_mqtt/packet/control_packet_type.hpp>
#include <async_mqtt/packet/reason_code.hpp>
#include <async_mqtt/packet/property_variant.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/static_vector.hpp>

/**
 * @defgroup disconnect_v5 DISCONNECT packet (v5.0)
 * @ingroup packet_v5
 */

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @ingroup disconnect_v5
 * @brief MQTT DISCONNECT packet (v5)
 *
 * When the endpoint sends DISCONNECT packet, then the endpoint become disconnecting status.
 * The endpoint can't send packets any more.
 * The underlying layer is not automatically closed from the client side.
 * If you want to close the underlying layer from the client side, you need to call basic_endpoint::close()
 * after sending DISCONNECT packet.
 * When the broker receives DISCONNECT packet, then close underlying layer from the broker.
 * In this case, Will is not published by the broker except reason_code is Disconnect with Will Message.
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901205
 */
class disconnect_packet {
public:
    /**
     * @brief constructor
     *
     * @param reason_code DisonnectReasonCode
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901208
     * @param props       properties.
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901209
     */
    disconnect_packet(
        disconnect_reason_code reason_code,
        properties props
    );

    /**
     * @brief constructor
     */
    disconnect_packet(
    );

    /**
     * @brief constructor
     *
     * @param reason_code DisonnectReasonCode
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901208
     */
    disconnect_packet(
        disconnect_reason_code reason_code
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
     * @brief Get reason code
     * @return reason_code
     */
    disconnect_reason_code code() const;

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
    std::ostream& operator<<(std::ostream& o, disconnect_packet const& v) {
        o <<
            "v5::disconnect{";
        if (v.reason_code_) {
            o << "rc:" << *v.reason_code_;
        }
        if (!v.props().empty()) {
            o << ",ps:" << v.props();
        };
        o << "}";
        return o;
    }

private:

    template <std::size_t PacketIdBytesArg>
    friend basic_packet_variant<PacketIdBytesArg>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v5_disconnect;
    friend struct ::ut_packet::v5_disconnect_no_arg;
    friend struct ::ut_packet::v5_disconnect_pid_rc;
    friend struct ::ut_packet::v5_disconnect_prop_len_last;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    disconnect_packet(buffer buf);

    disconnect_packet(
        std::optional<disconnect_reason_code> reason_code,
        properties props
    );

private:
    std::uint8_t fixed_header_;
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;

    std::optional<disconnect_reason_code> reason_code_;

    std::size_t property_length_ = 0;
    static_vector<char, 4> property_length_buf_;
    properties props_;
};

} // namespace async_mqtt::v5

#include <async_mqtt/packet/impl/v5_disconnect.hpp>

#endif // ASYNC_MQTT_PACKET_V5_DISCONNECT_HPP
