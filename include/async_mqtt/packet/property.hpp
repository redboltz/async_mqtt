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
property_variant make_property_variant(buffer& buf, property_location loc);

/**
 * @ingroup property
 * @brief payload_format
 */
enum class payload_format {
    binary, ///< binary
    string  ///< string
};

/**
 * @ingroup property
 * @brief type of the session expiry interval (seconds)
 */
using session_expiry_interval_type = std::uint32_t;

/**
 * @ingroup property
 * @brief type of the topic alias value
 */
using topic_alias_type = std::uint16_t;

/**
 * @ingroup property
 * @brief type of the receive maximum value
 */
using receive_maximum_type = std::uint16_t;

/**
 * @ingroup property
 * @brief the special session_expiry_interval value that session is never expire.
 */
static constexpr session_expiry_interval_type session_never_expire = 0xffffffffUL;

/**
 * @ingroup property
 * @brief the maximum topic_alias value
 */
static constexpr topic_alias_type topic_alias_max = 0xffff;

/**
 * @ingroup property
 * @brief the maximum receive_maximum value
 */
static constexpr receive_maximum_type receive_maximum_max = 0xffff;

/**
 * @ingroup property
 * @brief the maximum maximum_packet_size value
 */
static constexpr std::uint32_t packet_size_no_limit =
    1 + // fixed header
    4 + // remaining length
    128 * 128 * 128 * 128; // maximum value of remainin length

namespace property {

/**
 * @ingroup property
 * @brief payload_format_indicator property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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

    static constexpr detail::ostream_format const of_ = detail::ostream_format::binary_string;

private:

    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit payload_format_indicator(It b, End e);
};


/**
 * @ingroup property
 * @brief message_expiry_interval property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit message_expiry_interval(It b, End e);
};

/**
 * @ingroup property
 * @brief content_type property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 */
class content_type : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val content type string
     */
    explicit content_type(std::string val);

#if defined(GENERATING_DOCUMENTATION)

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
     * @brief Get value
     * @return value
     */
    constexpr std::string val() const;

    /**
     * @brief Get value
     * @return value
     */
    constexpr buffer const& val_as_buffer() const;

    /**
     * @brief less than operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
     */
    friend bool operator<(binary_property const& lhs, binary_property const& rhs);

    /**
     * @brief equal operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
    friend bool operator==(binary_property const& lhs, binary_property const& rhs);

#endif // defined(GENERATING_DOCUMENTATION)

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit content_type(Buffer&& val);
};

/**
 * @ingroup property
 * @brief response_topic property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 */
class response_topic : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val response_topic string
     */
    explicit response_topic(std::string val);

#if defined(GENERATING_DOCUMENTATION)

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
     * @brief Get value
     * @return value
     */
    constexpr std::string val() const;

    /**
     * @brief Get value
     * @return value
     */
    constexpr buffer const& val_as_buffer() const;

    /**
     * @brief less than operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
     */
    friend bool operator<(binary_property const& lhs, binary_property const& rhs);

    /**
     * @brief equal operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
    friend bool operator==(binary_property const& lhs, binary_property const& rhs);

#endif // defined(GENERATING_DOCUMENTATION)

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit response_topic(Buffer&& val);
};

/**
 * @ingroup property
 * @brief correlation_data property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 */
class correlation_data : public detail::binary_property {
public:
    /**
     * @brief constructor
     * @param val correlation_data string
     */
    explicit correlation_data(std::string val);

#if defined(GENERATING_DOCUMENTATION)

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
     * @brief Get value
     * @return value
     */
    constexpr std::string val() const;

    /**
     * @brief Get value
     * @return value
     */
    constexpr buffer const& val_as_buffer() const;

    /**
     * @brief less than operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
     */
    friend bool operator<(binary_property const& lhs, binary_property const& rhs);

