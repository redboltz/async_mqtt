// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_IMPL_V5_UNSUBACK_IPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_IMPL_V5_UNSUBACK_IPP

#include <async_mqtt/protocol/packet/v5_unsuback.hpp>
#include <async_mqtt/protocol/packet/impl/packet_helper.hpp>
#include <async_mqtt/util/buffer.hpp>

#include <async_mqtt/util/inline.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>

#include <async_mqtt/protocol/packet/packet_id_type.hpp>
#include <async_mqtt/protocol/packet/detail/fixed_header.hpp>
#include <async_mqtt/protocol/packet/property_variant.hpp>
#include <async_mqtt/protocol/packet/impl/copy_to_static_vector.hpp>
#include <async_mqtt/protocol/packet/impl/validate_property.hpp>

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/asio_bind/detail/instantiate_helper.hpp>
#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

namespace async_mqtt::v5 {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_unsuback_packet<PacketIdBytes>::basic_unsuback_packet(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id,
    std::vector<unsuback_reason_code> params,
    properties props
)
    : fixed_header_{detail::make_fixed_header(control_packet_type::unsuback, 0b0000)},
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
        if (!validate_property(property_location::unsuback, id)) {
            throw system_error(
                make_error_code(
                    disconnect_reason_code::malformed_packet
                )
            );
        }
    }

    remaining_length_ += property_length_buf_.size() + property_length_;
    remaining_length_buf_ = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<as::const_buffer> basic_unsuback_packet<PacketIdBytes>::const_buffer_sequence() const {
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
ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t basic_unsuback_packet<PacketIdBytes>::size() const {
    return
        1 +                            // fixed header
        remaining_length_buf_.size() +
        remaining_length_;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t basic_unsuback_packet<PacketIdBytes>::num_of_const_buffer_sequence() const {
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
        1;                    // unsuback_reason_code vector
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
typename basic_packet_id_type<PacketIdBytes>::type basic_unsuback_packet<PacketIdBytes>::packet_id() const {
    return endian_load<typename basic_packet_id_type<PacketIdBytes>::type>(packet_id_.data());
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<unsuback_reason_code> const& basic_unsuback_packet<PacketIdBytes>::entries() const {
    return entries_;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
properties const& basic_unsuback_packet<PacketIdBytes>::props() const {
    return props_;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_unsuback_packet<PacketIdBytes>::basic_unsuback_packet(buffer buf, error_code& ec) {
    // fixed_header
    if (buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    fixed_header_ = static_cast<std::uint8_t>(buf.front());
    buf.remove_prefix(1);
    auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(fixed_header_));
    if (!cpt_opt || *cpt_opt != control_packet_type::unsuback) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    // remaining_length
    if (auto vl_opt = insert_advance_variable_length(buf, remaining_length_buf_)) {
        remaining_length_ = *vl_opt;
    }
    else {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    if (remaining_length_ != buf.size()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    // packet_id
    if (!copy_advance(buf, packet_id_)) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    // property
    auto it = buf.begin();
    if (auto pl_opt = variable_bytes_to_val(it, buf.end())) {
        property_length_ = *pl_opt;
        std::copy(buf.begin(), it, std::back_inserter(property_length_buf_));
        buf.remove_prefix(std::size_t(std::distance(buf.begin(), it)));
        if (buf.size() < property_length_) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return;
        }
        auto prop_buf = buf.substr(0, property_length_);
        props_ = make_properties(prop_buf, property_location::unsuback, ec);
        if (ec) return;
        buf.remove_prefix(property_length_);
    }
    else {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    if (buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::protocol_error // no entry
        );
        return;
    }
    while (!buf.empty()) {
        // reason_code
        auto rc = static_cast<unsuback_reason_code>(buf.front());
        entries_.emplace_back(rc);
        buf.remove_prefix(1);
        switch (rc) {
        case unsuback_reason_code::success:
        case unsuback_reason_code::no_subscription_existed:
        case unsuback_reason_code::unspecified_error:
        case unsuback_reason_code::implementation_specific_error:
        case unsuback_reason_code::not_authorized:
        case unsuback_reason_code::topic_filter_invalid:
        case unsuback_reason_code::packet_identifier_in_use:
            break;
        default:
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return;
        }
    }
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator==(basic_unsuback_packet<PacketIdBytes> const& lhs, basic_unsuback_packet<PacketIdBytes> const& rhs) {
    return detail::equal(lhs, rhs);
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator<(basic_unsuback_packet<PacketIdBytes> const& lhs, basic_unsuback_packet<PacketIdBytes> const& rhs) {
    return detail::less_than(lhs, rhs);
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, basic_unsuback_packet<PacketIdBytes> const& v) {
    o <<
        "v5::unsuback{" <<
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
    };
    o << "}";
    return o;
}

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#define ASYNC_MQTT_INSTANTIATE_EACH(a_size) \
template class basic_unsuback_packet<a_size>; \
template bool operator==(basic_unsuback_packet<a_size> const& lhs, basic_unsuback_packet<a_size> const& rhs); \
template bool operator<(basic_unsuback_packet<a_size> const& lhs, basic_unsuback_packet<a_size> const& rhs); \
template std::ostream& operator<<(std::ostream& o, basic_unsuback_packet<a_size> const& v);

#define ASYNC_MQTT_PP_GENERATE(r, product) \
    BOOST_PP_EXPAND( \
        ASYNC_MQTT_INSTANTIATE_EACH \
        BOOST_PP_SEQ_TO_TUPLE( \
            product \
        ) \
    )

BOOST_PP_SEQ_FOR_EACH_PRODUCT(ASYNC_MQTT_PP_GENERATE, (ASYNC_MQTT_PP_SIZE))

#undef ASYNC_MQTT_PP_GENERATE
#undef ASYNC_MQTT_INSTANTIATE_EACH

#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PROTOCOL_PACKET_IMPL_V5_UNSUBACK_IPP
