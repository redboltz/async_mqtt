// Copyright Takatoshi Kondo 2018
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PROPERTY_HPP)
#define ASYNC_MQTT_PACKET_PROPERTY_HPP

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <numeric>
#include <iosfwd>
#include <iomanip>

#include <boost/asio/buffer.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/operators.hpp>

#include <async_mqtt/util/json_like_out.hpp>

#include <async_mqtt/packet/qos.hpp>
#include <async_mqtt/packet/detail/base_property.hpp>

/**
 * @defgroup property Property for MQTT v5.0 packets
 * @ingroup packet_v5
 */

/**
 * @defgroup property_internal implementation class
 * @ingroup property
 */

namespace async_mqtt {

namespace as = boost::asio;

// forward declarations
class property_variant;
enum class property_location;
property_variant make_property_variant(buffer& buf, property_location loc, error_code& ec);

/**
 * @ingroup property
 * @brief payload_format
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
enum class payload_format {
    binary, ///< binary
    string  ///< string
};

/**
 * @ingroup property
 * @brief type of the session expiry interval (seconds)
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
using session_expiry_interval_type = std::uint32_t;

/**
 * @ingroup property
 * @brief type of the topic alias value
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
using topic_alias_type = std::uint16_t;

/**
 * @ingroup property
 * @brief type of the receive maximum value
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
using receive_maximum_type = std::uint16_t;

/**
 * @ingroup property
 * @brief the special session_expiry_interval value that session is never expire.
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
static constexpr session_expiry_interval_type session_never_expire = 0xffffffffUL;

/**
 * @ingroup property
 * @brief the maximum topic_alias value
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
static constexpr topic_alias_type topic_alias_max = 0xffff;

/**
 * @ingroup property
 * @brief the maximum receive_maximum value
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
static constexpr receive_maximum_type receive_maximum_max = 0xffff;

/**
 * @ingroup property
 * @brief the maximum maximum_packet_size value
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
static constexpr std::uint32_t packet_size_no_limit =
    1 + // fixed header
    4 + // remaining length
    128 * 128 * 128 * 128; // maximum value of remainin length

namespace property {

/**
 * @ingroup property
 * @brief payload_format_indicator property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class payload_format_indicator : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param fmt payload_format
     */
    explicit payload_format_indicator(payload_format fmt = payload_format::binary);

    /**
     * @brief Get value
     * @return payload_format
     */
    payload_format val() const;

private:

    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit payload_format_indicator(It b, End e, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, payload_format_indicator const& v);

/**
 * @ingroup property
 * @brief message_expiry_interval property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class message_expiry_interval : public detail::n_bytes_property<4> {
public:
    /**
     * @brief constructor
     * @param val message_expiry_interval seconds
     */
    explicit message_expiry_interval(std::uint32_t val);

    /**
     * @brief Get value
     * @return message_expiry_interval seconds
     */
    std::uint32_t val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit message_expiry_interval(It b, End e);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, message_expiry_interval const& v);

/**
 * @ingroup property
 * @brief content_type property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class content_type : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val content type string
     */
    explicit content_type(std::string val);

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit content_type(Buffer&& val, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, content_type const& v);

/**
 * @ingroup property
 * @brief response_topic property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class response_topic : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val response_topic string
     */
    explicit response_topic(std::string val);

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit response_topic(Buffer&& val, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, response_topic const& v);

/**
 * @ingroup property
 * @brief correlation_data property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class correlation_data : public detail::binary_property {
public:
    /**
     * @brief constructor
     * @param val correlation_data string
     */
    explicit correlation_data(std::string val);

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit correlation_data(Buffer&& val, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, correlation_data const& v);

/**
 * @ingroup property
 * @brief subscription_identifier property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class subscription_identifier : public detail::variable_property {
public:
    /**
     * @brief constructor
     * @param val subscription_identifier
     */
    explicit subscription_identifier(std::uint32_t val);

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    explicit subscription_identifier(std::uint32_t val, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, subscription_identifier const& v);

/**
 * @ingroup property
 * @brief session_expiry_interval property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class session_expiry_interval : public detail::n_bytes_property<4> {
public:
    /**
     * @brief constructor
     * @param val session_expiry_interval seconds
     */
    session_expiry_interval(std::uint32_t val);

    /**
     * @brief Get value
     * @return session_expiry_interval seconds
     */
    std::uint32_t val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It>
    explicit session_expiry_interval(It b, It e);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, session_expiry_interval const& v);

