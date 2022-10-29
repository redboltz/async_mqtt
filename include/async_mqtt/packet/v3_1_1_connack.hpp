// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_CONNACK_HPP)
#define ASYNC_MQTT_PACKET_CONNACK_HPP

#include <utility>
#include <numeric>

#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/connect_return_code.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

class connack_packet {
public:
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
        buf.remove_prefix(1);
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
     * @brief Get whole size of sequence
     * @return whole size
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
    boost::container::static_vector<char, 4> all_;
};

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PACKET_HPP
