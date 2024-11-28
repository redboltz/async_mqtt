// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_PROPERTY_IPP)
#define ASYNC_MQTT_PACKET_IMPL_PROPERTY_IPP

#include <async_mqtt/protocol/packet/property.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::property {

namespace detail {
template <typename T>
std::ostream& stream_out_direct(std::ostream& o, T const& v) {
    o <<
        "{" <<
        "id:" << v.id() << "," <<
        "val:" << v.val() <<
        "}";
    return o;
}

template <typename T>
std::ostream& stream_out_json_like(std::ostream& o, T const& v) {
    o <<
        "{" <<
        "id:" << v.id() << "," <<
        "val:" << json_like_out(v.val()) <<
        "}";
    return o;
}

template <typename T>
std::ostream& stream_out_int_cast(std::ostream& o, T const& v) {
    o <<
        "{" <<
        "id:" << v.id() << "," <<
        "val:" << static_cast<int>(v.val()) <<
        "}";
    return o;
}

} // namespace detail

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, payload_format_indicator const& v) {
    o <<
        "{" <<
        "id:" << v.id() << "," <<
        "val:" <<
        [&] {
            if (v.val() == payload_format::binary) return "binary";
            return "string";
        }() <<
        "}";
    return o;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, message_expiry_interval const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, content_type const& v) {
    return detail::stream_out_json_like(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, response_topic const& v) {
    return detail::stream_out_json_like(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, correlation_data const& v) {
    return detail::stream_out_json_like(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, subscription_identifier const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, session_expiry_interval const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, assigned_client_identifier const& v) {
    return detail::stream_out_json_like(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, server_keep_alive const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, authentication_method const& v) {
    return detail::stream_out_json_like(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, authentication_data const& v) {
    return detail::stream_out_json_like(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, request_problem_information const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, will_delay_interval const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, request_response_information const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, response_information const& v) {
    return detail::stream_out_json_like(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, server_reference const& v) {
    return detail::stream_out_json_like(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, reason_string const& v) {
    return detail::stream_out_json_like(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, receive_maximum const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, topic_alias_maximum const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, topic_alias const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, maximum_qos const& v) {
    return detail::stream_out_int_cast(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, retain_available const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, user_property const& v) {
    o <<
        "{" <<
        "id:" << v.id() << "," <<
        "key:" << v.key() << "," <<
        "val:" << v.val() <<
        "}";
    return o;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, maximum_packet_size const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, wildcard_subscription_available const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, subscription_identifier_available const& v) {
    return detail::stream_out_direct(o, v);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, shared_subscription_available const& v) {
    return detail::stream_out_direct(o, v);
}

} // namespace async_mqtt::property

#endif // ASYNC_MQTT_PACKET_IMPL_PROPERTY_IPP
