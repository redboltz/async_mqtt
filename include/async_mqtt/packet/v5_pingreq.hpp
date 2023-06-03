// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_PINGREQ_HPP)
#define ASYNC_MQTT_PACKET_V5_PINGREQ_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

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
 * @brief MQTT PINGREQ packet (v5)
 *
 * Only MQTT client can send this packet.
 * This packet is to notify the client is living.
 * When you set keep_alive_sec > 0 on CONNECT packet (v5::connect_packet),
 * then PINGREQ packet is automatically sent after keep_alive_sec passed after the last packet sent.
 * When the broker receives this packet, KeepAlive timeout (keep_alive_sec * 1.5) is reset and
 * start counting from 0.
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901195
 */
class pingreq_packet {
public:
    pingreq_packet()
        : all_(all_.capacity())
    {
        all_[0] = static_cast<char>(make_fixed_header(control_packet_type::pingreq, 0b0000));
        all_[1] = char(0);
    }

    /**
     * @brief constructor
     */
    pingreq_packet(buffer buf) {
        // fixed_header
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::pingreq_packet fixed_header doesn't exist"
            );
        }
        all_.push_back(buf.front());
        buf.remove_prefix(1);
        auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(all_.back()));
        if (!cpt_opt || *cpt_opt != control_packet_type::pingreq) {
            throw make_error(
                errc::bad_message,
                "v5::pingreq_packet fixed_header is invalid"
            );
        }

        // remaining_length
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::pingreq_packet remaining_length doesn't exist"
            );
        }
        all_.push_back(buf.front());

        if (static_cast<std::uint8_t>(all_.back()) != 0) {
            throw make_error(
                errc::bad_message,
                "v5::pingreq_packet remaining_length is invalid"
            );
        }
    }

    constexpr control_packet_type type() const {
        return control_packet_type::pingreq;
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
    static_vector<char, 2> all_;
};

inline std::ostream& operator<<(std::ostream& o, pingreq_packet const& /*v*/) {
    o <<
        "v5::pingreq{}";
    return o;
}

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PACKET_V5_PINGREQ_HPP