    /**
     * @brief equal operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
    friend bool operator==(binary_property const& lhs, binary_property const& rhs);

#endif // defined(GENERATING_DOCUMENTATION)

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit correlation_data(Buffer&& val);
};

/**
 * @ingroup property
 * @brief subscription_identifier property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 */
class subscription_identifier : public detail::variable_property {
public:
    /**
     * @brief constructor
     * @param val subscription_identifier
     */
    explicit subscription_identifier(std::uint32_t val);
};

/**
 * @ingroup property
 * @brief session_expiry_interval property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It>
    explicit session_expiry_interval(It b, It e);
};

/**
 * @ingroup property
 * @brief assigned_client_identifier property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 */
class assigned_client_identifier : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val assigned_client_identifier string
     */
    explicit assigned_client_identifier(std::string val);

#if defined(GENERATING_DOCUMENTATION)

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
     * @brief Get value
     * @return value
     */
    constexpr std::string val() const;

    /**
     * @brief Get value
     * @return value
     */
    constexpr buffer const& val_as_buffer() const;

    /**
     * @brief less than operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
     */
    friend bool operator<(binary_property const& lhs, binary_property const& rhs);

    /**
     * @brief equal operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
    friend bool operator==(binary_property const& lhs, binary_property const& rhs);

#endif // defined(GENERATING_DOCUMENTATION)

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit assigned_client_identifier(Buffer&& val);
};

/**
 * @ingroup property
 * @brief server_keep_alive property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit server_keep_alive(It b, End e);
};

/**
 * @ingroup property
 * @brief authentication_method property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 */
class authentication_method : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val authentication_method string
     */
    explicit authentication_method(std::string val);

#if defined(GENERATING_DOCUMENTATION)

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
     * @brief Get value
     * @return value
     */
    constexpr std::string val() const;

    /**
     * @brief Get value
     * @return value
     */
    constexpr buffer const& val_as_buffer() const;

    /**
     * @brief less than operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
     */
    friend bool operator<(binary_property const& lhs, binary_property const& rhs);

    /**
     * @brief equal operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
    friend bool operator==(binary_property const& lhs, binary_property const& rhs);

#endif // defined(GENERATING_DOCUMENTATION)

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit authentication_method(Buffer&& val);
};

/**
 * @ingroup property
 * @brief authentication_data property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 */
class authentication_data : public detail::binary_property {
public:
    /**
     * @brief constructor
     * @param val authentication_data string
     */
    explicit authentication_data(std::string val);

#if defined(GENERATING_DOCUMENTATION)

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
     * @brief Get value
     * @return value
     */
    constexpr std::string val() const;

    /**
     * @brief Get value
     * @return value
     */
    constexpr buffer const& val_as_buffer() const;

    /**
     * @brief less than operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
     */
    friend bool operator<(binary_property const& lhs, binary_property const& rhs);

    /**
     * @brief equal operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
    friend bool operator==(binary_property const& lhs, binary_property const& rhs);

#endif // defined(GENERATING_DOCUMENTATION)

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit authentication_data(Buffer&& val);
};

/**
 * @ingroup property
 * @brief request_problem_information property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit request_problem_information(It b, End e);
};

/**
 * @ingroup property
 * @brief will_delay_interval property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit will_delay_interval(It b, End e);
};

/**
 * @ingroup property
 * @brief request_response_information property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit request_response_information(It b, End e);
};

/**
 * @ingroup property
 * @brief response_information property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 */
class response_information : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val response_information string
     */
    explicit response_information(std::string val);

#if defined(GENERATING_DOCUMENTATION)

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
     * @brief Get value
     * @return value
     */
    constexpr std::string val() const;

    /**
     * @brief Get value
     * @return value
     */
    constexpr buffer const& val_as_buffer() const;

    /**
     * @brief less than operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
     */
    friend bool operator<(binary_property const& lhs, binary_property const& rhs);

