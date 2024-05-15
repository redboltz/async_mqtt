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

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>
#include <async_mqtt/util/json_like_out.hpp>
#include <async_mqtt/util/utf8validate.hpp>

#include <async_mqtt/exception.hpp>
#include <async_mqtt/packet/qos.hpp>
#include <async_mqtt/packet/property_id.hpp>
#include <async_mqtt/variable_bytes.hpp>
#include <async_mqtt/util/buffer.hpp>

/**
 * @defgroup property
 * @ingroup packet_v5
 */

/**
 * @defgroup property_internal
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


namespace property {

namespace detail {

enum class ostream_format {
    direct,
    int_cast,
    key_val,
    binary_string,
    json_like
};

/**
 * @ingroup property_internal
 * @brief N bytes_property
 *
 * N is 1,2, or 4 in property usecases
 * But this class template can accept any N.
 */
template <std::size_t N>
struct n_bytes_property : private boost::totally_ordered<n_bytes_property<N>> {
    n_bytes_property(property::id id, static_vector<char, N> const& buf)
        : id_{id},
          buf_{buf} // very small size copy
    {
    }

    template <typename It, typename End>
    n_bytes_property(property::id id, It b, End e)
        :id_{id}, buf_(b, e) {}

    n_bytes_property(property::id id, buffer const& buf)
        :id_{id} {
        BOOST_ASSERT(buf.size() >= N);
        buf_.insert(buf_.end(), (buf.begin(), std::next(buf.begin(), N)));
    }

    /**
     * @brief Add const buffer sequence into the given buffer.
     * @return A vector of const_buffer
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
        std::vector<as::const_buffer> v;
        v.reserve(num_of_const_buffer_sequence());
        v.emplace_back(as::buffer(&id_, 1));
        v.emplace_back(as::buffer(buf_.data(), buf_.size()));
        return v;
    }

    /**
     * @brief Get property::id
     * @return id
     */
    property::id id() const {
        return id_;
    }

    /**
     * @brief Get property size
     * @return property size
     */
    std::size_t size() const {
        return 1 + buf_.size();
    }

    /**
     * @brief Get number of element of const_buffer_sequence
     * @return number of element of const_buffer_sequence
     */
    static constexpr std::size_t num_of_const_buffer_sequence() {
        return 2;
    }

    friend bool operator<(n_bytes_property<N> const& lhs, n_bytes_property<N> const& rhs) {
        return std::tie(lhs.id_, lhs.buf_) < std::tie(rhs.id_, rhs.buf_);
    }

    friend bool operator==(n_bytes_property<N> const& lhs, n_bytes_property<N> const& rhs) {
        return std::tie(lhs.id_, lhs.buf_) == std::tie(rhs.id_, rhs.buf_);
    }

    static constexpr ostream_format const of_ = ostream_format::direct;
    property::id id_;
    static_vector<char, N> buf_;
};

/**
 * @ingroup property_internal
 * @brief binary_property
 */
struct binary_property : private boost::totally_ordered<binary_property> {
    binary_property(property::id id, buffer buf)
        :id_{id},
         buf_{force_move(buf)},
         length_(2) // size 2
    {
        if (buf_.size() > 0xffff) {
            throw make_error(
                errc::bad_message,
                "property::binary_property length is invalid"
            );
        }
        endian_store(boost::numeric_cast<std::uint16_t>(buf_.size()), length_.data());
    }

    /**
     * @brief Add const buffer sequence into the given buffer.
     * @return A vector of const_buffer
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
        std::vector<as::const_buffer> v;
        v.reserve(num_of_const_buffer_sequence());
        v.emplace_back(as::buffer(&id_, 1));
        v.emplace_back(as::buffer(length_.data(), length_.size()));
        v.emplace_back(as::buffer(buf_.data(), buf_.size()));
        return v;
    }

    /**
     * @brief Get property::id
     * @return id
     */
    property::id id() const {
        return id_;
    }

    /**
     * @brief Get property size
     * @return property size
     */
    std::size_t size() const {
        return 1 + length_.size() + buf_.size();
    }

    /**
     * @brief Get number of element of const_buffer_sequence
     * @return number of element of const_buffer_sequence
     */
    static constexpr std::size_t num_of_const_buffer_sequence() {
        return 3;
    }

