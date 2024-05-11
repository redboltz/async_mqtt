// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_PINGRESP_HPP)
#define ASYNC_MQTT_PACKET_V5_PINGRESP_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/buffer_to_packet_variant_fwd.hpp>
#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>

#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/fixed_header.hpp>

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @brief MQTT PINGRESP packet (v5)
 *
 * Only MQTT broker(sever) can send this packet.
 * If basic_endpoint::set_auto_ping_response() is called with true, then this packet is
 * automatically sent when PINGREQ v5::pingreq_packet is received.
 * If the endpoint called basic_endpoint::set_pingresp_recv_timeout_ms() with non 0 argument,
 * and PINGRESP packet isn't received until the timer fired, then send DISCONNECT packet with
 * the reason code disconnect_reason_code::keep_alive_timeout automatically then close underlying
 * layer automatically.
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901200
 */
class pingresp_packet {
public:

    /**
     * @brief constructor
     */
    pingresp_packet()
        : all_(all_.capacity())
    {
        all_[0] = static_cast<char>(make_fixed_header(control_packet_type::pingresp, 0b0000));
        all_[1] = char(0);
    }

    constexpr control_packet_type type() const {
        return control_packet_type::pingresp;
    }

    /**
     * @brief Create const buffer sequence
     *        it is for boost asio APIs
     * @return const buffer sequence
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
        std::vector<as::const_buffer> ret;

        ret.emplace_back(as::buffer(all_.data(), all_.size()));
        return ret;
    }

    /**
     * @brief Get packet size.
     * @return packet size
     */
    std::size_t size() const {
        return all_.size();
    }

    /**
     * @brief Get number of element of const_buffer_sequence
     * @return number of element of const_buffer_sequence
     */
    static constexpr std::size_t num_of_const_buffer_sequence() {
        return 1; // all
    }

private:

    friend basic_packet_variant<2>
    buffer_to_basic_packet_variant<2>(buffer buf, protocol_version ver);
    friend basic_packet_variant<4>
    buffer_to_basic_packet_variant<4>(buffer buf, protocol_version ver);

    // private constructor for internal use
    pingresp_packet(buffer buf) {
        // fixed_header
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::pingresp_packet fixed_header doesn't exist"
            );
        }
        all_.push_back(buf.front());
        buf.remove_prefix(1);
        auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(all_.back()));
        if (!cpt_opt || *cpt_opt != control_packet_type::pingresp) {
            throw make_error(
                errc::bad_message,
                "v5::pingresp_packet fixed_header is invalid"
            );
        }

        // remaining_length
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::pingresp_packet remaining_length doesn't exist"
            );
        }
        all_.push_back(buf.front());

        if (static_cast<std::uint8_t>(all_.back()) != 0) {
            throw make_error(
                errc::bad_message,
                "v5::pingresp_packet remaining_length is invalid"
            );
        }
    }

private:
    static_vector<char, 2> all_;
};

inline std::ostream& operator<<(std::ostream& o, pingresp_packet const& /*v*/) {
    o <<
        "v5::pingresp{}";
    return o;
}

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PACKET_V5_PINGRESP_HPP
