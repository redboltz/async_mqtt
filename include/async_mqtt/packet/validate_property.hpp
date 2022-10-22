// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_VALIDATE_PROPERTY_HPP)
#define ASYNC_MQTT_PACKET_VALIDATE_PROPERTY_HPP


#include <async_mqtt/packet/property_id.hpp>

namespace async_mqtt {

enum class property_location {
    connect,
    will,
    connack,
    publish,
    puback,
    pubrec,
    pubrel,
    pubcomp,
    subscribe,
    suback,
    unsubscribe,
    unsuback,
    disconnect,
    auth
};

constexpr char const* property_location_to_str(property_location v) {
    switch (v) {
    case property_location::connect:     return "connect";
    case property_location::will:        return "will";
    case property_location::connack:     return "connack";
    case property_location::publish:     return "publish";
    case property_location::puback:      return "puback";
    case property_location::pubrec:      return "pubrec";
    case property_location::pubrel:      return "pubrel";
    case property_location::pubcomp:     return "pubcomp";
    case property_location::subscribe:   return "subscribe";
    case property_location::suback:      return "suback";
    case property_location::unsubscribe: return "unsubscribe";
    case property_location::unsuback:    return "unsuback";
    case property_location::disconnect:  return "disconnect";
    case property_location::auth:        return "auth";
    default:                             return "invalid property location";
    }
}

constexpr bool validate_property(property_location loc, property::id id) {
    switch (loc) {
    case property_location::connect:
        switch (id) {
        case property::id::session_expiry_interval:
        case property::id::authentication_method:
        case property::id::authentication_data:
        case property::id::request_problem_information:
        case property::id::request_response_information:
        case property::id::receive_maximum:
        case property::id::topic_alias_maximum:
        case property::id::user_property:
        case property::id::maximum_packet_size:
            return true;
        default:
            return false;
        }
        break;
    case property_location::will:
        switch (id) {
        case property::id::payload_format_indicator:
        case property::id::message_expiry_interval:
        case property::id::content_type:
        case property::id::response_topic:
        case property::id::correlation_data:
        case property::id::will_delay_interval:
        case property::id::user_property:
            return true;
        default:
            return false;
        }
        break;
    case property_location::connack:
        switch (id) {
        case property::id::session_expiry_interval:
        case property::id::assigned_client_identifier:
        case property::id::server_keep_alive:
        case property::id::authentication_method:
        case property::id::authentication_data:
        case property::id::response_information:
        case property::id::server_reference:
        case property::id::reason_string:
        case property::id::receive_maximum:
        case property::id::topic_alias_maximum:
        case property::id::maximum_qos:
        case property::id::retain_available:
        case property::id::user_property:
        case property::id::maximum_packet_size:
        case property::id::wildcard_subscription_available:
        case property::id::subscription_identifier_available:
        case property::id::shared_subscription_available:
            return true;
        default:
            return false;
        }
        break;
    case property_location::publish:
        switch (id) {
        case property::id::payload_format_indicator:
        case property::id::message_expiry_interval:
        case property::id::content_type:
        case property::id::response_topic:
        case property::id::correlation_data:
        case property::id::subscription_identifier:
        case property::id::topic_alias:
        case property::id::user_property:
            return true;
        default:
            return false;
        }
        break;
    case property_location::puback:
        switch (id) {
        case property::id::reason_string:
        case property::id::user_property:
            return true;
        default:
            return false;
        }
        break;
    case property_location::pubrec:
        switch (id) {
        case property::id::reason_string:
        case property::id::user_property:
            return true;
        default:
            return false;
        }
        break;
    case property_location::pubrel:
        switch (id) {
        case property::id::reason_string:
        case property::id::user_property:
            return true;
        default:
            return false;
        }
        break;
    case property_location::pubcomp:
        switch (id) {
        case property::id::reason_string:
        case property::id::user_property:
            return true;
        default:
            return false;
        }
        break;
    case property_location::subscribe:
        switch (id) {
        case property::id::subscription_identifier:
        case property::id::user_property:
            return true;
        default:
            return false;
        }
        break;
    case property_location::suback:
        switch (id) {
        case property::id::reason_string:
        case property::id::user_property:
            return true;
        default:
            return false;
        }
        break;
    case property_location::unsubscribe:
        switch (id) {
        case property::id::user_property:
            return true;
        default:
            return false;
        }
        break;
    case property_location::unsuback:
        switch (id) {
        case property::id::reason_string:
        case property::id::user_property:
            return true;
        default:
            return false;
        }
        break;
    case property_location::disconnect:
        switch (id) {
        case property::id::session_expiry_interval:
        case property::id::server_reference:
        case property::id::reason_string:
        case property::id::user_property:
            return true;
        default:
            return false;
        }
        break;
    case property_location::auth:
        switch (id) {
        case property::id::authentication_method:
        case property::id::authentication_data:
        case property::id::reason_string:
        case property::id::user_property:
            return true;
        default:
            return false;
        }
        break;
    default:
        break;
    }
    return false;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_VALIDATE_PROPERTY_HPP
