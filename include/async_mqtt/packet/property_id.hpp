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

/**
 * @ingroup property
 * @brief MQTT property identifier
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901029
 */
enum class id {
    payload_format_indicator          =  1, ///< Payload Format Indicator
    message_expiry_interval           =  2, ///< Message Expiry Interval
    content_type                      =  3, ///< Content Type
    response_topic                    =  8, ///< Response Topic
    correlation_data                  =  9, ///< Correlation Data
    subscription_identifier           = 11, ///< Subscription Identifier
    session_expiry_interval           = 17, ///< Session Expiry Interval
    assigned_client_identifier        = 18, ///< Assigned Client Identifier
    server_keep_alive                 = 19, ///< Server Keep Alive
    authentication_method             = 21, ///< Authentication Method
    authentication_data               = 22, ///< Authentication Data
    request_problem_information       = 23, ///< Request Problem Information
    will_delay_interval               = 24, ///< Will Delay Interval
    request_response_information      = 25, ///< Request Response Information
    response_information              = 26, ///< Response Information
    server_reference                  = 28, ///< Server Reference
    reason_string                     = 31, ///< Reason String
    receive_maximum                   = 33, ///< Receive Maximum
    topic_alias_maximum               = 34, ///< Topic Alias Maximum
    topic_alias                       = 35, ///< Topic Alias
    maximum_qos                       = 36, ///< Maximum QoS
    retain_available                  = 37, ///< Retain Available
    user_property                     = 38, ///< User Property
    maximum_packet_size               = 39, ///< Maximum Packet Size
    wildcard_subscription_available   = 40, ///< Wildcard Subscription Available
    subscription_identifier_available = 41, ///< Subscription Identifier Available
    shared_subscription_available     = 42, ///< Shared Subscription Available
};

/**
 * @ingroup property
 * @brief stringize packet identifier
 * @param v target
 * @return packet identifier
 */
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

/**
 * @ingroup property
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 */
inline
std::ostream& operator<<(std::ostream& o, id v)
{
    o << id_to_str(v);
    return o;
}

} // namespace property

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PROPERTY_ID_HPP
