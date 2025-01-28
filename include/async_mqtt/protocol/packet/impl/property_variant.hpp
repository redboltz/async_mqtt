// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_IMPL_PROPERTY_VARIANT_HPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_IMPL_PROPERTY_VARIANT_HPP

#include <variant>

#include <async_mqtt/util/overload.hpp>
#include <async_mqtt/protocol/packet/property.hpp>
#include <async_mqtt/protocol/packet/property_variant.hpp>
#include <async_mqtt/protocol/packet/impl/validate_property.hpp>

namespace async_mqtt {

template <
    typename Property,
    std::enable_if_t<
        !std::is_same_v<std::decay_t<Property>, property_variant>,
        std::nullptr_t
    >*
>
inline
property_variant::property_variant(Property&& property):var_{std::forward<Property>(property)}
{}

template <typename Func>
inline
auto property_variant::visit(Func&& func) const& {
    return
        std::visit(
            std::forward<Func>(func),
            var_
        );
}

template <typename Func>
inline
auto property_variant::visit(Func&& func) & {
    return
        std::visit(
            std::forward<Func>(func),
            var_
        );
}

template <typename Func>
inline
auto property_variant::visit(Func&& func) && {
    return
        std::visit(
            std::forward<Func>(func),
            force_move(var_)
        );
}

template <typename T>
inline
decltype(auto) property_variant::get() {
    return std::get<T>(var_);
}

template <typename T>
inline
decltype(auto) property_variant::get() const {
    return std::get<T>(var_);
}

template <typename T>
inline
decltype(auto) property_variant::get_if() {
    return std::get_if<T>(&var_);
}

template <typename T>
inline
decltype(auto) property_variant::get_if() const {
    return std::get_if<T>(&var_);
}

properties make_properties(buffer buf, property_location loc, error_code& ec);
std::vector<as::const_buffer> const_buffer_sequence(properties const& props);
std::size_t size(properties const& props);
std::size_t num_of_const_buffer_sequence(properties const& props);

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/packet/impl/property_variant.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PROTOCOL_PACKET_IMPL_PROPERTY_VARIANT_HPP