    /**
     * @brief Get value
     * @return value
     */
    constexpr buffer const& val() const {
        return buf_;
    }

    friend bool operator<(binary_property const& lhs, binary_property const& rhs) {
        return std::tie(lhs.id_, lhs.buf_) < std::tie(rhs.id_, rhs.buf_);
    }

    friend bool operator==(binary_property const& lhs, binary_property const& rhs) {
        return std::tie(lhs.id_, lhs.buf_) == std::tie(rhs.id_, rhs.buf_);
    }

    static constexpr ostream_format const of_ = ostream_format::json_like;
    property::id id_;
    buffer buf_;
    static_vector<char, 2> length_;
};

/**
 * @ingroup property_internal
 * @brief string_property
 */
struct string_property : binary_property {
    string_property(property::id id, buffer buf)
        :binary_property{id, force_move(buf)} {
        if (!utf8string_check(this->val())) {
            throw make_error(
                errc::bad_message,
                "string property invalid utf8"
            );
        }
    }
};

/**
 * @ingroup property_internal
 * @brief variable property
 *
 * The length is 1 to 4.
 */
struct variable_property : private boost::totally_ordered<variable_property> {
    variable_property(property::id id, std::uint32_t value)
        :id_{id}  {
        value_ = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(value));
    }

    /**
     * @brief Add const buffer sequence into the given buffer.
     * @return A vector of const_buffer
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
        std::vector<as::const_buffer> v;
        v.reserve(num_of_const_buffer_sequence());
        v.emplace_back(as::buffer(&id_, 1));
        v.emplace_back(as::buffer(value_.data(), value_.size()));
        return v;
    }

    /**
     * @brief Get property::id
     * @return id
     */
    property::id id() const {
        return id_;
    }

    /**
     * @brief Get property size
     * @return property size
     */
    std::size_t size() const {
        return 1 + value_.size();
    }

    /**
     * @brief Get number of element of const_buffer_sequence
     * @return number of element of const_buffer_sequence
     */
    static constexpr std::size_t num_of_const_buffer_sequence() {
        return 2;
    }

    /**
     * @brief Get value
     * @return value
     */
    std::size_t val() const {
        auto it{value_.begin()};
        auto val_opt{variable_bytes_to_val(it, value_.end())};
        BOOST_ASSERT(val_opt);
        return *val_opt;
    }

    friend bool operator<(variable_property const& lhs, variable_property const& rhs) {
        auto const& lval = lhs.val();
        auto const& rval = rhs.val();
        return std::tie(lhs.id_, lval) < std::tie(rhs.id_, rval);
    }

    friend bool operator==(variable_property const& lhs, variable_property const& rhs) {
        auto const& lval = lhs.val();
        auto const& rval = rhs.val();
        return std::tie(lhs.id_, lval) == std::tie(rhs.id_, rval);
    }

    static constexpr ostream_format const of_ = ostream_format::direct;
    property::id id_;
    static_vector<char, 4> value_;
};

} // namespace detail


/**
 * @ingroup property
 * @brief payload_format_indicator property
 */
class payload_format_indicator : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param fmt payload_format
     */
    payload_format_indicator(payload_format fmt = payload_format::binary)
        : detail::n_bytes_property<1>{
              id::payload_format_indicator,
              {
                  [&] {
                      if (fmt == payload_format::binary) return  char(0);
                      return char(1);
                  }()
              }
          }
    {}

    /**
     * @brief Get value
     * @return payload_format
     */
    payload_format val() const {
        return
            [&] {
                if (buf_.front() == 0) return payload_format::binary;
                return  payload_format::string;
            } ();
    }

    static constexpr detail::ostream_format const of_ = detail::ostream_format::binary_string;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    payload_format_indicator(It b, End e)
        : detail::n_bytes_property<1>{id::payload_format_indicator, b, e} {}


};


/**
 * @ingroup property
 * @brief message_expiry_interval property
 */
class message_expiry_interval : public detail::n_bytes_property<4> {
public:
    /**
     * @brief constructor
     * @param val message_expiry_interval seconds
     */
    message_expiry_interval(std::uint32_t val)
        : detail::n_bytes_property<4>{id::message_expiry_interval, endian_static_vector(val)} {}

