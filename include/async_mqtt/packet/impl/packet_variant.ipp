// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_PACKET_VARIANT_IPP)
#define ASYNC_MQTT_PACKET_IMPL_PACKET_VARIANT_IPP

#include <async_mqtt/packet/packet_variant.hpp>
#include <async_mqtt/util/inline.hpp>

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/detail/instantiate_helper.hpp>
#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

namespace async_mqtt {

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<control_packet_type> basic_packet_variant<PacketIdBytes>::type() const {
    return visit(
        overload {
            [] (auto const& p) -> std::optional<control_packet_type>{
                return p.type();
            },
                [] (std::monostate const&) -> std::optional<control_packet_type>{
                    return std::nullopt;
                }
        }
    );
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<as::const_buffer> basic_packet_variant<PacketIdBytes>::const_buffer_sequence() const {
    return visit(
        overload {
            [] (auto const& p) {
                return p.const_buffer_sequence();
            },
            [] (std::monostate const&) {
                return std::vector<as::const_buffer>{};
            }
        }
    );
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_packet_variant<PacketIdBytes>::operator bool() const {
    return var_.index() != 0;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, basic_packet_variant<PacketIdBytes> const& v) {
    v.visit(
        overload {
            [&] (auto const& p) {
                o << p;
            },
            [&] (std::monostate const&) {
            }
        }
    );
    return o;
}

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#define ASYNC_MQTT_INSTANTIATE_EACH(a_size) \
template class basic_packet_variant<a_size>; \
template std::ostream& operator<<(std::ostream& o, basic_packet_variant<a_size> const& v);

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

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_IMPL_PACKET_VARIANT_IPP
