// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ERROR_HPP)
#define ASYNC_MQTT_IMPL_ERROR_HPP

#include <exception>
#include <sstream>
#include <string_view>

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/assert.hpp>
#include <boost/operators.hpp>

#include <async_mqtt/error.hpp>

namespace async_mqtt {

////////////////////////////////////////////////////////////////////////////////

constexpr
char const* mqtt_error_to_string(mqtt_error v) {
    switch (v) {
    case mqtt_error::partial_error_detected:                 return "partial_error_detected";
    case mqtt_error::all_error_detected:                     return "all_error_detected";
    case mqtt_error::packet_identifier_fully_used:           return "packet_identifier_fully_used";
    case mqtt_error::packet_identifier_conflict:             return "packet_identifier_conflict";
    case mqtt_error::packet_not_allowed_to_send:             return "packet_not_allowed_to_send";
    case mqtt_error::packet_not_allowed_to_store:            return "packet_not_allowed_to_store";
    case mqtt_error::packet_not_regulated:                   return "packet_not_regulated";
    default:                                                 return "unknown_mqtt_error";
    }
}

constexpr
char const* connect_return_code_to_string(connect_return_code v) {
    switch(v) {
    case connect_return_code::accepted:                      return "accepted";
    case connect_return_code::unacceptable_protocol_version: return "unacceptable_protocol_version";
    case connect_return_code::identifier_rejected:           return "identifier_rejected";
    case connect_return_code::server_unavailable:            return "server_unavailable";
    case connect_return_code::bad_user_name_or_password:     return "bad_user_name_or_password";
    case connect_return_code::not_authorized:                return "not_authorized";
    default:                                                 return "unknown_connect_return_code";
    }
}

constexpr
char const* suback_return_code_to_string(suback_return_code v) {
    switch (v) {
    case suback_return_code::success_maximum_qos_0: return "success_maximum_qos_0";
    case suback_return_code::success_maximum_qos_1: return "success_maximum_qos_1";
    case suback_return_code::success_maximum_qos_2: return "success_maximum_qos_2";
    case suback_return_code::failure:               return "failure";
    default:                                        return "unknown_suback_return_code";
    }
}

constexpr
char const* connect_reason_code_to_string(connect_reason_code v) {
    switch (v) {
    case connect_reason_code::success:                       return "success";
    case connect_reason_code::unspecified_error:             return "unspecified_error";
    case connect_reason_code::malformed_packet:              return "malformed_packet";
    case connect_reason_code::protocol_error:                return "protocol_error";
    case connect_reason_code::implementation_specific_error: return "implementation_specific_error";
    case connect_reason_code::unsupported_protocol_version:  return "unsupported_protocol_version";
    case connect_reason_code::client_identifier_not_valid:   return "client_identifier_not_valid";
    case connect_reason_code::bad_user_name_or_password:     return "bad_user_name_or_password";
    case connect_reason_code::not_authorized:                return "not_authorized";
    case connect_reason_code::server_unavailable:            return "server_unavailable";
    case connect_reason_code::server_busy:                   return "server_busy";
    case connect_reason_code::banned:                        return "banned";
    case connect_reason_code::bad_authentication_method:     return "bad_authentication_method";
    case connect_reason_code::topic_name_invalid:            return "topic_name_invalid";
    case connect_reason_code::packet_too_large:              return "packet_too_large";
    case connect_reason_code::quota_exceeded:                return "quota_exceeded";
    case connect_reason_code::payload_format_invalid:        return "payload_format_invalid";
    case connect_reason_code::retain_not_supported:          return "retain_not_supported";
    case connect_reason_code::qos_not_supported:             return "qos_not_supported";
    case connect_reason_code::use_another_server:            return "use_another_server";
    case connect_reason_code::server_moved:                  return "server_moved";
    case connect_reason_code::connection_rate_exceeded:      return "connection_rate_exceeded";
    default:                                                 return "unknown_connect_reason_code";
    }
}

constexpr
char const* disconnect_reason_code_to_string(disconnect_reason_code v) {
    switch (v) {
    case disconnect_reason_code::normal_disconnection:                   return "normal_disconnection";
    case disconnect_reason_code::disconnect_with_will_message:           return "disconnect_with_will_message";
    case disconnect_reason_code::unspecified_error:                      return "unspecified_error";
    case disconnect_reason_code::malformed_packet:                       return "malformed_packet";
    case disconnect_reason_code::protocol_error:                         return "protocol_error";
    case disconnect_reason_code::implementation_specific_error:          return "implementation_specific_error";
    case disconnect_reason_code::not_authorized:                         return "not_authorized";
    case disconnect_reason_code::server_busy:                            return "server_busy";
    case disconnect_reason_code::server_shutting_down:                   return "server_shutting_down";
    case disconnect_reason_code::keep_alive_timeout:                     return "keep_alive_timeout";
    case disconnect_reason_code::session_taken_over:                     return "session_taken_over";
    case disconnect_reason_code::topic_filter_invalid:                   return "topic_filter_invalid";
    case disconnect_reason_code::topic_name_invalid:                     return "topic_name_invalid";
    case disconnect_reason_code::receive_maximum_exceeded:               return "receive_maximum_exceeded";
    case disconnect_reason_code::topic_alias_invalid:                    return "topic_alias_invalid";
    case disconnect_reason_code::packet_too_large:                       return "packet_too_large";
    case disconnect_reason_code::message_rate_too_high:                  return "message_rate_too_high";
    case disconnect_reason_code::quota_exceeded:                         return "quota_exceeded";
    case disconnect_reason_code::administrative_action:                  return "administrative_action";
    case disconnect_reason_code::payload_format_invalid:                 return "payload_format_invalid";
    case disconnect_reason_code::retain_not_supported:                   return "retain_not_supported";
    case disconnect_reason_code::qos_not_supported:                      return "qos_not_supported";
    case disconnect_reason_code::use_another_server:                     return "use_another_server";
    case disconnect_reason_code::server_moved:                           return "server_moved";
    case disconnect_reason_code::shared_subscriptions_not_supported:     return "shared_subscriptions_not_supported";
    case disconnect_reason_code::connection_rate_exceeded:               return "connection_rate_exceeded";
    case disconnect_reason_code::maximum_connect_time:                   return "maximum_connect_time";
    case disconnect_reason_code::subscription_identifiers_not_supported: return "subscription_identifiers_not_supported";
    case disconnect_reason_code::wildcard_subscriptions_not_supported:   return "wildcard_subscriptions_not_supported";
    default:                                                             return "unknown_disconnect_reason_code";
    }
}

constexpr
char const* suback_reason_code_to_string(suback_reason_code v) {
    switch (v) {
    case suback_reason_code::granted_qos_0:                          return "granted_qos_0";
    case suback_reason_code::granted_qos_1:                          return "granted_qos_1";
    case suback_reason_code::granted_qos_2:                          return "granted_qos_2";
    case suback_reason_code::unspecified_error:                      return "unspecified_error";
    case suback_reason_code::implementation_specific_error:          return "implementation_specific_error";
    case suback_reason_code::not_authorized:                         return "not_authorized";
    case suback_reason_code::topic_filter_invalid:                   return "topic_filter_invalid";
    case suback_reason_code::packet_identifier_in_use:               return "packet_identifier_in_use";
    case suback_reason_code::quota_exceeded:                         return "quota_exceeded";
    case suback_reason_code::shared_subscriptions_not_supported:     return "shared_subscriptions_not_supported";
    case suback_reason_code::subscription_identifiers_not_supported: return "subscription_identifiers_not_supported";
    case suback_reason_code::wildcard_subscriptions_not_supported:   return "wildcard_subscriptions_not_supported";
    default:                                                         return "unknown_suback_reason_code";
    }
}

constexpr
char const* unsuback_reason_code_to_string(unsuback_reason_code v) {
    switch (v) {
    case unsuback_reason_code::success:                       return "success";
    case unsuback_reason_code::no_subscription_existed:       return "no_subscription_existed";
    case unsuback_reason_code::unspecified_error:             return "unspecified_error";
    case unsuback_reason_code::implementation_specific_error: return "implementation_specific_error";
    case unsuback_reason_code::not_authorized:                return "not_authorized";
    case unsuback_reason_code::topic_filter_invalid:          return "topic_filter_invalid";
    case unsuback_reason_code::packet_identifier_in_use:      return "packet_identifier_in_use";
    default:                                                  return "unknown_unsuback_reason_code";
    }
}

constexpr
char const* puback_reason_code_to_string(puback_reason_code v) {
    switch (v) {
    case puback_reason_code::success:                       return "success";
    case puback_reason_code::no_matching_subscribers:       return "no_matching_subscribers";
    case puback_reason_code::unspecified_error:             return "unspecified_error";
    case puback_reason_code::implementation_specific_error: return "implementation_specific_error";
    case puback_reason_code::not_authorized:                return "not_authorized";
    case puback_reason_code::topic_name_invalid:            return "topic_name_invalid";
    case puback_reason_code::packet_identifier_in_use:      return "packet_identifier_in_use";
    case puback_reason_code::quota_exceeded:                return "quota_exceeded";
    case puback_reason_code::payload_format_invalid:        return "payload_format_invalid";
    default:                                                return "unknown_puback_reason_code";
    }
}

constexpr
char const* pubrec_reason_code_to_string(pubrec_reason_code v) {
    switch (v) {
    case pubrec_reason_code::success:                       return "success";
    case pubrec_reason_code::no_matching_subscribers:       return "no_matching_subscribers";
    case pubrec_reason_code::unspecified_error:             return "unspecified_error";
    case pubrec_reason_code::implementation_specific_error: return "implementation_specific_error";
    case pubrec_reason_code::not_authorized:                return "not_authorized";
    case pubrec_reason_code::topic_name_invalid:            return "topic_name_invalid";
    case pubrec_reason_code::packet_identifier_in_use:      return "packet_identifier_in_use";
    case pubrec_reason_code::quota_exceeded:                return "quota_exceeded";
    case pubrec_reason_code::payload_format_invalid:        return "payload_format_invalid";
    default:                                                return "unknown_pubrec_reason_code";
    }
}

constexpr
char const* pubrel_reason_code_to_string(pubrel_reason_code v) {
    switch (v) {
    case pubrel_reason_code::success:                     return "success";
    case pubrel_reason_code::packet_identifier_not_found: return "packet_identifier_not_found";
    default:                                              return "unknown_pubrel_reason_code";
    }
}

constexpr
char const* pubcomp_reason_code_to_string(pubcomp_reason_code v) {
    switch (v) {
    case pubcomp_reason_code::success:                     return "success";
    case pubcomp_reason_code::packet_identifier_not_found: return "packet_identifier_not_found";
    default:                                               return "unknown_pubcomp_reason_code";
    }
}

constexpr
char const* auth_reason_code_to_string(auth_reason_code v) {
    switch (v) {
    case auth_reason_code::success:                 return "success";
    case auth_reason_code::continue_authentication: return "continue_authentication";
    case auth_reason_code::re_authenticate:         return "re_authenticate";
    default:                                        return "unknown_auth_reason_code";
    }
}

////////////////////////////////////////////////////////////////////////////////

inline
std::ostream& operator<<(std::ostream& o, mqtt_error v)
{
    o << mqtt_error_to_string(v);
    return o;
}

inline
std::ostream& operator<<(std::ostream& o, connect_return_code v)
{
    o << connect_return_code_to_string(v);
    return o;
}

inline
std::ostream& operator<<(std::ostream& o, suback_return_code v)
{
    o << suback_return_code_to_string(v);
    return o;
}

inline
std::ostream& operator<<(std::ostream& o, connect_reason_code v)
{
    o << connect_reason_code_to_string(v);
    return o;
}

inline
std::ostream& operator<<(std::ostream& o, disconnect_reason_code v)
{
    o << disconnect_reason_code_to_string(v);
    return o;
}

inline
std::ostream& operator<<(std::ostream& o, suback_reason_code v)
{
    o << suback_reason_code_to_string(v);
    return o;
}

inline
std::ostream& operator<<(std::ostream& o, unsuback_reason_code v)
{
    o << unsuback_reason_code_to_string(v);
    return o;
}

inline
std::ostream& operator<<(std::ostream& o, puback_reason_code v)
{
    o << puback_reason_code_to_string(v);
    return o;
}

inline
std::ostream& operator<<(std::ostream& o, pubrec_reason_code v)
{
    o << pubrec_reason_code_to_string(v);
    return o;
}

inline
std::ostream& operator<<(std::ostream& o, pubrel_reason_code v)
{
    o << pubrel_reason_code_to_string(v);
    return o;
}

inline
std::ostream& operator<<(std::ostream& o, pubcomp_reason_code v)
{
    o << pubcomp_reason_code_to_string(v);
    return o;
}

inline
std::ostream& operator<<(std::ostream& o, auth_reason_code v)
{
    o << auth_reason_code_to_string(v);
    return o;
}

////////////////////////////////////////////////////////////////////////////////

namespace detail {

class mqtt_error_category : public sys::error_category {
public:
    char const* name() const noexcept override {
        return "async_mqtt_mqtt_error";
    }