/**
 * @ingroup property
 * @brief assigned_client_identifier property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class assigned_client_identifier : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val assigned_client_identifier string
     */
    explicit assigned_client_identifier(std::string val);

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit assigned_client_identifier(Buffer&& val, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, assigned_client_identifier const& v);

/**
 * @ingroup property
 * @brief server_keep_alive property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class server_keep_alive : public detail::n_bytes_property<2> {
public:
    /**
     * @brief constructor
     * @param val server_keep_alive seconds
     */
    explicit server_keep_alive(std::uint16_t val);

    /**
     * @brief Get value
     * @return server_keep_alive seconds
     */
    std::uint16_t val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit server_keep_alive(It b, End e);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, server_keep_alive const& v);

/**
 * @ingroup property
 * @brief authentication_method property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class authentication_method : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val authentication_method string
     */
    explicit authentication_method(std::string val);

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit authentication_method(Buffer&& val, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, authentication_method const& v);

/**
 * @ingroup property
 * @brief authentication_data property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class authentication_data : public detail::binary_property {
public:
    /**
     * @brief constructor
     * @param val authentication_data string
     */
    explicit authentication_data(std::string val);

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit authentication_data(Buffer&& val, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, authentication_data const& v);

/**
 * @ingroup property
 * @brief request_problem_information property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class request_problem_information : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param value request_problem_information
     */
    explicit request_problem_information(bool value);

    /**
     * @brief Get value
     * @return value
     */
    bool val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit request_problem_information(It b, End e);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, request_problem_information const& v);

/**
 * @ingroup property
 * @brief will_delay_interval property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class will_delay_interval : public detail::n_bytes_property<4> {
public:
    /**
     * @brief constructor
     * @param val will_delay_interval seconds
     */
    explicit will_delay_interval(std::uint32_t val);

    /**
     * @brief Get value
     * @return will_delay_interval seconds
     */
    std::uint32_t val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit will_delay_interval(It b, End e);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, will_delay_interval const& v);

/**
 * @ingroup property
 * @brief request_response_information property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class request_response_information : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param value request_response_information
     */
    explicit request_response_information(bool value);

    /**
     * @brief Get value
     * @return value
     */
    bool val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit request_response_information(It b, End e);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, request_response_information const& v);

/**
 * @ingroup property
 * @brief response_information property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class response_information : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val response_information string
     */
    explicit response_information(std::string val);

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit response_information(Buffer&& val, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, response_information const& v);

/**
 * @ingroup property
 * @brief server_reference property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class server_reference : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val server_reference string
     */
    explicit server_reference(std::string val);

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit server_reference(Buffer&& val, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, server_reference const& v);

/**
 * @ingroup property
 * @brief reason_string property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class reason_string : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val reason_string
     */
    explicit reason_string(std::string val);

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit reason_string(Buffer&& val, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, reason_string const& v);

/**
 * @ingroup property
 * @brief receive_maximum property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class receive_maximum : public detail::n_bytes_property<2> {
public:
    /**
     * @brief constructor
     * @param val receive_maximum
     */
    explicit receive_maximum(std::uint16_t val);

    /**
     * @brief Get value
     * @return value
     */
    std::uint16_t val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit receive_maximum(It b, End e, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, receive_maximum const& v);

/**
 * @ingroup property
 * @brief topic_alias_maximum property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class topic_alias_maximum : public detail::n_bytes_property<2> {
public:
    /**
     * @brief constructor
     * @param val topic_alias_maximum
     */
    explicit topic_alias_maximum(std::uint16_t val);

    /**
     * @brief Get value
     * @return value
     */
    std::uint16_t val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit topic_alias_maximum(It b, End e);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, topic_alias_maximum const& v);

/**
 * @ingroup property
 * @brief topic_alias property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class topic_alias : public detail::n_bytes_property<2> {
public:
    /**
     * @brief constructor
     * @param val topic_alias
     */
    explicit topic_alias(std::uint16_t val);

    /**
     * @brief Get value
     * @return value
     */
    std::uint16_t val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit topic_alias(It b, End e);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, topic_alias const& v);

