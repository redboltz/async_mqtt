// Copyright Takatoshi Kondo 2018
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_REASON_CODE_HPP)
#define ASYNC_MQTT_PACKET_REASON_CODE_HPP

#include <cstdint>
#include <ostream>

#include <async_mqtt/packet/subopts.hpp>

namespace async_mqtt {

/**
 * @ingroup connack_v5
 * @brief MQTT connect_reason_code
 *
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718071
 */
enum class connect_reason_code : std::uint8_t {
    success                       = 0x00, ///< Success
    unspecified_error             = 0x80, ///< Unspecified error
    malformed_packet              = 0x81, ///< Malformed Packet
    protocol_error                = 0x82, ///< Protocol Error
    implementation_specific_error = 0x83, ///< Implementation specific error
    unsupported_protocol_version  = 0x84, ///< Unsupported Protocol Version
    client_identifier_not_valid   = 0x85, ///< Client Identifier not valid
    bad_user_name_or_password     = 0x86, ///< Bad User Name or Password
    not_authorized                = 0x87, ///< Not authorized
    server_unavailable            = 0x88, ///< Server unavailable
    server_busy                   = 0x89, ///< Server busy
    banned                        = 0x8a, ///< Banned
    bad_authentication_method     = 0x8c, ///< Bad authentication method
    topic_name_invalid            = 0x90, ///< Topic Name invalid
    packet_too_large              = 0x95, ///< Packet too large
    quota_exceeded                = 0x97, ///< Quota exceeded
    payload_format_invalid        = 0x99, ///< Payload format invalid
    retain_not_supported          = 0x9a, ///< Retain not supported
    qos_not_supported             = 0x9b, ///< QoS not supported
    use_another_server            = 0x9c, ///< Use another server
    server_moved                  = 0x9d, ///< Server moved
    connection_rate_exceeded      = 0x9f, ///< Connection rate exceeded
};

/**
 * @ingroup connack_v5
 * @brief stringize connect_reason_code
 * @param v target
 * @return connect_reason_code string
 */
constexpr
char const* connect_reason_code_to_str(connect_reason_code v) {
    switch(v)
    {
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

/**
 * @ingroup connack_v5
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 */
inline
std::ostream& operator<<(std::ostream& o, connect_reason_code v)
{
    o << connect_reason_code_to_str(v);
    return o;
}

/**
 * @ingroup disconnect_v5
 * @brief MQTT disconnect_reason_code
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901208
 */
enum class disconnect_reason_code : std::uint8_t {
    normal_disconnection                   = 0x00, ///< Normal disconnection
    disconnect_with_will_message           = 0x04, ///< Disconnect with Will Message
    unspecified_error                      = 0x80, ///< Unspecified error
    malformed_packet                       = 0x81, ///< Malformed Packet
    protocol_error                         = 0x82, ///< Protocol Error
    implementation_specific_error          = 0x83, ///< Implementation specific error
    not_authorized                         = 0x87, ///< Not authorized
    server_busy                            = 0x89, ///< Server busy
    server_shutting_down                   = 0x8b, ///< Server shutting down
    keep_alive_timeout                     = 0x8d, ///< Keep Alive timeout
    session_taken_over                     = 0x8e, ///< Session taken over
    topic_filter_invalid                   = 0x8f, ///< Topic Filter invalid
    topic_name_invalid                     = 0x90, ///< Topic Name invalid
    receive_maximum_exceeded               = 0x93, ///< Receive Maximum exceeded
    topic_alias_invalid                    = 0x94, ///< Topic Alias invalid
    packet_too_large                       = 0x95, ///< Packet too large
    message_rate_too_high                  = 0x96, ///< Message rate too high
    quota_exceeded                         = 0x97, ///< Quota exceeded
    administrative_action                  = 0x98, ///< Administrative action
    payload_format_invalid                 = 0x99, ///< Payload format invalid
    retain_not_supported                   = 0x9a, ///< Retain not supported
    qos_not_supported                      = 0x9b, ///< QoS not supported
    use_another_server                     = 0x9c, ///< Use another server
    server_moved                           = 0x9d, ///< Server moved
    shared_subscriptions_not_supported     = 0x9e, ///< Shared Subscriptions not supported
    connection_rate_exceeded               = 0x9f, ///< Connection rate exceeded
    maximum_connect_time                   = 0xa0, ///< Maximum connect time
    subscription_identifiers_not_supported = 0xa1, ///< Subscription Identifiers not supported
    wildcard_subscriptions_not_supported   = 0xa2, ///< Wildcard Subscriptions not supported
};

/**
 * @ingroup disconnect_v5
 * @brief stringize disconnect_reason_code
 * @param v target
 * @return disconnect_reason_code string
 */
constexpr
char const* disconnect_reason_code_to_str(disconnect_reason_code v) {
    switch(v)
    {
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

/**
 * @ingroup disconnect_v5
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 */
inline
std::ostream& operator<<(std::ostream& o, disconnect_reason_code v)
{
    o << disconnect_reason_code_to_str(v);
    return o;
}


/**
 * @ingroup suback_v5
 * @brief MQTT suback_reason_code
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901178
 */
enum class suback_reason_code : std::uint8_t {
    granted_qos_0                          = 0x00, ///< Granted QoS 0
    granted_qos_1                          = 0x01, ///< Granted QoS 1
    granted_qos_2                          = 0x02, ///< Granted QoS 2
    unspecified_error                      = 0x80, ///< Unspecified error
    implementation_specific_error          = 0x83, ///< Implementation specific error
    not_authorized                         = 0x87, ///< Not authorized
    topic_filter_invalid                   = 0x8f, ///< Topic Filter invalid
    packet_identifier_in_use               = 0x91, ///< Packet Identifier in use
    quota_exceeded                         = 0x97, ///< Quota exceeded
    shared_subscriptions_not_supported     = 0x9e, ///< Shared Subscriptions not supported
    subscription_identifiers_not_supported = 0xa1, ///< Subscription Identifiers not supported
    wildcard_subscriptions_not_supported   = 0xa2, ///< Wildcard Subscriptions not supported
};

/**
 * @ingroup suback_v5
 * @brief stringize suback_reason_code
 * @param v target
 * @return suback_reason_code string
 */
constexpr
char const* suback_reason_code_to_str(suback_reason_code v) {
    switch(v)
    {
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

/**
 * @ingroup suback_v5
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 */
inline
std::ostream& operator<<(std::ostream& o, suback_reason_code v)
{
    o << suback_reason_code_to_str(v);
    return o;
}

/**
 * @ingroup suback_v5
 * @brief create suback_reason_code corresponding to the QoS
 * @param q QoS
 * @return suback_reason_code
 */
constexpr suback_reason_code qos_to_suback_reason_code(qos q) {
    return static_cast<suback_reason_code>(q);
}

/**
 * @ingroup unsuback_v5
 * @brief MQTT unsuback_reason_code
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901194
 */
enum class unsuback_reason_code : std::uint8_t {
    success                       = 0x00, ///< Success
    no_subscription_existed       = 0x11, ///< No subscription existed
    unspecified_error             = 0x80, ///< Unspecified error
    implementation_specific_error = 0x83, ///< Implementation specific error
    not_authorized                = 0x87, ///< Not authorized
    topic_filter_invalid          = 0x8f, ///< Topic Filter invalid
    packet_identifier_in_use      = 0x91, ///< Packet Identifier in use
};

/**
 * @ingroup unsuback_v5
 * @brief stringize unsuback_reason_code
 * @param v target
 * @return unsuback_reason_code string
 */
constexpr
char const* unsuback_reason_code_to_str(unsuback_reason_code v) {
    switch(v)
    {
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

/**
 * @ingroup unsuback_v5
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 */
inline
std::ostream& operator<<(std::ostream& o, unsuback_reason_code v)
{
    o << unsuback_reason_code_to_str(v);
    return o;
}

/**
 * @ingroup puback_v5
 * @brief MQTT puback_reason_code
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901124
 */
enum class puback_reason_code : std::uint8_t {
    success                       = 0x00, ///< Success
    no_matching_subscribers       = 0x10, ///< No matching subscribers
    unspecified_error             = 0x80, ///< Unspecified error
    implementation_specific_error = 0x83, ///< Implementation specific error
    not_authorized                = 0x87, ///< Not authorized
    topic_name_invalid            = 0x90, ///< Topic Name invalid
    packet_identifier_in_use      = 0x91, ///< Packet identifier in use
    quota_exceeded                = 0x97, ///< Quota exceeded
    payload_format_invalid        = 0x99, ///< Payload format invalid
};

/**
 * @ingroup puback_v5
 * @brief stringize puback_reason_code
 * @param v target
 * @return puback_reason_code string
 */
constexpr
char const* puback_reason_code_to_str(puback_reason_code v) {
    switch(v)
    {
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

/**
 * @ingroup puback_v5
 * @brief check reason code error
 * @param v  target
 * @return true if the reason code is error, otherwise false
 */
constexpr
bool is_error(puback_reason_code v) {
    return static_cast<std::uint8_t>(v) >= 0x80;
}

/**
 * @ingroup puback_v5
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 */
inline
std::ostream& operator<<(std::ostream& o, puback_reason_code v)
{
    o << puback_reason_code_to_str(v);
    return o;
}

/**
 * @ingroup pubrec_v5
 * @brief MQTT pubrec_reason_code
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901134
 */
enum class pubrec_reason_code : std::uint8_t {
    success                       = 0x00, ///< Success
    no_matching_subscribers       = 0x10, ///< No matching subscribers.
    unspecified_error             = 0x80, ///< Unspecified error
    implementation_specific_error = 0x83, ///< Implementation specific error
    not_authorized                = 0x87, ///< Not authorized
    topic_name_invalid            = 0x90, ///< Topic Name invalid
    packet_identifier_in_use      = 0x91, ///< Packet Identifier in use
    quota_exceeded                = 0x97, ///< Quota exceeded
    payload_format_invalid        = 0x99, ///< Payload format invalid
};

/**
 * @ingroup pubrec_v5
 * @brief stringize pubrec_reason_code
 * @param v target
 * @return pubrec_reason_code string
 */
constexpr
char const* pubrec_reason_code_to_str(pubrec_reason_code v) {
    switch(v)
    {
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

/**
 * @ingroup pubrec_v5
 * @brief check reason code error
 * @param v  target
 * @return true if the reason code is error, otherwise false
 */
constexpr
bool is_error(pubrec_reason_code v) {
    return static_cast<std::uint8_t>(v) >= 0x80;
}

/**
 * @ingroup pubrec_v5
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 */
inline
std::ostream& operator<<(std::ostream& o, pubrec_reason_code v)
{
    o << pubrec_reason_code_to_str(v);
    return o;
}

/**
 * @ingroup pubrel_v5
 * @brief MQTT pubrel_reason_code
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901144
 */
enum class pubrel_reason_code : std::uint8_t {
    success                     = 0x00, ///< Success
    packet_identifier_not_found = 0x92, ///< Packet Identifier not found
};

/**
 * @ingroup pubrel_v5
 * @brief stringize pubrel_reason_code
 * @param v target
 * @return pubrel_reason_code string
 */
constexpr
char const* pubrel_reason_code_to_str(pubrel_reason_code v) {
    switch(v)
    {
    case pubrel_reason_code::success:                      return "success";
    case pubrel_reason_code::packet_identifier_not_found:  return "packet_identifier_not_found";
    default:                                               return "unknown_pubrel_reason_code";
    }
}

/**
 * @ingroup pubrel_v5
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 */
inline
std::ostream& operator<<(std::ostream& o, pubrel_reason_code v)
{
    o << pubrel_reason_code_to_str(v);
    return o;
}

/**
 * @ingroup pubcomp_v5
 * @brief MQTT pubcomp_reason_code
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901154
 */
enum class pubcomp_reason_code : std::uint8_t {
    success                     = 0x00, ///< Success
    packet_identifier_not_found = 0x92, ///< Packet Identifier not found
};

/**
 * @ingroup pubcomp_v5
 * @brief stringize pubcomp_reason_code
 * @param v target
 * @return pubcomp_reason_code string
 */
constexpr
char const* pubcomp_reason_code_to_str(pubcomp_reason_code v) {
    switch(v)
    {
    case pubcomp_reason_code::success:                      return "success";
    case pubcomp_reason_code::packet_identifier_not_found:  return "packet_identifier_not_found";
    default:                                                return "unknown_pubcomp_reason_code";
    }
}

/**
 * @ingroup pubcomp_v5
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 */
inline
std::ostream& operator<<(std::ostream& o, pubcomp_reason_code v)
{
    o << pubcomp_reason_code_to_str(v);
    return o;
}

/**
 * @ingroup auth_v5
 * @brief MQTT auth_reason_code
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901220
 */
enum class auth_reason_code : std::uint8_t {
    success                 = 0x00, ///< Success
    continue_authentication = 0x18, ///< Continue authentication
    re_authenticate         = 0x19, ///< Re-authenticate
};

/**
 * @ingroup auth_v5
 * @brief stringize auth_reason_code
 * @param v target
 * @return auth_reason_code string
 */
constexpr
char const* auth_reason_code_to_str(auth_reason_code v) {
    switch(v)
    {
    case auth_reason_code::success:                 return "success";
    case auth_reason_code::continue_authentication: return "continue_authentication";
    case auth_reason_code::re_authenticate:         return "re_authenticate";
    default:                                        return "unknown_auth_reason_code";
    }
}

/**
 * @ingroup auth_v5
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 */
inline
std::ostream& operator<<(std::ostream& o, auth_reason_code v)
{
    o << auth_reason_code_to_str(v);
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_REASON_CODE_HPP