    std::string message(int v) const override {
        return mqtt_error_to_string(static_cast<mqtt_error>(v));
    }

    sys::error_condition default_error_condition(int v) const noexcept override {
        return sys::error_condition(v, *this);
    }

    bool failed(int v) const noexcept override {
        return (v & 0x80) != 0;
    }
};

class connect_return_code_category : public sys::error_category {
public:
    char const* name() const noexcept override {
        return "async_mqtt_connect_return_code";
    }

    std::string message(int v) const override {
        return connect_return_code_to_string(static_cast<connect_return_code>(v));
    }

    sys::error_condition default_error_condition(int v) const noexcept override {
        return sys::error_condition(v, *this);
    }

    // MQTT v3.1.1 CONNACK packet code
    // no failed overload here
    // only connect_return_code has different way on numbering
    // 0 is success, otherwise failed
};

class suback_return_code_category : public sys::error_category {
public:
    char const* name() const noexcept override {
        return "async_mqtt_suback_return_code";
    }

    std::string message(int v) const override {
        return suback_return_code_to_string(static_cast<suback_return_code>(v));
    }

    sys::error_condition default_error_condition(int v) const noexcept override {
        return sys::error_condition(v, *this);
    }

    bool failed(int v) const noexcept override {
        return v >= 0x80;
    }
};

class connect_reason_code_category : public sys::error_category {
public:
    char const* name() const noexcept override {
        return "async_mqtt_connect_reason_code";
    }