    /**
     * @brief Get value
     * @return message_expiry_interval seconds
     */
    std::uint32_t val() const {
        return endian_load<std::uint32_t>(buf_.data());
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    message_expiry_interval(It b, End e)
        : detail::n_bytes_property<4>{id::message_expiry_interval, b, e} {}
};

/**
 * @ingroup property
 * @brief content_type property
 */
class content_type : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val content type string
     */
    explicit content_type(std::string val)
        : detail::string_property{id::content_type, buffer{force_move(val)}} {}

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit content_type(Buffer&& val)
        : detail::string_property{id::content_type, std::forward<Buffer>(val)} {}
};

/**
 * @ingroup property
 * @brief response_topic property
 */
class response_topic : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val response_topic string
     */
    explicit response_topic(std::string val)
        : detail::string_property{id::response_topic, buffer{force_move(val)}} {}
private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit response_topic(Buffer&& val)
        : detail::string_property{id::response_topic, std::forward<Buffer>(val)} {}
};

/**
 * @ingroup property
 * @brief correlation_data property
 */
class correlation_data : public detail::binary_property {
public:
    /**
     * @brief constructor
     * @param val correlation_data string
     */
    explicit correlation_data(std::string val)
        : detail::binary_property{id::correlation_data, buffer{force_move(val)}} {}
private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit correlation_data(Buffer&& val)
        : detail::binary_property{id::correlation_data, std::forward<Buffer>(val)} {}
};

/**
 * @ingroup property
 * @brief subscription_identifier property
 */
class subscription_identifier : public detail::variable_property {
public:
    /**
     * @brief constructor
     * @param val subscription_identifier
     */
    subscription_identifier(std::uint32_t subscription_id)
        : detail::variable_property{id::subscription_identifier, subscription_id} {}
};

/**
 * @ingroup property
 * @brief session_expiry_interval property
 */
class session_expiry_interval : public detail::n_bytes_property<4> {
public:
    /**
     * @brief constructor
     * @param val session_expiry_interval seconds
     */
    session_expiry_interval(std::uint32_t val)
        : detail::n_bytes_property<4>{id::session_expiry_interval, endian_static_vector(val)} {
    }

    /**
     * @brief Get value
     * @return session_expiry_interval seconds
     */
    std::uint32_t val() const {
        return endian_load<std::uint32_t>(buf_.data());
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It>
    session_expiry_interval(It b, It e)
        : detail::n_bytes_property<4>{id::session_expiry_interval, b, e} {}
};

/**
 * @ingroup property
 * @brief assigned_client_identifier property
 */
class assigned_client_identifier : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val assigned_client_identifier string
     */
    explicit assigned_client_identifier(std::string val)
        : detail::string_property{id::assigned_client_identifier, buffer{force_move(val)}} {}

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit assigned_client_identifier(Buffer&& val)
        : detail::string_property{id::assigned_client_identifier, std::forward<Buffer>(val)} {}
};

/**
 * @ingroup property
 * @brief server_keep_alive property
 */
class server_keep_alive : public detail::n_bytes_property<2> {
public:
    /**
     * @brief constructor
     * @param val server_keep_alive seconds
     */
    server_keep_alive(std::uint16_t val)
        : detail::n_bytes_property<2>{id::server_keep_alive, endian_static_vector(val)} {}

    /**
     * @brief Get value
     * @return server_keep_alive seconds
     */
    std::uint16_t val() const {
        return endian_load<uint16_t>(buf_.data());
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    server_keep_alive(It b, End e)
        : detail::n_bytes_property<2>{id::server_keep_alive, b, e} {}
};

/**
 * @ingroup property
 * @brief authentication_method property
 */
class authentication_method : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val authentication_method string
     */
    explicit authentication_method(std::string val)
        : detail::string_property{id::authentication_method, buffer{force_move(val)}} {}

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit authentication_method(Buffer&& val)
        : detail::string_property{id::authentication_method, std::forward<Buffer>(val)} {}
};

/**
 * @ingroup property
 * @brief authentication_data property
 */
class authentication_data : public detail::binary_property {
public:
    /**
     * @brief constructor
     * @param val authentication_data string
     */
    explicit authentication_data(std::string val)
        : detail::binary_property{id::authentication_data, buffer{force_move(val)}} {}
private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit authentication_data(Buffer&& val)
        : detail::binary_property{id::authentication_data, std::forward<Buffer>(val)} {}
};