/**
 * @ingroup property
 * @brief maximum_qos property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class maximum_qos : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param value maximum_qos
     */
    explicit maximum_qos(qos value);

    /**
     * @brief Get value
     * @return value
     */
    qos val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit maximum_qos(It b, End e, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, maximum_qos const& v);

/**
 * @ingroup property
 * @brief retain_available property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class retain_available : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param value retain_available
     */
    explicit retain_available(bool value);

    /**
     * @brief Get value
     * @return value
     */
    bool val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit retain_available(It b, End e);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, retain_available const& v);

/**
 * @ingroup property
 * @brief user property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class user_property : private boost::totally_ordered<user_property> {
public:
    /**
     * @brief constructor
     * @param key key string
     * @param val value string
     */
    explicit user_property(std::string key, std::string val);

    /**
     * @brief Add const buffer sequence into the given buffer.
     * @return A vector of const_buffer
     */
    std::vector<as::const_buffer> const_buffer_sequence() const;

    /**
     * @brief Get property::id
     * @return id
     */
    property::id id() const;

    /**
     * @brief Get property size
     * @return property size
     */
    std::size_t size() const;

    /**
     * @brief Get number of element of const_buffer_sequence
     * @return number of element of const_buffer_sequence
     */
    static constexpr std::size_t num_of_const_buffer_sequence();

    /**
     * @brief Get key
     * @return key
     */
    std::string key() const;

    /**
     * @brief Get value
     * @return value
     */
    std::string val() const;

    /**
     * @brief Get key as buffer
     * @return key
     */
    constexpr buffer const& key_as_buffer() const;

    /**
     * @brief Get value as buffer
     * @return value
     */
    constexpr buffer const& val_as_buffer() const;

    friend bool operator<(user_property const& lhs, user_property const& rhs) {
        return std::tie(lhs.id_, lhs.key_.buf, lhs.val_.buf) < std::tie(rhs.id_, rhs.key_.buf, rhs.val_.buf);
    }

    friend bool operator==(user_property const& lhs, user_property const& rhs) {
        return std::tie(lhs.id_, lhs.key_.buf, lhs.val_.buf) == std::tie(rhs.id_, rhs.key_.buf, rhs.val_.buf);
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit user_property(Buffer&& key, Buffer&& val, error_code& ec);

private:
    property::id id_ = id::user_property;
    detail::len_str key_;
    detail::len_str val_;
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, user_property const& v);

/**
 * @ingroup property
 * @brief maximum_packet_size property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class maximum_packet_size : public detail::n_bytes_property<4> {
public:
    /**
     * @brief constructor
     * @param val maximum_packet_size
     */
    explicit maximum_packet_size(std::uint32_t val);

    /**
     * @brief Get value
     * @return value
     */
    std::uint32_t val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit maximum_packet_size(It b, End e, error_code& ec);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, maximum_packet_size const& v);

/**
 * @ingroup property
 * @brief wildcard_subscription_available property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class wildcard_subscription_available : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param value shared_subscription_available
     */
    explicit wildcard_subscription_available(bool value);

    /**
     * @brief Get value
     * @return value
     */
    bool val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit wildcard_subscription_available(It b, End e);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, wildcard_subscription_available const& v);

/**
 * @ingroup property
 * @brief subscription_identifier_available property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class subscription_identifier_available : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param value subscription_identifier_available
     */
    explicit subscription_identifier_available(bool value);

    /**
     * @brief Get value
     * @return value
     */
    bool val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit subscription_identifier_available(It b, End e);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, subscription_identifier_available const& v);

/**
 * @ingroup property
 * @brief shared_subscription_available property
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class shared_subscription_available : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param value shared_subscription_available
     */
    explicit shared_subscription_available(bool value);

    /**
     * @brief Get value
     * @return value
     */
    bool val() const;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc, error_code& ec);

    // private constructor for internal use
    template <typename It, typename End>
    explicit shared_subscription_available(It b, End e);
};

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/property.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, shared_subscription_available const& v);

} // namespace property

} // namespace async_mqtt

#include <async_mqtt/packet/impl/property.hpp>

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/packet/impl/property.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PACKET_PROPERTY_HPP
