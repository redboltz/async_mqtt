// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_V5_PUBLISH_HPP)
#define ASYNC_MQTT_PACKET_IMPL_V5_PUBLISH_HPP

#include <utility>
#include <async_mqtt/packet/v5_publish.hpp>
#include <async_mqtt/util/move.hpp>

namespace async_mqtt::v5 {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
template <
    typename StringViewLike,
    typename Payload,
    std::enable_if_t<
        std::is_convertible_v<std::decay_t<StringViewLike>, std::string_view> &&
        detail::is_payload<Payload>(),
        std::nullptr_t
    >
>
inline
basic_publish_packet<PacketIdBytes>::basic_publish_packet(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id,
    StringViewLike&& topic_name,
    Payload&& payloads,
    pub::opts pubopts,
    properties props
):basic_publish_packet{
    packet_id,
    [&]() -> buffer {
        if constexpr(std::is_same_v<std::decay_t<StringViewLike>, buffer>) {
            return topic_name;
        }
        else {
            return buffer{std::string{std::forward<StringViewLike>(topic_name)}};
        }
    }(),
    [&]() -> std::vector<buffer> {
        if constexpr(std::is_same_v<std::decay_t<Payload>, std::vector<buffer>>) {
            return payloads;
        }
        else {
            return std::vector<buffer>{buffer{std::string{std::forward<Payload>(payloads)}}};
        }
    }(),
    pubopts,
    force_move(props)
}
{}

template <std::size_t PacketIdBytes>
template <
    typename StringViewLike,
    typename Payload,
    std::enable_if_t<
        std::is_convertible_v<std::decay_t<StringViewLike>, std::string_view> &&
        detail::is_payload<Payload>(),
        std::nullptr_t
    >
>
inline
basic_publish_packet<PacketIdBytes>::basic_publish_packet(
    StringViewLike&& topic_name,
    Payload&& payloads,
    pub::opts pubopts,
    properties props
) : basic_publish_packet{
    0,
    [&]() -> buffer {
        if constexpr(std::is_same_v<std::decay_t<StringViewLike>, buffer>) {
            return topic_name;
        }
        else {
            return buffer{std::string{std::forward<StringViewLike>(topic_name)}};
        }
    }(),
    [&]() -> std::vector<buffer> {
        if constexpr(std::is_same_v<std::decay_t<Payload>, std::vector<buffer>>) {
            return payloads;
        }
        else {
            return std::vector<buffer>{buffer{std::string{std::forward<Payload>(payloads)}}};
        }
    }(),
    pubopts,
    force_move(props)
}
{}

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PACKET_IMPL_V5_PUBLISH_HPP
