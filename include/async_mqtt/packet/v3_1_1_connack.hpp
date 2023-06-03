// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_CONNACK_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_CONNACK_HPP

#include <utility>
#include <numeric>

#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/session_present.hpp>
#include <async_mqtt/packet/connect_return_code.hpp>

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

/**
 * @bried MQTT CONNACK packet (v3.1.1)
 *
 * Only MQTT broker(sever) can send this packet.
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718033
 */
class connack_packet {
public:
    /**
     * @bried constructor
     * @param session_present If the broker stores the session, then true, otherwise false.
     *                        When the endpoint receives CONNACK packet with session_present is false,
     *                        then stored packets are erased.
     * @param return_code ConnectReturnCode
     *                    See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349256
     */
    connack_packet(
        bool session_present,
        connect_return_code return_code
    )
        : all_{
              static_cast<char>(make_fixed_header(control_packet_type::connack, 0b0000)),
              0b0010,
              static_cast<char>(session_present ? 1 : 0),
              static_cast<char>(return_code)
        }
    {
    }

    connack_packet(buffer buf) {
        // fixed_header
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connack_packet fixed_header doesn't exist"
            );
        }
        all_.push_back(buf.front());
        buf.remove_prefix(1);
        auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(all_.back()));
        if (!cpt_opt || *cpt_opt != control_packet_type::connack) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connack_packet fixed_header is invalid"
            );
        }

        // remaining_length
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connack_packet remaining_length doesn't exist"
            );
        }
        all_.push_back(buf.front());
        buf.remove_prefix(1);
        if (static_cast<std::uint8_t>(all_.back()) != 0b00000010) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connack_packet remaining_length is invalid"
            );
        }

        // variable header
        if (buf.size() != 2) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connack_packet variable header doesn't match its length"
            );
        }
        all_.push_back(buf.front());
        buf.remove_prefix(1);
        if ((static_cast<std::uint8_t>(all_.back()) & 0b11111110)!= 0) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connack_packet connect acknowledge flags is invalid"
            );
        }
        all_.push_back(buf.front());
        if (static_cast<std::uint8_t>(all_.back()) > 5) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connack_packet connect_return_code is invalid"
            );
        }
    }

    constexpr control_packet_type type() const {
        return control_packet_type::connack;
    }

    /**
     * @brief Create const buffer sequence.
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
     * @brief Get number of element of const_buffer_sequence.
     * @return number of element of const_buffer_sequence
     */
    static constexpr std::size_t num_of_const_buffer_sequence() {
        return 1; // all
    }

    /**
     * @brief Get session_present.
     * @return session_present
     */
    bool session_present() const {
        return is_session_present(all_[2]);
    }

    /**
     * @brief Get connect_return_code.
     * @return connect_return_code
     */
    connect_return_code code() const {
        return static_cast<connect_return_code>(all_[3]);
    }

private:
    static_vector<char, 4> all_;
};

inline std::ostream& operator<<(std::ostream& o, connack_packet const& v) {
    o <<
        "v3_1_1::connack{" <<
        "rc:" << v.code() << "," <<
        "sp:" << v.session_present() <<
        "}";
    return o;
}

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PACKET_V3_1_1_CONNACK_HPP
