// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASYNC_MQTT_ERROR_HPP)
#define ASYNC_MQTT_ASYNC_MQTT_ERROR_HPP

#include <exception>
#include <sstream>
#include <string_view>

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/assert.hpp>
#include <boost/operators.hpp>

namespace async_mqtt {

/**
 * @brief sys is a namespace alias of <a href="https://www.boost.org/libs/system/doc/html/system.html">boost::system</a>.
 *
 */
namespace sys = boost::system;

/**
 * @brief errc is a namespace alias of <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">boost::system::errc</a>.
 *
 */
namespace errc = sys::errc;

/**
 * @brief error_code is a type alias of <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_error_code">boost::system::error_code</a>.
 *
 * #### Errors
 * The following errors could be reported as CompletionHander's error code
 * or exception @ref system_error code().
 *
 * ##### Underlying errors
 * @li Boost.System Error
 * @li Boost.Asio Error
 * @li Boost.Beast Error
 *
 * ##### MQTT reason code based errors
 * @li @ref connect_reason_code
 *     - Error on MQTT handshaking
 *     - @ref connect_reason_code is used not only for error reporting but also as
 *       the field of @ref v5::connack_packet
 * @li @ref disconnect_reason_code
 *     - Error on after MQTT connection is established
 *     - @ref disconnect_reason_code is used not only for error reporting but also as
 *       the field of @ref v5::disconnect_packet
 *
 * ##### MQTT related error but not directly mapping to MQTT packets
 * @li @ref mqtt_error
 *
 */
using error_code = sys::error_code;

/**
 * @brief system_error is a type alias of <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_system_error">boost::system::system_error</a>.
 *
 */
using system_error = sys::system_error;


/**
 * @brief general error code
 *
 */
enum class mqtt_error {
    partial_error_detected                 = 0x0101, ///< Some entries have error on suback/unsuback (not an error)
    packet_enqueued                        = 0x0102, ///< Due to receive_maximum, sent packet is enqueued (not an error)
    all_error_detected                     = 0x0180, ///< All entries have error on suback/unsuback
    packet_identifier_fully_used           = 0x0181, ///< Packet Identifier fully used
    packet_identifier_conflict             = 0x0182, ///< Packet Identifier conflict
    packet_identifier_invalid              = 0x0183, ///< Packet Identifier is invalid (e.g neither allocated nor registered)
    packet_not_allowed_to_send             = 0x0184, ///< Packet is not allowd to be sent
    packet_not_allowed_to_store            = 0x0185, ///< Packet is not allowd to be stored
    packet_not_regulated                   = 0x0186, ///< Packet is not regulated
};

/**
 * @brief make error code
 * @param v target
 * @return mqtt_error string
 *
 */
error_code make_error_code(mqtt_error v);

/**
 * @brief stringize mqtt_error
 * @param v target
 * @return mqtt_error string
 *
 */
constexpr char const* mqtt_error_to_string(mqtt_error v);

/**
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 */
std::ostream& operator<<(std::ostream& o, mqtt_error v);

/**
 * @brief   get gategory of mqtt_error
 * @return  category
 *
 */
sys::error_category const& get_mqtt_error_category();

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief connect return code
 * See https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349256
 *
 */
enum class connect_return_code : std::uint8_t {
    accepted                      = 0, ///< Connection accepted (not an error)
    unacceptable_protocol_version = 1, ///< The Server does not support the level of the MQTT protocol requested by the Client
    identifier_rejected           = 2, ///< The Client identifier is correct UTF-8 but not allowed by the Server
    server_unavailable            = 3, ///< The Network Connection has been made but the MQTT service is unavailable
    bad_user_name_or_password     = 4, ///< The data in the user name or password is malformed
    not_authorized                = 5, ///< The Client is not authorized to connect
};

/**
 * @brief make error code
 * @param v target
 * @return connect_return_code string
 *
 */
error_code make_error_code(connect_return_code v);

/**
 * @brief stringize connect_return_code
 * @param v target
 * @return connect_return_code string
 *
 */
constexpr char const* connect_return_code_to_string(connect_return_code v);

/**
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 */
std::ostream& operator<<(std::ostream& o, connect_return_code v);

/**
 * @brief   get gategory of connect_return_code
 * @return  category
 *
 */
sys::error_category const& get_connect_return_code_category();

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief MQTT suback_return_code
 *
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718071
 *
 */
enum class suback_return_code : std::uint8_t {
    success_maximum_qos_0                  = 0x00, ///< Success with QoS0 (not an error)
    success_maximum_qos_1                  = 0x01, ///< Success with QoS1 (not an error)
    success_maximum_qos_2                  = 0x02, ///< Success with QoS2 (not an error)
    failure                                = 0x80, ///< Failure
};

/**
 * @brief make error code
 * @param v target
 * @return suback_return_code string
 *
 */
error_code make_error_code(suback_return_code v);

/**
 * @brief stringize suback_return_code
 * @param v target
 * @return suback_return_code string
 *
 */
constexpr char const* suback_return_code_to_string(suback_return_code v);

/**
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 */
std::ostream& operator<<(std::ostream& o, suback_return_code v);

/**
 * @brief   get gategory of suback_return_code
 * @return  category
 *
 */
sys::error_category const& get_suback_return_code_category();

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief connect reason code
 * It is reported as CONNECT response via CONNACK packet
 *
 */
enum class connect_reason_code : std::uint8_t {
    success                       = 0x00, ///< Success (not an error)
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
 * @brief make error code
 * @param v target
 * @return connect_reason_code string
 *
 */
error_code make_error_code(connect_reason_code v);

/**
 * @brief stringize connect_reason_code
 * @param v target
 * @return connect_reason_code string
 *
 */
constexpr char const* connect_reason_code_to_string(connect_reason_code v);

/**
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 */
std::ostream& operator<<(std::ostream& o, connect_reason_code v);

/**
 * @brief   get gategory of connect_reason_code
 * @return  category
 *
 */
sys::error_category const& get_connect_reason_code_category();

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief disconnect reason code
 * It is reported via DISCONNECT
 *
 */
enum class disconnect_reason_code : std::uint8_t {
    normal_disconnection                   = 0x00, ///< Normal disconnection (not an error)
    disconnect_with_will_message           = 0x04, ///< Disconnect with Will Message (not an error)
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
 * @brief make error code
 * @param v target
 * @return disconnect_reason_code string
 *
 */
error_code make_error_code(disconnect_reason_code v);

/**
 * @brief stringize disconnect_reason_code
 * @param v target
 * @return disconnect_reason_code string
 *
 */
constexpr char const* disconnect_reason_code_to_string(disconnect_reason_code v);

/**
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 */
std::ostream& operator<<(std::ostream& o, disconnect_reason_code v);

/**
 * @brief   get gategory of disconnect_reason_code
 * @return  category
 *
 */
sys::error_category const& get_disconnect_reason_code_category();

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief suback reason code
 * It is reported as SUBSCRIBE response via SUBNACK packet
 *
 */
enum class suback_reason_code : std::uint8_t {
    granted_qos_0                          = 0x00, ///< Granted QoS 0 (not an error)
    granted_qos_1                          = 0x01, ///< Granted QoS 1 (not an error)
    granted_qos_2                          = 0x02, ///< Granted QoS 2 (not an error)
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
 * @brief make error code
 * @param v target
 * @return suback_reason_code string
 *
 */
error_code make_error_code(suback_reason_code v);

/**
 * @brief stringize suback_reason_code
 * @param v target
 * @return suback_reason_code string
 *
 */
constexpr char const* suback_reason_code_to_string(suback_reason_code v);

/**
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 */
std::ostream& operator<<(std::ostream& o, suback_reason_code v);

/**
 * @brief   get gategory of suback_reason_code
 * @return  category
 *
 */
sys::error_category const& get_suback_reason_code_category();

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief unsuback reason code
 * It is reported as UNSUBSCRIBE response via UNSUBNACK packet
 *
 */
enum class unsuback_reason_code : std::uint8_t {
    success                       = 0x00, ///< Success (not an error)
    no_subscription_existed       = 0x11, ///< No subscription existed (not an error)
    unspecified_error             = 0x80, ///< Unspecified error
    implementation_specific_error = 0x83, ///< Implementation specific error
    not_authorized                = 0x87, ///< Not authorized
    topic_filter_invalid          = 0x8f, ///< Topic Filter invalid
    packet_identifier_in_use      = 0x91, ///< Packet Identifier in use
};

/**
 * @brief make error code
 * @param v target
 * @return unsuback_reason_code string
 *
 */
error_code make_error_code(unsuback_reason_code v);

/**
 * @brief stringize unsuback_reason_code
 * @param v target
 * @return unsuback_reason_code string
 *
 */
constexpr char const* unsuback_reason_code_to_string(unsuback_reason_code v);

/**
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 */
std::ostream& operator<<(std::ostream& o, unsuback_reason_code v);

/**
 * @brief   get gategory of unsuback_reason_code
 * @return  category
 *
 */
sys::error_category const& get_unsuback_reason_code_category();

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief puback reason code
 * It is reported as PUBLISH (QoS1) response via PUBACK packet
 *
 */
enum class puback_reason_code : std::uint8_t {
    success                       = 0x00, ///< Success (not an error)
    no_matching_subscribers       = 0x10, ///< No matching subscribers (not an error)
    unspecified_error             = 0x80, ///< Unspecified error
    implementation_specific_error = 0x83, ///< Implementation specific error
    not_authorized                = 0x87, ///< Not authorized
    topic_name_invalid            = 0x90, ///< Topic Name invalid
    packet_identifier_in_use      = 0x91, ///< Packet identifier in use
    quota_exceeded                = 0x97, ///< Quota exceeded
    payload_format_invalid        = 0x99, ///< Payload format invalid
};

/**
 * @brief make error code
 * @param v target
 * @return puback_reason_code string
 *
 */
error_code make_error_code(puback_reason_code v);

/**
 * @brief stringize puback_reason_code
 * @param v target
 * @return puback_reason_code string
 *
 */
constexpr char const* puback_reason_code_to_string(puback_reason_code v);

/**
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 */
std::ostream& operator<<(std::ostream& o, puback_reason_code v);

/**
 * @brief   get gategory of puback_reason_code
 * @return  category
 *
 */
sys::error_category const& get_puback_reason_code_category();

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief pubrec reason code
 * It is reported as PUBLISH (QoS2) response via PUBREC packet
 *
 */
enum class pubrec_reason_code : std::uint8_t {
    success                       = 0x00, ///< Success (not an error)
    no_matching_subscribers       = 0x10, ///< No matching subscribers (not an error)
    unspecified_error             = 0x80, ///< Unspecified error
    implementation_specific_error = 0x83, ///< Implementation specific error
    not_authorized                = 0x87, ///< Not authorized
    topic_name_invalid            = 0x90, ///< Topic Name invalid
    packet_identifier_in_use      = 0x91, ///< Packet Identifier in use
    quota_exceeded                = 0x97, ///< Quota exceeded
    payload_format_invalid        = 0x99, ///< Payload format invalid
};

/**
 * @brief make error code
 * @param v target
 * @return pubrec_reason_code string
 *
 */
error_code make_error_code(pubrec_reason_code v);

/**
 * @brief stringize pubrec_reason_code
 * @param v target
 * @return pubrec_reason_code string
 *
 */
constexpr char const* pubrec_reason_code_to_string(pubrec_reason_code v);

/**
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 */
std::ostream& operator<<(std::ostream& o, pubrec_reason_code v);

/**
 * @brief   get gategory of pubrec_reason_code
 * @return  category
 *
 */
sys::error_category const& get_pubrec_reason_code_category();

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief pubrel reason code
 * It is reported as PUBREC response via PUBREL packet
 *
 */
enum class pubrel_reason_code : std::uint8_t {
    success                     = 0x00, ///< Success (not an error)
    packet_identifier_not_found = 0x92, ///< Packet Identifier not found
};

/**
 * @brief make error code
 * @param v target
 * @return pubrel_reason_code string
 *
 */
error_code make_error_code(pubrel_reason_code v);

/**
 * @brief stringize pubrel_reason_code
 * @param v target
 * @return pubrel_reason_code string
 *
 */
constexpr char const* pubrel_reason_code_to_string(pubrel_reason_code v);

/**
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 */
std::ostream& operator<<(std::ostream& o, pubrel_reason_code v);

/**
 * @brief   get gategory of pubrel_reason_code
 * @return  category
 *
 */
sys::error_category const& get_pubrel_reason_code_category();

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief pubcomp reason code
 * It is reported as PUBREL response via PUBCOMP packet
 *
 */
enum class pubcomp_reason_code : std::uint8_t {
    success                     = 0x00, ///< Success (not an error)
    packet_identifier_not_found = 0x92, ///< Packet Identifier not found
};

/**
 * @brief make error code
 * @param v target
 * @return pubcomp_reason_code string
 *
 */
error_code make_error_code(pubcomp_reason_code v);

/**
 * @brief stringize pubcomp_reason_code
 * @param v target
 * @return pubcomp_reason_code string
 *
 */
constexpr char const* pubcomp_reason_code_to_string(pubcomp_reason_code v);

/**
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 */
std::ostream& operator<<(std::ostream& o, pubcomp_reason_code v);

/**
 * @brief   get gategory of pubcomp_reason_code
 * @return  category
 *
 */
sys::error_category const& get_pubcomp_reason_code_category();

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief auth reason code
 * It is reported via AUTH packet
 *
 */
enum class auth_reason_code : std::uint8_t {
    success                 = 0x00, ///< Success (not an error)
    continue_authentication = 0x18, ///< Continue authentication (not an error)
    re_authenticate         = 0x19, ///< Re-authenticate (not an error)
};

/**
 * @brief make error code
 * @param v target
 * @return auth_reason_code string
 *
 */
error_code make_error_code(auth_reason_code v);

/**
 * @brief stringize auth_reason_code
 * @param v target
 * @return auth_reason_code string
 *
 */
constexpr char const* auth_reason_code_to_string(auth_reason_code v);

/**
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 */
std::ostream& operator<<(std::ostream& o, auth_reason_code v);

/**
 * @brief   get gategory of auth_reason_code
 * @return  category
 *
 */
sys::error_category const& get_auth_reason_code_category();

} // namespace async_mqtt

////////////////////////////////////////////////////////////////////////////////

namespace boost::system {

template <>
struct is_error_code_enum<async_mqtt::mqtt_error> : public std::true_type {};
template <>
struct is_error_code_enum<async_mqtt::connect_reason_code> : public std::true_type {};
template <>
struct is_error_code_enum<async_mqtt::connect_return_code> : public std::true_type {};
template <>
struct is_error_code_enum<async_mqtt::suback_return_code> : public std::true_type {};
template <>
struct is_error_code_enum<async_mqtt::disconnect_reason_code> : public std::true_type {};
template <>
struct is_error_code_enum<async_mqtt::suback_reason_code> : public std::true_type {};
template <>
struct is_error_code_enum<async_mqtt::unsuback_reason_code> : public std::true_type {};
template <>
struct is_error_code_enum<async_mqtt::puback_reason_code> : public std::true_type {};
template <>
struct is_error_code_enum<async_mqtt::pubrec_reason_code> : public std::true_type {};
template <>
struct is_error_code_enum<async_mqtt::pubrel_reason_code> : public std::true_type {};
template <>
struct is_error_code_enum<async_mqtt::pubcomp_reason_code> : public std::true_type {};
template <>
struct is_error_code_enum<async_mqtt::auth_reason_code> : public std::true_type {};

} // namespace boost::system

#include <async_mqtt/protocol/impl/error.hpp>

#endif // ASYNC_MQTT_ASYNC_MQTT_ERROR_HPP