    /**
     * @brief equal operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
    friend bool operator==(binary_property const& lhs, binary_property const& rhs);

#endif // defined(GENERATING_DOCUMENTATION)

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit response_information(Buffer&& val);
};

/**
 * @ingroup property
 * @brief server_reference property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 */
class server_reference : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val server_reference string
     */
    explicit server_reference(std::string val);

#if defined(GENERATING_DOCUMENTATION)

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
     * @brief Get value
     * @return value
     */
    constexpr std::string val() const;

    /**
     * @brief Get value
     * @return value
     */
    constexpr buffer const& val_as_buffer() const;

    /**
     * @brief less than operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
     */
    friend bool operator<(binary_property const& lhs, binary_property const& rhs);

    /**
     * @brief equal operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
    friend bool operator==(binary_property const& lhs, binary_property const& rhs);

#endif // defined(GENERATING_DOCUMENTATION)

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit server_reference(Buffer&& val);
};

/**
 * @ingroup property
 * @brief reason_string property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 */
class reason_string : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val reason_string
     */
    explicit reason_string(std::string val);

#if defined(GENERATING_DOCUMENTATION)

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
     * @brief Get value
     * @return value
     */
    constexpr std::string val() const;

    /**
     * @brief Get value
     * @return value
     */
    constexpr buffer const& val_as_buffer() const;

    /**
     * @brief less than operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
     */
    friend bool operator<(binary_property const& lhs, binary_property const& rhs);

    /**
     * @brief equal operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
    friend bool operator==(binary_property const& lhs, binary_property const& rhs);

#endif // defined(GENERATING_DOCUMENTATION)

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit reason_string(Buffer&& val);
};

/**
 * @ingroup property
 * @brief receive_maximum property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit receive_maximum(It b, End e);
};


/**
 * @ingroup property
 * @brief topic_alias_maximum property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit topic_alias_maximum(It b, End e);
};


/**
 * @ingroup property
 * @brief topic_alias property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit topic_alias(It b, End e);
};

/**
 * @ingroup property
 * @brief maximum_qos property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    std::uint8_t val() const;

    static constexpr const detail::ostream_format of_ = detail::ostream_format::int_cast;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit maximum_qos(It b, End e);
};

/**
 * @ingroup property
 * @brief retain_available property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit retain_available(It b, End e);
};


/**
 * @ingroup property
 * @brief user property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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

    static constexpr detail::ostream_format const of_ = detail::ostream_format::key_val;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit user_property(Buffer&& key, Buffer&& val);

private:
    property::id id_ = id::user_property;
    detail::len_str key_;
    detail::len_str val_;
};

/**
 * @ingroup property
 * @brief maximum_packet_size property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit maximum_packet_size(It b, End e);
};


/**
 * @ingroup property
 * @brief wildcard_subscription_available property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit wildcard_subscription_available(It b, End e);
};


/**
 * @ingroup property
 * @brief subscription_identifier_available property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    explicit subscription_identifier_available(It b, End e);
};


/**
 * @ingroup property
 * @brief shared_subscription_available property
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
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
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

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
 */
template <typename Property>
std::enable_if_t<
    Property::of_ == detail::ostream_format::direct,
    std::ostream&
>
operator<<(std::ostream& o, Property const& p);

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 */
template <typename Property>
std::enable_if_t<
    Property::of_ == detail::ostream_format::int_cast,
    std::ostream&
>
operator<<(std::ostream& o, Property const& p);

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 */
template <typename Property>
std::enable_if_t<
    Property::of_ == detail::ostream_format::key_val,
    std::ostream&
>
operator<<(std::ostream& o, Property const& p);

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 */
template <typename Property>
std::enable_if_t<
    Property::of_ == detail::ostream_format::binary_string,
    std::ostream&
>
operator<<(std::ostream& o, Property const& p);

/**
 * @ingroup property
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 */
template <typename Property>
std::enable_if_t<
    Property::of_ == detail::ostream_format::json_like,
    std::ostream&
>
operator<<(std::ostream& o, Property const& p);

} // namespace property

} // namespace async_mqtt

#include <async_mqtt/packet/impl/property.hpp>

#endif // ASYNC_MQTT_PACKET_PROPERTY_HPP
