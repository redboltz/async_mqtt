// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_V3_1_1_CONNACK_HPP)
#define ASYNC_MQTT_PACKET_IMPL_V3_1_1_CONNACK_HPP

#include <utility>
#include <numeric>

#include <async_mqtt/packet/v3_1_1_connack.hpp>
#include <async_mqtt/exception.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

inline
connack_packet::connack_packet(
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

inline
constexpr control_packet_type connack_packet::type() {
    return control_packet_type::connack;
}

inline
std::vector<as::const_buffer> connack_packet::const_buffer_sequence() const {
    std::vector<as::const_buffer> ret;

    ret.emplace_back(as::buffer(all_.data(), all_.size()));
    return ret;
}

inline
std::size_t connack_packet::size() const {
    return all_.size();
}

constexpr std::size_t connack_packet::num_of_const_buffer_sequence() {
    return 1; // all
}

inline
bool connack_packet::session_present() const {
    return is_session_present(all_[2]);
}

inline
connect_return_code connack_packet::code() const {
    return static_cast<connect_return_code>(all_[3]);
}

inline
connack_packet::connack_packet(buffer buf) {
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

inline
std::ostream& operator<<(std::ostream& o, connack_packet const& v) {
    o <<
        "v3_1_1::connack{" <<
        "rc:" << v.code() << "," <<
        "sp:" << v.session_present() <<
        "}";
    return o;
}

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PACKET_IMPL_V3_1_1_CONNACK_HPP
