// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_V3_1_1_CONNACK_IPP)
#define ASYNC_MQTT_PACKET_IMPL_V3_1_1_CONNACK_IPP

#include <utility>
#include <numeric>

#include <async_mqtt/protocol/packet/v3_1_1_connack.hpp>
#include <async_mqtt/protocol/packet/impl/packet_helper.hpp>

#include <async_mqtt/util/inline.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/protocol/packet/detail/fixed_header.hpp>
#include <async_mqtt/protocol/packet/impl/session_present.hpp>

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

ASYNC_MQTT_HEADER_ONLY_INLINE
connack_packet::connack_packet(
    bool session_present,
    connect_return_code return_code
)
    : all_{
        static_cast<char>(detail::make_fixed_header(control_packet_type::connack, 0b0000)),
        0b0010,
        static_cast<char>(session_present ? 1 : 0),
        static_cast<char>(return_code)
      }
{
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<as::const_buffer> connack_packet::const_buffer_sequence() const {
    std::vector<as::const_buffer> ret;

    ret.emplace_back(as::buffer(all_.data(), all_.size()));
    return ret;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t connack_packet::size() const {
    return all_.size();
}

ASYNC_MQTT_HEADER_ONLY_INLINE
bool connack_packet::session_present() const {
    return is_session_present(all_[2]);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
connect_return_code connack_packet::code() const {
    return static_cast<connect_return_code>(all_[3]);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
connack_packet::connack_packet(buffer buf, error_code& ec) {
    // fixed_header
    if (buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    all_.push_back(buf.front());
    buf.remove_prefix(1);
    auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(all_.back()));
    if (!cpt_opt || *cpt_opt != control_packet_type::connack) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    // remaining_length
    if (buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    all_.push_back(buf.front());
    buf.remove_prefix(1);
    if (static_cast<std::uint8_t>(all_.back()) != 0b00000010) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    // variable header
    if (buf.size() != 2) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    all_.push_back(buf.front());
    buf.remove_prefix(1);
    if ((static_cast<std::uint8_t>(all_.back()) & 0b11111110)!= 0) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    all_.push_back(buf.front());
    if (static_cast<std::uint8_t>(all_.back()) > 5) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    ec = error_code{};
}

ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator==(connack_packet const& lhs, connack_packet const& rhs) {
    return detail::equal(lhs, rhs);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator<(connack_packet const& lhs, connack_packet const& rhs) {
    return detail::less_than(lhs, rhs);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, connack_packet const& v) {
    o <<
        "v3_1_1::connack{" <<
        "rc:" << v.code() << "," <<
        "sp:" << v.session_present() <<
        "}";
    return o;
}

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PACKET_IMPL_V3_1_1_CONNACK_IPP
