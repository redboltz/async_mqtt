// Copyright Takatoshi Kondo 2018
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_DETAIL_BASE_PROPERTY_HPP)
#define ASYNC_MQTT_PACKET_DETAIL_BASE_PROPERTY_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/operators.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/packet/property_id.hpp>

#include <async_mqtt/exception.hpp>
#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/endian_convert.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/variable_bytes.hpp>
#include <async_mqtt/util/utf8validate.hpp>

namespace async_mqtt::property::detail {

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
    std::string val() const {
        return std::string{buf_};
    }

    /**
     * @brief Get value
     * @return value
     */
    constexpr buffer const& val_as_buffer() const {
        return buf_;
    }

    /**
     * @brief less than operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
     */
    friend bool operator<(binary_property const& lhs, binary_property const& rhs) {
        return std::tie(lhs.id_, lhs.buf_) < std::tie(rhs.id_, rhs.buf_);
    }

    /**
     * @brief equal operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
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

struct len_str {
    explicit len_str(buffer b)
        : buf{force_move(b)},
          len{endian_static_vector(boost::numeric_cast<std::uint16_t>(buf.size()))}
    {
        if (!utf8string_check(buf)) {
            throw make_error(
                errc::bad_message,
                "string property invalid utf8"
            );
        }
    }

    std::size_t size() const {
        return len.size() + buf.size();
    }

    buffer buf;
    static_vector<char, 2> len;
};

} // namespace async_mqtt::property::detail

#endif // ASYNC_MQTT_PACKET_DETAIL_BASE_PROPERTY_HPP
