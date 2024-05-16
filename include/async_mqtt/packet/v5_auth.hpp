// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_AUTH_HPP)
#define ASYNC_MQTT_PACKET_V5_AUTH_HPP

#include <utility>
#include <numeric>


#include <async_mqtt/packet/buffer_to_packet_variant_fwd.hpp>
#include <async_mqtt/exception.hpp>
#include <async_mqtt/util/buffer.hpp>

#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/reason_code.hpp>
#include <async_mqtt/packet/property_variant.hpp>

/**
 * @defgroup auth_v5
 * @ingroup packet_v5
 * @brief AUTH packet (v5.0)
 */

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @ingroup auth_v5
 * @brief MQTT AUTH packet (v35)
 *
 * MQTT UNSUBACK packet.
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901217
 */
class auth_packet {
public:

    /**
     * @brief constructor
     * @param reason_code auth_reason_code
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901220
     * @param props       properties
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901221
     */
    auth_packet(
        auth_reason_code reason_code,
        properties props
    );

    /**
     * @brief constructor
     */
    auth_packet();

    /**
     * @brief constructor
     * @param reason_code auth_reason_code
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901220
     */
    auth_packet(
        auth_reason_code reason_code
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
    auth_reason_code code() const;

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
    std::ostream& operator<<(std::ostream& o, auth_packet const& v) {
        o <<
            "v5::auth{";
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
    auth_packet(
        std::optional<auth_reason_code> reason_code,
        properties props
    );

private:

    template <std::size_t PacketIdBytesArg>
    friend basic_packet_variant<PacketIdBytesArg>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v5_auth;
    friend struct ::ut_packet::v5_auth_no_arg;
    friend struct ::ut_packet::v5_auth_pid_rc;
    friend struct ::ut_packet::v5_auth_prop_len_last;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    auth_packet(buffer buf);

private:
    std::uint8_t fixed_header_;
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;

    std::optional<auth_reason_code> reason_code_;

    std::size_t property_length_ = 0;
    static_vector<char, 4> property_length_buf_;
    properties props_;
};

} // namespace async_mqtt::v5

#include <async_mqtt/packet/impl/v5_auth.hpp>

#endif // ASYNC_MQTT_PACKET_V5_AUTH_HPP