    std::string message(int v) const override {
        return connect_reason_code_to_string(static_cast<connect_reason_code>(v));
    }

    sys::error_condition default_error_condition(int v) const noexcept override {
        return sys::error_condition(v, *this);
    }

    bool failed(int v) const noexcept override {
        return v >= 0x80;
    }
};

class disconnect_reason_code_category : public sys::error_category {
public:
    char const* name() const noexcept override {
        return "async_mqtt_disconnect_reason_code";
    }

    std::string message(int v) const override {
        return disconnect_reason_code_to_string(static_cast<disconnect_reason_code>(v));
    }

    sys::error_condition default_error_condition(int v) const noexcept override {
        return sys::error_condition(v, *this);
    }

    bool failed(int v) const noexcept override {
        return v >= 0x80;
    }
};

class suback_reason_code_category : public sys::error_category {
public:
    char const* name() const noexcept override {
        return "async_mqtt_suback_reason_code";
    }

    std::string message(int v) const override {
        return suback_reason_code_to_string(static_cast<suback_reason_code>(v));
    }

    sys::error_condition default_error_condition(int v) const noexcept override {
        return sys::error_condition(v, *this);
    }

    bool failed(int v) const noexcept override {
        return v >= 0x80;
    }
};

class unsuback_reason_code_category : public sys::error_category {
public:
    char const* name() const noexcept override {
        return "async_mqtt_unsuback_reason_code";
    }

