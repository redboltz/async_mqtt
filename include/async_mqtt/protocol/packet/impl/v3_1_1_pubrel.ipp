// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_V3_1_1_PUBREL_IPP)
#define ASYNC_MQTT_PACKET_IMPL_V3_1_1_PUBREL_IPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/packet/v3_1_1_pubrel.hpp>
#include <async_mqtt/packet/impl/packet_helper.hpp>
#include <async_mqtt/util/buffer.hpp>

#include <async_mqtt/util/inline.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>

#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/detail/fixed_header.hpp>

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/detail/instantiate_helper.hpp>
#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_pubrel_packet<PacketIdBytes>::basic_pubrel_packet(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id
)
    : all_(all_.capacity())
{
    all_[0] = static_cast<char>(detail::make_fixed_header(control_packet_type::pubrel, 0b0010));
    all_[1] = boost::numeric_cast<char>(PacketIdBytes);
    endian_store(packet_id, &all_[2]);
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<as::const_buffer> basic_pubrel_packet<PacketIdBytes>::const_buffer_sequence() const {
    std::vector<as::const_buffer> ret;

    ret.emplace_back(as::buffer(all_.data(), all_.size()));
    return ret;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t basic_pubrel_packet<PacketIdBytes>::size() const {
    return all_.size();
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
typename basic_packet_id_type<PacketIdBytes>::type basic_pubrel_packet<PacketIdBytes>::packet_id() const {
    return endian_load<typename basic_packet_id_type<PacketIdBytes>::type>(&all_[2]);
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_pubrel_packet<PacketIdBytes>::basic_pubrel_packet(buffer buf, error_code& ec) {
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
    if (!cpt_opt || *cpt_opt != control_packet_type::pubrel) {
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
    if (static_cast<std::uint8_t>(all_.back()) != PacketIdBytes) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    // variable header
    if (buf.size() != PacketIdBytes) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    std::copy(buf.begin(), buf.end(), std::back_inserter(all_));
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator==(basic_pubrel_packet<PacketIdBytes> const& lhs, basic_pubrel_packet<PacketIdBytes> const& rhs) {
    return detail::equal(lhs, rhs);
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator<(basic_pubrel_packet<PacketIdBytes> const& lhs, basic_pubrel_packet<PacketIdBytes> const& rhs) {
    return detail::less_than(lhs, rhs);
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, basic_pubrel_packet<PacketIdBytes> const& v) {
    o <<
        "v3_1_1::pubrel{" <<
        "pid:" << v.packet_id() <<
        "}";
    return o;
}

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#define ASYNC_MQTT_INSTANTIATE_EACH(a_size) \
template class basic_pubrel_packet<a_size>; \
template bool operator==(basic_pubrel_packet<a_size> const& lhs, basic_pubrel_packet<a_size> const& rhs); \
template bool operator<(basic_pubrel_packet<a_size> const& lhs, basic_pubrel_packet<a_size> const& rhs); \
template std::ostream& operator<<(std::ostream& o, basic_pubrel_packet<a_size> const& v);

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

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PACKET_IMPL_V3_1_1_PUBREL_IPP