/**
 * @ingroup property
 * @brief request_problem_information property
 */
class request_problem_information : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param val request_problem_information
     */
    request_problem_information(bool value)
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

    /**
     * @brief Get value
     * @return value
     */
    bool val() const {
        return buf_.front() == 1;
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    request_problem_information(It b, End e)
        : detail::n_bytes_property<1>{id::request_problem_information, b, e} {}
};

/**
 * @ingroup property
 * @brief will_delay_interval property
 */
class will_delay_interval : public detail::n_bytes_property<4> {
public:
    /**
     * @brief constructor
     * @param val will_delay_interval seconds
     */
    will_delay_interval(std::uint32_t val)
        : detail::n_bytes_property<4>{id::will_delay_interval, endian_static_vector(val)} {}

    /**
     * @brief Get value
     * @return will_delay_interval seconds
     */
    std::uint32_t val() const {
        return endian_load<uint32_t>(buf_.data());
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    will_delay_interval(It b, End e)
        : detail::n_bytes_property<4>{id::will_delay_interval, b, e} {}
};

/**
 * @ingroup property
 * @brief request_response_information property
 */
class request_response_information : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param val request_response_information
     */
    request_response_information(bool value)
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

    /**
     * @brief Get value
     * @return value
     */
    bool val() const {
        return buf_.front() == 1;
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    request_response_information(It b, End e)
        : detail::n_bytes_property<1>(id::request_response_information, b, e) {}
};

/**
 * @ingroup property
 * @brief response_information property
 */
class response_information : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val response_information string
     */
    explicit response_information(std::string val)
        : detail::string_property{id::response_information, buffer{force_move(val)}} {}

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit response_information(Buffer&& val)
        : detail::string_property{id::response_information, std::forward<Buffer>(val)} {}
};

/**
 * @ingroup property
 * @brief server_reference property
 */
class server_reference : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val server_reference string
     */
    explicit server_reference(std::string val)
        : detail::string_property{id::server_reference, buffer{force_move(val)}} {}

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit server_reference(Buffer&& val)
        : detail::string_property{id::server_reference, std::forward<Buffer>(val)} {}
};

/**
 * @ingroup property
 * @brief reason_string property
 */
class reason_string : public detail::string_property {
public:
    /**
     * @brief constructor
     * @param val reason_string
     */
    explicit reason_string(std::string val)
        : detail::string_property{id::reason_string, buffer{force_move(val)}} {}

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit reason_string(Buffer&& val)
        : detail::string_property{id::reason_string, std::forward<Buffer>(val)} {}
};

/**
 * @ingroup property
 * @brief receive_maximum property
 */
class receive_maximum : public detail::n_bytes_property<2> {
public:
    /**
     * @brief constructor
     * @param val receive_maximum
     */
    receive_maximum(std::uint16_t val)
        : detail::n_bytes_property<2>{id::receive_maximum, endian_static_vector(val)} {
        if (val == 0) {
            throw make_error(
                errc::bad_message,
                "property::receive_maximum value is invalid"
            );
        }
    }

    /**
     * @brief Get value
     * @return value
     */
    std::uint16_t val() const {
        return endian_load<std::uint16_t>(buf_.data());
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    receive_maximum(It b, End e)
        : detail::n_bytes_property<2>{id::receive_maximum, b, e} {
        if (val() == 0) {
            throw make_error(
                errc::bad_message,
                "property::receive_maximum value is invalid"
            );
        }
    }
};


/**
 * @ingroup property
 * @brief topic_alias_maximum property
 */
class topic_alias_maximum : public detail::n_bytes_property<2> {
public:
    /**
     * @brief constructor
     * @param val topic_alias_maximum
     */
    topic_alias_maximum(std::uint16_t val)
        : detail::n_bytes_property<2>{id::topic_alias_maximum, endian_static_vector(val)} {}

    /**
     * @brief Get value
     * @return value
     */
    std::uint16_t val() const {
        return endian_load<std::uint16_t>(buf_.data());
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    topic_alias_maximum(It b, End e)
        : detail::n_bytes_property<2>{id::topic_alias_maximum, b, e} {}
};


/**
 * @ingroup property
 * @brief topic_alias property
 */
class topic_alias : public detail::n_bytes_property<2> {
public:
    /**
     * @brief constructor
     * @param val topic_alias
     */
    topic_alias(std::uint16_t val)
        : detail::n_bytes_property<2>{id::topic_alias, endian_static_vector(val)} {}