    std::string message(int v) const override {
        return unsuback_reason_code_to_string(static_cast<unsuback_reason_code>(v));
    }

    sys::error_condition default_error_condition(int v) const noexcept override {
        return sys::error_condition(v, *this);
    }

    bool failed(int v) const noexcept override {
        return v >= 0x80;
    }
};


class puback_reason_code_category : public sys::error_category {
public:
    char const* name() const noexcept override {
        return "async_mqtt_puback_reason_code";
    }

    std::string message(int v) const override {
        return puback_reason_code_to_string(static_cast<puback_reason_code>(v));
    }

    sys::error_condition default_error_condition(int v) const noexcept override {
        return sys::error_condition(v, *this);
    }

    bool failed(int v) const noexcept override {
        return v >= 0x80;
    }
};

class pubrec_reason_code_category : public sys::error_category {
public:
    char const* name() const noexcept override {
        return "async_mqtt_pubrec_reason_code";
    }

    std::string message(int v) const override {
        return pubrec_reason_code_to_string(static_cast<pubrec_reason_code>(v));
    }

    sys::error_condition default_error_condition(int v) const noexcept override {
        return sys::error_condition(v, *this);
    }

    bool failed(int v) const noexcept override {
        return v >= 0x80;
    }
};

class pubrel_reason_code_category : public sys::error_category {
public:
    char const* name() const noexcept override {
        return "async_mqtt_pubrel_reason_code";
    }

    std::string message(int v) const override {
        return pubrel_reason_code_to_string(static_cast<pubrel_reason_code>(v));
    }

    sys::error_condition default_error_condition(int v) const noexcept override {
        return sys::error_condition(v, *this);
    }

    bool failed(int v) const noexcept override {
        return v >= 0x80;
    }
};

class pubcomp_reason_code_category : public sys::error_category {
public:
    char const* name() const noexcept override {
        return "async_mqtt_pubcomp_reason_code";
    }

    std::string message(int v) const override {
        return pubcomp_reason_code_to_string(static_cast<pubcomp_reason_code>(v));
    }

    sys::error_condition default_error_condition(int v) const noexcept override {
        return sys::error_condition(v, *this);
    }

