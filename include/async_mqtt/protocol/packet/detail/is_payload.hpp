// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_DETAIL_IS_PAYLOAD_HPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_DETAIL_IS_PAYLOAD_HPP

#include <type_traits>
#include <iterator>

#include <async_mqtt/util/buffer.hpp>

namespace async_mqtt::detail {

template <typename T, typename Enable = void>
struct has_begin_end : std::false_type
{};

template <typename T>
struct has_begin_end<
    T,
    std::enable_if_t<
        std::is_invocable_v<decltype(std::cbegin<T>), T const&> &&
        std::is_invocable_v<decltype(std::cend<T>), T const&>
    >
>
: std::true_type
{};

template <typename T>
constexpr bool is_payload() {
    return
        std::disjunction<
            std::is_convertible<T, std::string_view>,
            has_begin_end<std::decay_t<T>>
        >::value;
}

} // namespace async_mqtt::detail

#endif // ASYNC_MQTT_PROTOCOL_PACKET_DETAIL_IS_PAYLOAD_HPP
