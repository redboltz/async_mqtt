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

#include <async_mqtt/util/optional.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>
#include <async_mqtt/util/json_like_out.hpp>

#include <async_mqtt/exception.hpp>
#include <async_mqtt/packet/qos.hpp>
#include <async_mqtt/packet/property_id.hpp>
#include <async_mqtt/variable_bytes.hpp>
#include <async_mqtt/buffer.hpp>

namespace async_mqtt {

namespace as = boost::asio;

namespace property {

namespace detail {

enum class ostream_format {
    direct,
    int_cast,
    key_val,
    binary_string,
    json_like
};

// N is 1,2, or 4 in property usecases
// But this class template can accept any N.
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
     * @param v buffer to add
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
     * @brief Get whole size of sequence
     * @return whole size
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
     * @param v buffer to add
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
     * @brief Get whole size of sequence
     * @return whole size
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

struct string_property : binary_property {
    string_property(property::id id, buffer buf)
        :binary_property{id, force_move(buf)} {
#if 0 // TBD
        auto r = utf8string::validate_contents(this->val());
        if (r != utf8string::validation::well_formed) throw utf8string_contents_error(r);
#endif
    }
};

struct variable_property : private boost::totally_ordered<variable_property> {
    variable_property(property::id id, std::uint32_t value)
        :id_{id}  {
        value_ = val_to_variable_bytes(value);
    }

