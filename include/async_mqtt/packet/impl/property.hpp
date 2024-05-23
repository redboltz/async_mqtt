// Copyright Takatoshi Kondo 2018
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_PROPERTY_HPP)
#define ASYNC_MQTT_PACKET_IMPL_PROPERTY_HPP

#include <async_mqtt/packet/property.hpp>

namespace async_mqtt::property {

inline
payload_format_indicator::payload_format_indicator(payload_format fmt)
    :
    detail::n_bytes_property<1>{
        id::payload_format_indicator,
        {
            [&] {
                if (fmt == payload_format::binary) return char(0);
                return char(1);
            }()
        }
    }
{}

inline
payload_format payload_format_indicator::val() const {
    return
        [&] {
            if (buf_.front() == 0) return payload_format::binary;
            return  payload_format::string;
        } ();
}

template <typename It, typename End>
inline
payload_format_indicator::payload_format_indicator(It b, End e)
    : detail::n_bytes_property<1>{id::payload_format_indicator, b, e} {}


inline
message_expiry_interval::message_expiry_interval(std::uint32_t val)
    : detail::n_bytes_property<4>{id::message_expiry_interval, endian_static_vector(val)} {}

inline
std::uint32_t message_expiry_interval::val() const {
    return endian_load<std::uint32_t>(buf_.data());
}

template <typename It, typename End>
inline
message_expiry_interval::message_expiry_interval(It b, End e)
    : detail::n_bytes_property<4>{id::message_expiry_interval, b, e} {}


inline
content_type::content_type(std::string val)
    : detail::string_property{id::content_type, buffer{force_move(val)}} {}

template <
    typename Buffer,
    std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t>
>
inline
content_type::content_type(Buffer&& val)
    : detail::string_property{id::content_type, std::forward<Buffer>(val)} {}


inline
response_topic::response_topic(std::string val)
    : detail::string_property{id::response_topic, buffer{force_move(val)}} {}

template <
    typename Buffer,
    std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t>
>
inline
response_topic::response_topic(Buffer&& val)
    : detail::string_property{id::response_topic, std::forward<Buffer>(val)} {}


inline
correlation_data::correlation_data(std::string val)
    : detail::binary_property{id::correlation_data, buffer{force_move(val)}} {}

template <
    typename Buffer,
    std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t>
>
inline
correlation_data::correlation_data(Buffer&& val)
    : detail::binary_property{id::correlation_data, std::forward<Buffer>(val)} {}


inline
subscription_identifier::subscription_identifier(std::uint32_t val)
    : detail::variable_property{id::subscription_identifier, val} {}


inline
session_expiry_interval::session_expiry_interval(std::uint32_t val)
    : detail::n_bytes_property<4>{id::session_expiry_interval, endian_static_vector(val)} {}

inline
std::uint32_t session_expiry_interval::val() const {
    return endian_load<std::uint32_t>(buf_.data());
}

template <typename It>
inline
session_expiry_interval::session_expiry_interval(It b, It e)
    : detail::n_bytes_property<4>{id::session_expiry_interval, b, e} {}

inline
assigned_client_identifier::assigned_client_identifier(std::string val)
    : detail::string_property{id::assigned_client_identifier, buffer{force_move(val)}} {}

template <
    typename Buffer,
    std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t>
>
inline
assigned_client_identifier::assigned_client_identifier(Buffer&& val)
    : detail::string_property{id::assigned_client_identifier, std::forward<Buffer>(val)} {}


inline
server_keep_alive::server_keep_alive(std::uint16_t val)
    : detail::n_bytes_property<2>{id::server_keep_alive, endian_static_vector(val)} {}

inline
std::uint16_t server_keep_alive::val() const {
    return endian_load<uint16_t>(buf_.data());
}

template <typename It, typename End>
inline
server_keep_alive::server_keep_alive(It b, End e)
    : detail::n_bytes_property<2>{id::server_keep_alive, b, e} {}


inline
authentication_method::authentication_method(std::string val)
    : detail::string_property{id::authentication_method, buffer{force_move(val)}} {}

template <
    typename Buffer,
    std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t>
>
inline
authentication_method::authentication_method(Buffer&& val)
    : detail::string_property{id::authentication_method, std::forward<Buffer>(val)} {}


inline
authentication_data::authentication_data(std::string val)
    : detail::binary_property{id::authentication_data, buffer{force_move(val)}} {}

template <
    typename Buffer,
    std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t>
>
inline
authentication_data::authentication_data(Buffer&& val)
    : detail::binary_property{id::authentication_data, std::forward<Buffer>(val)} {}


inline
request_problem_information::request_problem_information(bool value)
    : detail::n_bytes_property<1>{
        id::request_problem_information,
        {
            [&] {
                if (value) return char(1);
                return char(0);
            }()
        }
    }
{}

inline
bool request_problem_information::val() const {
    return buf_.front() == 1;
}

template <typename It, typename End>
inline
request_problem_information::request_problem_information(It b, End e)
    : detail::n_bytes_property<1>{id::request_problem_information, b, e} {}


inline
will_delay_interval::will_delay_interval(std::uint32_t val)
    : detail::n_bytes_property<4>{id::will_delay_interval, endian_static_vector(val)} {}

inline
std::uint32_t will_delay_interval::val() const {
    return endian_load<uint32_t>(buf_.data());
}

template <typename It, typename End>
inline
will_delay_interval::will_delay_interval(It b, End e)
    : detail::n_bytes_property<4>{id::will_delay_interval, b, e} {}


inline
request_response_information::request_response_information(bool value)
    : detail::n_bytes_property<1>{
        id::request_response_information,
        {
            [&] {
                if (value) return char(1);
                return char(0);
            }()
        }
    }
{}

inline
bool request_response_information::val() const {
    return buf_.front() == 1;
}

template <typename It, typename End>
inline
request_response_information::request_response_information(It b, End e)
    : detail::n_bytes_property<1>(id::request_response_information, b, e) {}


inline
response_information::response_information(std::string val)
    : detail::string_property{id::response_information, buffer{force_move(val)}} {}

template <
    typename Buffer,
    std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t>
>
inline
response_information::response_information(Buffer&& val)
    : detail::string_property{id::response_information, std::forward<Buffer>(val)} {}


inline
server_reference::server_reference(std::string val)
    : detail::string_property{id::server_reference, buffer{force_move(val)}} {}

template <
    typename Buffer,
    std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t>
>
inline
server_reference::server_reference(Buffer&& val)
    : detail::string_property{id::server_reference, std::forward<Buffer>(val)} {}


inline
reason_string::reason_string(std::string val)
    : detail::string_property{id::reason_string, buffer{force_move(val)}} {}

template <
    typename Buffer,
    std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t>
>
inline
reason_string::reason_string(Buffer&& val)
    : detail::string_property{id::reason_string, std::forward<Buffer>(val)} {}


inline
receive_maximum::receive_maximum(std::uint16_t val)
    : detail::n_bytes_property<2>{id::receive_maximum, endian_static_vector(val)} {
    if (val == 0) {
        throw make_error(
            errc::bad_message,
            "property::receive_maximum value is invalid"
        );
    }
}

inline
std::uint16_t receive_maximum::val() const {
    return endian_load<std::uint16_t>(buf_.data());
}

template <typename It, typename End>
inline
receive_maximum::receive_maximum(It b, End e)
    : detail::n_bytes_property<2>{id::receive_maximum, b, e} {
    if (val() == 0) {
        throw make_error(
            errc::bad_message,
            "property::receive_maximum value is invalid"
        );
    }
}


inline
topic_alias_maximum::topic_alias_maximum(std::uint16_t val)
    : detail::n_bytes_property<2>{id::topic_alias_maximum, endian_static_vector(val)} {}

inline
std::uint16_t topic_alias_maximum::val() const {
    return endian_load<std::uint16_t>(buf_.data());
}

template <typename It, typename End>
inline
topic_alias_maximum::topic_alias_maximum(It b, End e)
    : detail::n_bytes_property<2>{id::topic_alias_maximum, b, e} {}


inline
topic_alias::topic_alias(std::uint16_t val)
    : detail::n_bytes_property<2>{id::topic_alias, endian_static_vector(val)} {}

inline
std::uint16_t topic_alias::val() const {
    return endian_load<std::uint16_t>(buf_.data());
}

template <typename It, typename End>
inline
topic_alias::topic_alias(It b, End e)
    : detail::n_bytes_property<2>(id::topic_alias, b, e) {}


inline
maximum_qos::maximum_qos(qos value)
    : detail::n_bytes_property<1>{id::maximum_qos, {static_cast<char>(value)}} {
    if (value != qos::at_most_once &&
        value != qos::at_least_once) {
        throw make_error(
            errc::bad_message,
            "property::maximum_qos value is invalid"
        );
    }
}

inline
std::uint8_t maximum_qos::val() const {
    return static_cast<std::uint8_t>(buf_.front());
}

template <typename It, typename End>
inline
maximum_qos::maximum_qos(It b, End e)
    : detail::n_bytes_property<1>{id::maximum_qos, b, e} {}


inline
retain_available::retain_available(bool value)
    : detail::n_bytes_property<1>{
          id::retain_available,
          {
              [&] {
                  if (value) return char(1);
                  return char(0);
              }()
          }
      }
{}

inline
bool retain_available::val() const {
    return buf_.front() == 1;
}

template <typename It, typename End>
inline
retain_available::retain_available(It b, End e)
    : detail::n_bytes_property<1>{id::retain_available, b, e} {}

inline
user_property::user_property(std::string key, std::string val)
    : user_property{buffer{force_move(key)}, buffer{force_move(val)}}
{}

inline
std::vector<as::const_buffer> user_property::const_buffer_sequence() const {
    std::vector<as::const_buffer> v;
    v.reserve(num_of_const_buffer_sequence());
    v.emplace_back(as::buffer(&id_, 1));
    v.emplace_back(as::buffer(key_.len.data(), key_.len.size()));
    v.emplace_back(as::buffer(key_.buf));
    v.emplace_back(as::buffer(val_.len.data(), val_.len.size()));
    v.emplace_back(as::buffer(val_.buf));
    return v;
}

inline
property::id user_property::id() const {
    return id_;
}

inline
std::size_t user_property::size() const {
    return
        1 + // id_
        key_.size() +
        val_.size();
}

inline
constexpr std::size_t user_property::num_of_const_buffer_sequence() {
    return
        1 + // header
        2 + // key (len, buf)
        2;  // val (len, buf)
}

inline
std::string user_property::key() const {
    return std::string{key_.buf};
}

inline
std::string user_property::val() const {
    return std::string{val_.buf};
}

inline
constexpr buffer const& user_property::key_as_buffer() const {
    return key_.buf;
}

inline
constexpr buffer const& user_property::val_as_buffer() const {
    return val_.buf;
}

template <
    typename Buffer,
    std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t>
>
inline
user_property::user_property(Buffer&& key, Buffer&& val)
    : key_{std::forward<Buffer>(key)}, val_{std::forward<Buffer>(val)}
{
    if (key_.size() > 0xffff) {
        throw make_error(
            errc::bad_message,
            "property::user_property key length is invalid"
        );
    }
    if (val_.size() > 0xffff) {
        throw make_error(
            errc::bad_message,
            "property::user_property val length is invalid"
        );
    }
}


inline
maximum_packet_size::maximum_packet_size(std::uint32_t val)
    : detail::n_bytes_property<4>{id::maximum_packet_size, endian_static_vector(val)} {
    if (val == 0) {
        throw make_error(
            errc::bad_message,
            "property::maximum_packet_size value is invalid"
        );
    }
}

inline
std::uint32_t maximum_packet_size::val() const {
    return endian_load<std::uint32_t>(buf_.data());
}

template <typename It, typename End>
inline
maximum_packet_size::maximum_packet_size(It b, End e)
    : detail::n_bytes_property<4>{id::maximum_packet_size, b, e} {
    if (val() == 0) {
        throw make_error(
            errc::bad_message,
            "property::maximum_packet_size value is invalid"
        );
    }
}


inline
wildcard_subscription_available::wildcard_subscription_available(bool value)
    : detail::n_bytes_property<1>{
          id::wildcard_subscription_available,
          {
              [&] {
                  if (value) return char(1);
                  return char(0);
              }()
          }
      }
{}

inline
bool wildcard_subscription_available::val() const {
    return buf_.front() == 1;
}

template <typename It, typename End>
inline
wildcard_subscription_available::wildcard_subscription_available(It b, End e)
    : detail::n_bytes_property<1>{id::wildcard_subscription_available, b, e} {}


inline
subscription_identifier_available::subscription_identifier_available(bool value)
    : detail::n_bytes_property<1>{
          id::subscription_identifier_available,
          {
              [&] {
                  if (value) return char(1);
                  return char(0);
              }()
          }
      }
{}

inline
bool subscription_identifier_available::val() const {
    return buf_.front() == 1;
}

template <typename It, typename End>
inline
subscription_identifier_available::subscription_identifier_available(It b, End e)
    : detail::n_bytes_property<1>{id::subscription_identifier_available, b, e} {}


inline
shared_subscription_available::shared_subscription_available(bool value)
    : detail::n_bytes_property<1>{
        id::shared_subscription_available,
        {
            [&] {
                if (value) return char(1);
                return char(0);
            }()
        }
    }
{}

inline
bool shared_subscription_available::val() const {
    return buf_.front() == 1;
}

template <typename It, typename End>
inline
shared_subscription_available::shared_subscription_available(It b, End e)
    : detail::n_bytes_property<1>{id::shared_subscription_available, b, e} {}


template <typename Property>
inline
std::enable_if_t<
    Property::of_ == detail::ostream_format::direct,
    std::ostream&
>
operator<<(std::ostream& o, Property const& p) {
    o <<
        "{" <<
        "id:" << p.id() << "," <<
        "val:" << p.val() <<
        "}";
    return o;
}

template <typename Property>
inline
std::enable_if_t<
    Property::of_ == detail::ostream_format::int_cast,
    std::ostream&
>
operator<<(std::ostream& o, Property const& p) {
    o <<
        "{" <<
        "id:" << p.id() << "," <<
        "val:" << static_cast<int>(p.val()) <<
        "}";
    return o;
}

template <typename Property>
inline
std::enable_if_t<
    Property::of_ == detail::ostream_format::key_val,
    std::ostream&
>
operator<<(std::ostream& o, Property const& p) {
    o <<
        "{" <<
        "id:" << p.id() << "," <<
        "key:" << p.key() << "," <<
        "val:" << p.val() <<
        "}";
    return o;
}

template <typename Property>
inline
std::enable_if_t<
    Property::of_ == detail::ostream_format::binary_string,
    std::ostream&
>
operator<<(std::ostream& o, Property const& p) {
    // Note this only compiles because both strings below are the same length.
    o <<
        "{" <<
        "id:" << p.id() << "," <<
        "val:" <<
        [&] {
            if (p.val() == payload_format::binary) return "binary";
            return "string";
        }() <<
        "}";
    return o;
}

template <typename Property>
inline
std::enable_if_t<
    Property::of_ == detail::ostream_format::json_like,
    std::ostream&
>
operator<<(std::ostream& o, Property const& p) {
    o <<
        "{" <<
        "id:" << p.id() << "," <<
        "val:" << json_like_out(p.val()) <<
        "}";
    return o;
}

} // namespace async_mqtt::property

#endif // ASYNC_MQTT_PACKET_PROPERTY_HPP