    /**
     * @brief Get value
     * @return value
     */
    std::uint16_t val() const {
        return endian_load<std::uint16_t>(buf_.data());
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    topic_alias(It b, End e)
        : detail::n_bytes_property<2>(id::topic_alias, b, e) {}
};

/**
 * @ingroup property
 * @brief maximum_qos property
 */
class maximum_qos : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param val maximum_qos
     */
    maximum_qos(qos value)
        : detail::n_bytes_property<1>{id::maximum_qos, {static_cast<char>(value)}} {
        if (value != qos::at_most_once &&
            value != qos::at_least_once) {
            throw make_error(
                errc::bad_message,
                "property::maximum_qos value is invalid"
            );
        }
    }

    /**
     * @brief Get value
     * @return value
     */
    std::uint8_t val() const {
        return static_cast<std::uint8_t>(buf_.front());
    }

    static constexpr const detail::ostream_format of_ = detail::ostream_format::int_cast;

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    maximum_qos(It b, End e)
        : detail::n_bytes_property<1>{id::maximum_qos, b, e} {}
};

/**
 * @ingroup property
 * @brief retain_available property
 */
class retain_available : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param val retain_available
     */
    retain_available(bool value)
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

    /**
     * @brief Get value
     * @return value
     */
    bool val() const {
        return buf_.front() == 1;
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    retain_available(It b, End e)
        : detail::n_bytes_property<1>{id::retain_available, b, e} {}
};


/**
 * @ingroup property
 * @brief user property
 */
class user_property : private boost::totally_ordered<user_property> {
public:
    /**
     * @brief constructor
     * @param val response_information string
     */
    user_property(std::string key, std::string val)
        : user_property{buffer{force_move(key)}, buffer{force_move(val)}}
    {}

    /**
     * @brief Add const buffer sequence into the given buffer.
     * @return A vector of const_buffer
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
        std::vector<as::const_buffer> v;
        v.reserve(num_of_const_buffer_sequence());
        v.emplace_back(as::buffer(&id_, 1));
        v.emplace_back(as::buffer(key_.len.data(), key_.len.size()));
        v.emplace_back(as::buffer(key_.buf));
        v.emplace_back(as::buffer(val_.len.data(), val_.len.size()));
        v.emplace_back(as::buffer(val_.buf));
        return v;
    }

    /**
     * @brief Get property::id
     * @return id
     */
    property::id id() const {
        return id_;
    }

    /**
     * @brief Get property size
     * @return property size
     */
    std::size_t size() const {
        return
            1 + // id_
            key_.size() +
            val_.size();
    }

    /**
     * @brief Get number of element of const_buffer_sequence
     * @return number of element of const_buffer_sequence
     */
    static constexpr std::size_t num_of_const_buffer_sequence() {
        return
            1 + // header
            2 + // key (len, buf)
            2;  // val (len, buf)
    }

    /**
     * @brief Get key
     * @return key
     */
    constexpr buffer const& key() const {
        return key_.buf;
    }

    /**
     * @brief Get value
     * @return value
     */
    constexpr buffer const& val() const {
        return val_.buf;
    }

    friend bool operator<(user_property const& lhs, user_property const& rhs) {
        return std::tie(lhs.id_, lhs.key_.buf, lhs.val_.buf) < std::tie(rhs.id_, rhs.key_.buf, rhs.val_.buf);
    }

    friend bool operator==(user_property const& lhs, user_property const& rhs) {
        return std::tie(lhs.id_, lhs.key_.buf, lhs.val_.buf) == std::tie(rhs.id_, rhs.key_.buf, rhs.val_.buf);
    }

    static constexpr detail::ostream_format const of_ = detail::ostream_format::key_val;

private:
    struct len_str {
        explicit len_str(buffer b)
            : buf{force_move(b)},
              len{endian_static_vector(boost::numeric_cast<std::uint16_t>(buf.size()))}
        {
#if 0 // TBD
            auto r = utf8string::validate_contents(buf);
            if (r != utf8string::validation::well_formed) throw utf8string_contents_error(r);
#endif
        }