    bool failed(int v) const noexcept override {
        return v >= 0x80;
    }
};

class auth_reason_code_category : public sys::error_category {
public:
    char const* name() const noexcept override {
        return "async_mqtt_auth_reason_code";
    }

    std::string message(int v) const override {
        return auth_reason_code_to_string(static_cast<auth_reason_code>(v));
    }

    sys::error_condition default_error_condition(int v) const noexcept override {
        return sys::error_condition(v, *this);
    }

    bool failed(int v) const noexcept override {
        return v >= 0x80;
    }
};

// Optimization, so that static initialization happens only once
// (reduces thread-safe initialization overhead)
struct all_categories {
    mqtt_error_category mqtt_error;
    connect_return_code_category connect_return_code;
    suback_return_code_category suback_return_code;
    connect_reason_code_category connect_reason_code;
    disconnect_reason_code_category disconnect_reason_code;
    suback_reason_code_category suback_reason_code;
    unsuback_reason_code_category unsuback_reason_code;
    puback_reason_code_category puback_reason_code;
    pubrec_reason_code_category pubrec_reason_code;
    pubrel_reason_code_category pubrel_reason_code;
    pubcomp_reason_code_category pubcomp_reason_code;
    auth_reason_code_category auth_reason_code;

    static const all_categories& get() noexcept {
        static all_categories res;
        return res;
    }
};


} // namespace detail

////////////////////////////////////////////////////////////////////////////////

inline
sys::error_category const& get_mqtt_error_category() {
    return detail::all_categories::get().mqtt_error;
}

inline
sys::error_category const& get_connect_return_code_category() {
    return detail::all_categories::get().connect_return_code;
}

inline
sys::error_category const& get_suback_return_code_category() {
    return detail::all_categories::get().suback_return_code;
}

inline
sys::error_category const& get_connect_reason_code_category() {
    return detail::all_categories::get().connect_reason_code;
}

inline
sys::error_category const& get_disconnect_reason_code_category() {
    return detail::all_categories::get().disconnect_reason_code;
}

inline
sys::error_category const& get_suback_reason_code_category() {
    return detail::all_categories::get().suback_reason_code;
}

inline
sys::error_category const& get_unsuback_reason_code_category() {
    return detail::all_categories::get().unsuback_reason_code;
}

inline
sys::error_category const& get_puback_reason_code_category() {
    return detail::all_categories::get().puback_reason_code;
}

inline
sys::error_category const& get_pubrec_reason_code_category() {
    return detail::all_categories::get().pubrec_reason_code;
}

inline
sys::error_category const& get_pubrel_reason_code_category() {
    return detail::all_categories::get().pubrel_reason_code;
}

inline
sys::error_category const& get_pubcomp_reason_code_category() {
    return detail::all_categories::get().pubcomp_reason_code;
}

inline
sys::error_category const& get_auth_reason_code_category() {
    return detail::all_categories::get().auth_reason_code;
}

////////////////////////////////////////////////////////////////////////////////

inline error_code make_error_code(mqtt_error v) {
    return {static_cast<int>(v), get_mqtt_error_category()};
}

inline error_code make_error_code(connect_return_code v) {
    return {static_cast<int>(v), get_connect_return_code_category()};
}

inline error_code make_error_code(suback_return_code v) {
    return {static_cast<int>(v), get_suback_return_code_category()};
}

inline error_code make_error_code(connect_reason_code v) {
    return {static_cast<int>(v), get_connect_reason_code_category()};
}

inline error_code make_error_code(disconnect_reason_code v) {
    return {static_cast<int>(v), get_disconnect_reason_code_category()};
}

inline error_code make_error_code(suback_reason_code v) {
    return {static_cast<int>(v), get_suback_reason_code_category()};
}

inline error_code make_error_code(unsuback_reason_code v) {
    return {static_cast<int>(v), get_unsuback_reason_code_category()};
}

inline error_code make_error_code(puback_reason_code v) {
    return {static_cast<int>(v), get_puback_reason_code_category()};
}

inline error_code make_error_code(pubrec_reason_code v) {
    return {static_cast<int>(v), get_pubrec_reason_code_category()};
}

inline error_code make_error_code(pubrel_reason_code v) {
    return {static_cast<int>(v), get_pubrel_reason_code_category()};
}

inline error_code make_error_code(pubcomp_reason_code v) {
    return {static_cast<int>(v), get_pubcomp_reason_code_category()};
}

inline error_code make_error_code(auth_reason_code v) {
    return {static_cast<int>(v), get_auth_reason_code_category()};
}

////////////////////////////////////////////////////////////////////////////////

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ERROR_HPP