    /**
     * @brief Add const buffer sequence into the given buffer.
     * @param v buffer to add
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
     * @brief Get whole size of sequence
     * @return whole size
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

class payload_format_indicator : public detail::n_bytes_property<1> {
public:
    using recv = payload_format_indicator;
    using store = payload_format_indicator;
    enum payload_format {
        binary,
        string
    };

    payload_format_indicator(payload_format fmt = binary)
        : detail::n_bytes_property<1>{
              id::payload_format_indicator,
              {
                  [&] {
                      if (fmt == binary) return  char(0);
                      return char(1);
                  }()
              }
          }
    {}

    template <typename It, typename End>
    payload_format_indicator(It b, End e)
        : detail::n_bytes_property<1>{id::payload_format_indicator, b, e} {}

    payload_format val() const {
        return (  (buf_.front() == 0)
                ? binary
                : string);
    }

    static constexpr detail::ostream_format const of_ = detail::ostream_format::binary_string;
};


class message_expiry_interval : public detail::n_bytes_property<4> {
public:
    using recv = message_expiry_interval;
    using store = message_expiry_interval;
    message_expiry_interval(std::uint32_t val)
        : detail::n_bytes_property<4>{id::message_expiry_interval, endian_static_vector(val)} {}

    template <typename It, typename End>
    message_expiry_interval(It b, End e)
        : detail::n_bytes_property<4>{id::message_expiry_interval, b, e} {}

    std::uint32_t val() const {
        return endian_load<std::uint32_t>(buf_.data());
    }
};

class content_type : public detail::string_property {
public:
    explicit content_type(buffer val)
        : detail::string_property{id::content_type, force_move(val)} {}
};

class response_topic : public detail::string_property {
public:
    explicit response_topic(buffer val)
        : detail::string_property{id::response_topic, force_move(val)} {}
};

class correlation_data : public detail::binary_property {
public:
    explicit correlation_data(buffer val)
        : detail::binary_property{id::correlation_data, force_move(val)} {}
};

class subscription_identifier : public detail::variable_property {
public:
    using recv = subscription_identifier;
    using store = subscription_identifier;
    subscription_identifier(std::uint32_t subscription_id)
        : detail::variable_property{id::subscription_identifier, subscription_id} {}
};

class session_expiry_interval : public detail::n_bytes_property<4> {
public:
    using recv = session_expiry_interval;
    using store = session_expiry_interval;
    session_expiry_interval(std::uint32_t val)
        : detail::n_bytes_property<4>{id::session_expiry_interval, endian_static_vector(val)} {
    }

    template <typename It>
    session_expiry_interval(It b, It e)
        : detail::n_bytes_property<4>{id::session_expiry_interval, b, e} {}

    std::uint32_t val() const {
        return endian_load<std::uint32_t>(buf_.data());
    }
};

class assigned_client_identifier : public detail::string_property {
public:
    explicit assigned_client_identifier(buffer val)
        : detail::string_property{id::assigned_client_identifier, force_move(val)} {}
};

class server_keep_alive : public detail::n_bytes_property<2> {
public:
    using recv = server_keep_alive;
    using store = server_keep_alive;
    server_keep_alive(std::uint16_t val)
        : detail::n_bytes_property<2>{id::server_keep_alive, endian_static_vector(val)} {}

    template <typename It, typename End>
    server_keep_alive(It b, End e)
        : detail::n_bytes_property<2>{id::server_keep_alive, b, e} {}

    std::uint16_t val() const {
        return endian_load<uint16_t>(buf_.data());
    }
};

class authentication_method : public detail::string_property {
public:
    explicit authentication_method(buffer val)
        : detail::string_property{id::authentication_method, force_move(val)} {}
};

class authentication_data : public detail::binary_property {
public:
    explicit authentication_data(buffer val)
        : detail::binary_property{id::authentication_data, force_move(val)} {}
};

class request_problem_information : public detail::n_bytes_property<1> {
public:
    using recv = request_problem_information;
    using store = request_problem_information;
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
    template <typename It, typename End>
    request_problem_information(It b, End e)
        : detail::n_bytes_property<1>{id::request_problem_information, b, e} {}

    bool val() const {
        return buf_.front() == 1;
    }
};

class will_delay_interval : public detail::n_bytes_property<4> {
public:
    using recv = will_delay_interval;
    using store = will_delay_interval;
    will_delay_interval(std::uint32_t val)
        : detail::n_bytes_property<4>{id::will_delay_interval, endian_static_vector(val)} {}

    template <typename It, typename End>
    will_delay_interval(It b, End e)
        : detail::n_bytes_property<4>{id::will_delay_interval, b, e} {}

    std::uint32_t val() const {
        return endian_load<uint32_t>(buf_.data());
    }
};

class request_response_information : public detail::n_bytes_property<1> {
public:
    using recv = request_response_information;
    using store = request_response_information;
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

    template <typename It, typename End>
    request_response_information(It b, End e)
        : detail::n_bytes_property<1>(id::request_response_information, b, e) {}

    bool val() const {
        return buf_.front() == 1;
    }
};

class response_information : public detail::string_property {
public:
    explicit response_information(buffer val)
        : detail::string_property{id::response_information, force_move(val)} {}
};

class server_reference : public detail::string_property {
public:
    explicit server_reference(buffer val)
        : detail::string_property{id::server_reference, force_move(val)} {}
};

class reason_string : public detail::string_property {
public:
    explicit reason_string(buffer val)
        : detail::string_property{id::reason_string, force_move(val)} {}
};

class receive_maximum : public detail::n_bytes_property<2> {
public:
    using recv = receive_maximum;
    using store = receive_maximum;
    receive_maximum(std::uint16_t val)
        : detail::n_bytes_property<2>{id::receive_maximum, endian_static_vector(val)} {
        if (val == 0) {
            throw make_error(
                errc::bad_message,
                "property::receive_maximum value is invalid"
            );
        }
    }

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

    std::uint16_t val() const {
        return endian_load<std::uint16_t>(buf_.data());
    }
};


class topic_alias_maximum : public detail::n_bytes_property<2> {
public:
    using recv = topic_alias_maximum;
    using store = topic_alias_maximum;
    topic_alias_maximum(std::uint16_t val)
        : detail::n_bytes_property<2>{id::topic_alias_maximum, endian_static_vector(val)} {}

    template <typename It, typename End>
    topic_alias_maximum(It b, End e)
        : detail::n_bytes_property<2>{id::topic_alias_maximum, b, e} {}

    std::uint16_t val() const {
        return endian_load<std::uint16_t>(buf_.data());
    }
};


class topic_alias : public detail::n_bytes_property<2> {
public:
    using recv = topic_alias;
    using store = topic_alias;
    topic_alias(std::uint16_t val)
        : detail::n_bytes_property<2>{id::topic_alias, endian_static_vector(val)} {}

    template <typename It, typename End>
    topic_alias(It b, End e)
        : detail::n_bytes_property<2>(id::topic_alias, b, e) {}

    std::uint16_t val() const {
        return endian_load<std::uint16_t>(buf_.data());
    }
};

class maximum_qos : public detail::n_bytes_property<1> {
public:
    using recv = maximum_qos;
    using store = maximum_qos;
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

    template <typename It, typename End>
    maximum_qos(It b, End e)
        : detail::n_bytes_property<1>{id::maximum_qos, b, e} {}

    std::uint8_t val() const {
        return static_cast<std::uint8_t>(buf_.front());
    }

    static constexpr const detail::ostream_format of_ = detail::ostream_format::int_cast;
};

class retain_available : public detail::n_bytes_property<1> {
public:
    using recv = retain_available;
    using store = retain_available;
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

    template <typename It, typename End>
    retain_available(It b, End e)
        : detail::n_bytes_property<1>{id::retain_available, b, e} {}

    bool val() const {
        return buf_.front() == 1;
    }
};


class user_property : private boost::totally_ordered<user_property> {
public:
    user_property(buffer key, buffer val)
        : key_{force_move(key)}, val_{force_move(val)} {
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

    /**
     * @brief Add const buffer sequence into the given buffer.
     * @param v buffer to add
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
     * @brief Get whole size of sequence
     * @return whole size
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

    constexpr buffer const& key() const {
        return key_.buf;
    }

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
    property::id id_ = id::user_property;
    len_str key_;
    len_str val_;
};

class maximum_packet_size : public detail::n_bytes_property<4> {
public:
    using recv = maximum_packet_size;
    using store = maximum_packet_size;
    maximum_packet_size(std::uint32_t val)
        : detail::n_bytes_property<4>{id::maximum_packet_size, endian_static_vector(val)} {
        if (val == 0) {
            throw make_error(
                errc::bad_message,
                "property::maximum_packet_size value is invalid"
            );
        }
    }

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

    std::uint32_t val() const {
        return endian_load<std::uint32_t>(buf_.data());
    }
};


class wildcard_subscription_available : public detail::n_bytes_property<1> {
public:
    using recv = wildcard_subscription_available;
    using store = wildcard_subscription_available;
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

    template <typename It, typename End>
    wildcard_subscription_available(It b, End e)
        : detail::n_bytes_property<1>{id::wildcard_subscription_available, b, e} {}

    bool val() const {
        return buf_.front() == 1;
    }
};


class subscription_identifier_available : public detail::n_bytes_property<1> {
public:
    using recv = subscription_identifier_available;
    using store = subscription_identifier_available;
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

    template <typename It, typename End>
    subscription_identifier_available(It b, End e)
        : detail::n_bytes_property<1>{id::subscription_identifier_available, b, e} {}

    bool val() const {
        return buf_.front() == 1;
    }
};


class shared_subscription_available : public detail::n_bytes_property<1> {
public:
    using recv = shared_subscription_available;
    using store = shared_subscription_available;
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

    template <typename It, typename End>
    shared_subscription_available(It b, End e)
        : detail::n_bytes_property<1>{id::shared_subscription_available, b, e} {}

    bool val() const {
        return buf_.front() == 1;
    }
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
             if (p.val() == payload_format_indicator::binary) return "binary";
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