        std::size_t size() const {
            return len.size() + buf.size();
        }
        buffer buf;
        static_vector<char, 2> len;
    };

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <
        typename Buffer,
        std::enable_if_t<std::is_same_v<Buffer, buffer>, std::nullptr_t> = nullptr
    >
    explicit user_property(Buffer&& key, Buffer&& val)
        : key_{std::forward<Buffer>(key)}, val_{std::forward<Buffer>(val)} {
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

private:
    property::id id_ = id::user_property;
    len_str key_;
    len_str val_;
};

/**
 * @ingroup property
 * @brief maximum_packet_size property
 */
class maximum_packet_size : public detail::n_bytes_property<4> {
public:
    /**
     * @brief constructor
     * @param val maximum_packet_size
     */
    maximum_packet_size(std::uint32_t val)
        : detail::n_bytes_property<4>{id::maximum_packet_size, endian_static_vector(val)} {
        if (val == 0) {
            throw make_error(
                errc::bad_message,
                "property::maximum_packet_size value is invalid"
            );
        }
    }

    /**
     * @brief Get value
     * @return value
     */
    std::uint32_t val() const {
        return endian_load<std::uint32_t>(buf_.data());
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    maximum_packet_size(It b, End e)
        : detail::n_bytes_property<4>{id::maximum_packet_size, b, e} {
        if (val() == 0) {
            throw make_error(
                errc::bad_message,
                "property::maximum_packet_size value is invalid"
            );
        }
    }
};


/**
 * @ingroup property
 * @brief wildcard_subscription_available property
 */
class wildcard_subscription_available : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param val shared_subscription_available
     */
    wildcard_subscription_available(bool value)
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

    /**
     * @brief Get value
     * @return value
     */
    bool val() const {
        return buf_.front() == 1;
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    wildcard_subscription_available(It b, End e)
        : detail::n_bytes_property<1>{id::wildcard_subscription_available, b, e} {}
};


/**
 * @ingroup property
 * @brief subscription_identifier_available property
 */
class subscription_identifier_available : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param val subscription_identifier_available
     */
    subscription_identifier_available(bool value)
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

    /**
     * @brief Get value
     * @return value
     */
    bool val() const {
        return buf_.front() == 1;
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    subscription_identifier_available(It b, End e)
        : detail::n_bytes_property<1>{id::subscription_identifier_available, b, e} {}
};


/**
 * @ingroup property
 * @brief shared_subscription_available property
 */
class shared_subscription_available : public detail::n_bytes_property<1> {
public:
    /**
     * @brief constructor
     * @param val shared_subscription_available
     */
    shared_subscription_available(bool value)
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

    /**
     * @brief Get value
     * @return value
     */
    bool val() const {
        return buf_.front() == 1;
    }

private:
    friend property_variant async_mqtt::make_property_variant(buffer& buf, property_location loc);

    // private constructor for internal use
    template <typename It, typename End>
    shared_subscription_available(It b, End e)
        : detail::n_bytes_property<1>{id::shared_subscription_available, b, e} {}
};

template <typename Property>
std::enable_if_t< Property::of_ == detail::ostream_format::direct, std::ostream& >
operator<<(std::ostream& o, Property const& p) {
    o <<
        "{" <<
        "id:" << p.id() << "," <<
        "val:" << p.val() <<
        "}";
    return o;
}

template <typename Property>
std::enable_if_t< Property::of_ == detail::ostream_format::int_cast, std::ostream& >
operator<<(std::ostream& o, Property const& p) {
    o <<
        "{" <<
        "id:" << p.id() << "," <<
        "val:" << static_cast<int>(p.val()) <<
        "}";
    return o;
}

template <typename Property>
std::enable_if_t< Property::of_ == detail::ostream_format::key_val, std::ostream& >
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
std::enable_if_t< Property::of_ == detail::ostream_format::binary_string, std::ostream& >
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
std::enable_if_t< Property::of_ == detail::ostream_format::json_like, std::ostream& >
operator<<(std::ostream& o, Property const& p) {
    o <<
        "{" <<
        "id:" << p.id() << "," <<
        "val:" << json_like_out(p.val()) <<
        "}";
    return o;
}


} // namespace property

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PROPERTY_HPP
