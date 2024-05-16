// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_V5_SUBACK_HPP)
#define ASYNC_MQTT_PACKET_IMPL_V5_SUBACK_HPP

#include <async_mqtt/packet/v5_suback.hpp>
#include <async_mqtt/exception.hpp>
#include <async_mqtt/util/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>

#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/topic_subopts.hpp>
#include <async_mqtt/packet/reason_code.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/packet/copy_to_static_vector.hpp>

namespace async_mqtt::v5 {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
inline
basic_suback_packet<PacketIdBytes>::basic_suback_packet(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id,
    std::vector<suback_reason_code> params,
    properties props
)
    : fixed_header_{make_fixed_header(control_packet_type::suback, 0b0000)},
      entries_{force_move(params)},
      remaining_length_{PacketIdBytes + entries_.size()},
      property_length_(async_mqtt::size(props)),
      props_(force_move(props))
{
    using namespace std::literals;
    endian_store(packet_id, packet_id_.data());

    auto pb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(property_length_));
    for (auto e : pb) {
        property_length_buf_.push_back(e);
    }

    for (auto const& prop : props_) {
        auto id = prop.id();
        if (!validate_property(property_location::suback, id)) {
            throw make_error(
                errc::bad_message,
                "v5::suback_packet property "s + id_to_str(id) + " is not allowed"
            );
        }
    }

    remaining_length_ += property_length_buf_.size() + property_length_;
    remaining_length_buf_ = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
}

template <std::size_t PacketIdBytes>
inline
constexpr control_packet_type basic_suback_packet<PacketIdBytes>::type() {
    return control_packet_type::suback;
}

template <std::size_t PacketIdBytes>
inline
std::vector<as::const_buffer> basic_suback_packet<PacketIdBytes>::const_buffer_sequence() const {
    std::vector<as::const_buffer> ret;
    ret.reserve(num_of_const_buffer_sequence());

    ret.emplace_back(as::buffer(&fixed_header_, 1));

    ret.emplace_back(as::buffer(remaining_length_buf_.data(), remaining_length_buf_.size()));

    ret.emplace_back(as::buffer(packet_id_.data(), packet_id_.size()));

    ret.emplace_back(as::buffer(property_length_buf_.data(), property_length_buf_.size()));
    auto props_cbs = async_mqtt::const_buffer_sequence(props_);
    std::move(props_cbs.begin(), props_cbs.end(), std::back_inserter(ret));

    ret.emplace_back(as::buffer(entries_.data(), entries_.size()));

    return ret;
}

template <std::size_t PacketIdBytes>
inline
std::size_t basic_suback_packet<PacketIdBytes>::size() const {
    return
        1 +                            // fixed header
        remaining_length_buf_.size() +
        remaining_length_;
}

template <std::size_t PacketIdBytes>
inline
std::size_t basic_suback_packet<PacketIdBytes>::num_of_const_buffer_sequence() const {
    return
        1 +                   // fixed header
        1 +                   // remaining length
        1 +                   // packet id
        [&] () -> std::size_t {
            if (property_length_buf_.size() == 0) return 0;
            return
                1 +                   // property length
                async_mqtt::num_of_const_buffer_sequence(props_);
        }() +
        1;                    // suback_reason_code vector
}

template <std::size_t PacketIdBytes>
inline
typename basic_packet_id_type<PacketIdBytes>::type basic_suback_packet<PacketIdBytes>::packet_id() const {
    return endian_load<typename basic_packet_id_type<PacketIdBytes>::type>(packet_id_.data());
}

template <std::size_t PacketIdBytes>
inline
std::vector<suback_reason_code> const& basic_suback_packet<PacketIdBytes>::entries() const {
    return entries_;
}

template <std::size_t PacketIdBytes>
inline
properties const& basic_suback_packet<PacketIdBytes>::props() const {
    return props_;
}

template <std::size_t PacketIdBytes>
inline
basic_suback_packet<PacketIdBytes>::basic_suback_packet(buffer buf) {
    // fixed_header
    if (buf.empty()) {
        throw make_error(
            errc::bad_message,
            "v5::suback_packet fixed_header doesn't exist"
        );
    }
    fixed_header_ = static_cast<std::uint8_t>(buf.front());
    buf.remove_prefix(1);
    auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(fixed_header_));
    if (!cpt_opt || *cpt_opt != control_packet_type::suback) {
        throw make_error(
            errc::bad_message,
            "v5::suback_packet fixed_header is invalid"
        );
    }

    // remaining_length
    if (auto vl_opt = insert_advance_variable_length(buf, remaining_length_buf_)) {
        remaining_length_ = *vl_opt;
    }
    else {
        throw make_error(errc::bad_message, "v5::suback_packet remaining length is invalid");
    }
    if (remaining_length_ != buf.size()) {
        throw make_error(errc::bad_message, "v5::suback_packet remaining length doesn't match buf.size()");
    }

    // packet_id
    if (!copy_advance(buf, packet_id_)) {
        throw make_error(
            errc::bad_message,
            "v5::suback_packet packet_id doesn't exist"
        );
    }

    // property
    auto it = buf.begin();
    if (auto pl_opt = variable_bytes_to_val(it, buf.end())) {
        property_length_ = *pl_opt;
        std::copy(buf.begin(), it, std::back_inserter(property_length_buf_));
        buf.remove_prefix(std::size_t(std::distance(buf.begin(), it)));
        if (buf.size() < property_length_) {
            throw make_error(
                errc::bad_message,
                "v5::suback_packet properties_don't match its length"
            );
        }
        auto prop_buf = buf.substr(0, property_length_);
        props_ = make_properties(prop_buf, property_location::suback);
        buf.remove_prefix(property_length_);
    }
    else {
        throw make_error(
            errc::bad_message,
            "v5::suback_packet property_length is invalid"
        );
    }

    if (remaining_length_ == 0) {
        throw make_error(errc::bad_message, "v5::suback_packet doesn't have entries");
    }

    while (!buf.empty()) {
        // reason_code
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::suback_packet suback_reason_code  doesn't exist"
            );
        }
        auto rc = static_cast<suback_reason_code>(buf.front());
        entries_.emplace_back(rc);
        buf.remove_prefix(1);
    }
}

template <std::size_t PacketIdBytes>
inline
std::ostream& operator<<(std::ostream& o, basic_suback_packet<PacketIdBytes> const& v) {
    o <<
        "v5::suback{" <<
        "pid:" << v.packet_id() << ",[";
    auto b = v.entries().cbegin();
    auto e = v.entries().cend();
    if (b != e) {
        o << *b++;
    }
    for (; b != e; ++b) {
        o << "," << *b;
    }
    o << "]";
    if (!v.props().empty()) {
        o << ",ps:" << v.props();
    }
    o << "}";
    return o;
}

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PACKET_IMPL_V5_SUBACK_HPP
