// Copyright Takatoshi Kondo 2018
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PROPERTY_ID_HPP)
#define ASYNC_MQTT_PACKET_PROPERTY_ID_HPP

#include <cstdint>
#include <ostream>

namespace async_mqtt {

namespace property {

enum class id {
    payload_format_indicator          =  1,
    message_expiry_interval           =  2,
    content_type                      =  3,
    response_topic                    =  8,
    correlation_data                  =  9,
    subscription_identifier           = 11,
    session_expiry_interval           = 17,
    assigned_client_identifier        = 18,
    server_keep_alive                 = 19,
    authentication_method             = 21,
    authentication_data               = 22,
    request_problem_information       = 23,
    will_delay_interval               = 24,
    request_response_information      = 25,
    response_information              = 26,
    server_reference                  = 28,
    reason_string                     = 31,
    receive_maximum                   = 33,
    topic_alias_maximum               = 34,
    topic_alias                       = 35,
    maximum_qos                       = 36,
    retain_available                  = 37,
    user_property                     = 38,
    maximum_packet_size               = 39,
    wildcard_subscription_available   = 40,
    subscription_identifier_available = 41,
    shared_subscription_available     = 42,
};

constexpr char const* id_to_str(id v) {
    switch (v) {
    case id::payload_format_indicator:           return "payload_format_indicator";
    case id::message_expiry_interval:            return "message_expiry_interval";
    case id::content_type:                       return "content_type";
    case id::response_topic:                     return "response_topic";
    case id::correlation_data:                   return "correlation_data";
    case id::subscription_identifier:            return "subscription_identifier";
    case id::session_expiry_interval:            return "session_expiry_interval";
    case id::assigned_client_identifier:         return "assigned_client_identifier";
    case id::server_keep_alive:                  return "server_keep_alive";
    case id::authentication_method:              return "authentication_method";
    case id::authentication_data:                return "authentication_data";
    case id::request_problem_information:        return "request_problem_information";
    case id::will_delay_interval:                return "will_delay_interval";
    case id::request_response_information:       return "request_response_information";
    case id::response_information:               return "response_information";
    case id::server_reference:                   return "server_reference";
    case id::reason_string:                      return "reason_string";
    case id::receive_maximum:                    return "receive_maximum";
    case id::topic_alias_maximum:                return "topic_alias_maximum";
    case id::topic_alias:                        return "topic_alias";
    case id::maximum_qos:                        return "maximum_qos";
    case id::retain_available:                   return "retain_available";
    case id::user_property:                      return "user_property";
    case id::maximum_packet_size:                return "maximum_packet_size";
    case id::wildcard_subscription_available:    return "wildcard_subscription_available";
    case id::subscription_identifier_available:  return "subscription_identifier_available";
    case id::shared_subscription_available:      return "shared_subscription_available";
    default:                                     return "invalid id";
    }
}

inline
std::ostream& operator<<(std::ostream& os, id val)
{
    os << id_to_str(val);
    return os;
}

} // namespace property

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PROPERTY_ID_HPP
