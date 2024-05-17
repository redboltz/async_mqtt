// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_V3_1_1_DISCONNECT_HPP)
#define ASYNC_MQTT_PACKET_IMPL_V3_1_1_DISCONNECT_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/packet/v3_1_1_disconnect.hpp>
#include <async_mqtt/exception.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>

#include <async_mqtt/packet/detail/fixed_header.hpp>

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

inline
disconnect_packet::disconnect_packet()
    : all_(all_.capacity())
{
    all_[0] = static_cast<char>(detail::make_fixed_header(control_packet_type::disconnect, 0b0000));
    all_[1] = char(0);
}

inline
constexpr control_packet_type disconnect_packet::type() {
    return control_packet_type::disconnect;
}

inline
std::vector<as::const_buffer> disconnect_packet::const_buffer_sequence() const {
    std::vector<as::const_buffer> ret;

    ret.emplace_back(as::buffer(all_.data(), all_.size()));
    return ret;
}

inline
std::size_t disconnect_packet::size() const {
    return all_.size();
}

inline
constexpr std::size_t disconnect_packet::num_of_const_buffer_sequence() {
    return 1; // all
}

inline
disconnect_packet::disconnect_packet(buffer buf) {
    // fixed_header
    if (buf.empty()) {
        throw make_error(
            errc::bad_message,
            "v3_1_1::disconnect_packet fixed_header doesn't exist"
        );
    }
    all_.push_back(buf.front());
    buf.remove_prefix(1);
    auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(all_.back()));
    if (!cpt_opt || *cpt_opt != control_packet_type::disconnect) {
        throw make_error(
            errc::bad_message,
            "v3_1_1::disconnect_packet fixed_header is invalid"
        );
    }

    // remaining_length
    if (buf.empty()) {
        throw make_error(
            errc::bad_message,
            "v3_1_1::disconnect_packet remaining_length doesn't exist"
        );
    }
    all_.push_back(buf.front());

    if (static_cast<std::uint8_t>(all_.back()) != 0) {
        throw make_error(
            errc::bad_message,
            "v3_1_1::disconnect_packet remaining_length is invalid"
        );
    }
}

inline
std::ostream& operator<<(std::ostream& o, disconnect_packet const& /*v*/) {
    o <<
        "v3_1_1::disconnect{}";
    return o;
}

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PACKET_IMPL_V3_1_1_DISCONNECT_HPP
