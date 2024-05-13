// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_EXCEPTION_HPP)
#define ASYNC_MQTT_EXCEPTION_HPP

#include <exception>
#include <sstream>
#include <string_view>

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/assert.hpp>
#include <boost/operators.hpp>

/**
 * @defgroup error
 * @brief Common error
 */

namespace async_mqtt {

/**
 * @ingroup error
 * @brief type alias of boost::sytem.
 */
namespace sys = boost::system;

/**
 * @ingroup error
 * @brief type alias of boost::sytem::error_code.
 */
using error_code = sys::error_code;

/**
 * @ingroup error
 * @brief type alias of boost::sytem::errc.
 */
namespace errc = sys::errc;

/**
 * @ingroup error
 * @brief async_mqtt error class. It is used as CompletionToken parameter and exception.
 */
struct system_error : sys::system_error, private boost::totally_ordered<system_error> {
    using base_type = sys::system_error;
    using base_type::base_type;

    /**
     * @brief constructor
     * @param ec error code. If omit, then no error (success).
     */
    system_error(error_code const& ec = error_code())
        : base_type{ec}
    {}

    // std::string what() const noexcept; return more detailed message
    // It is defined in base_type

    /**
     * @brief get error message
     * If you want to more detaied error message, call what() instead.
     *
     * @return error message string
     */
    std::string message() const {
        return code().message();
    }

    /**
     * @brief bool operator
     * @return if error then true, otherwise false.
     */
    operator bool() const {
        return code() != errc::success;
    }
};


/**
 * @related system_error
 * @brief system_error factory function
 *
 * @param ec error code.
 * @param wa detailed error message that is gotten by system_error::what().
 * @return system_error
 */
template <typename WhatArg>
inline system_error make_error(errc::errc_t ec, WhatArg&& wa) {
    return system_error(make_error_code(ec), std::forward<WhatArg>(wa));
}

/**
 * @related system_error
 * @brief system_error equality comparison operator
 * @param lhs left hand side operand
 * @param rhs right hand side operand
 * @return true if lhs code() and what() equal to rhs, otherwise false.
 */
inline bool operator==(system_error const& lhs, system_error const& rhs) {
    return
        std::tuple<error_code, std::string_view>(lhs.code(), lhs.what()) ==
        std::tuple<error_code, std::string_view>(rhs.code(), rhs.what());
}

/**
 * @related system_error
 * @brief system_error less than comparison operator
 * @param lhs left hand side operand
 * @param rhs right hand side operand
 * @return true if lhs code() and what() is less than rhs, otherwise false.
 */
inline bool operator<(system_error const& lhs, system_error const& rhs) {
    return
        std::tuple<error_code, std::string_view>(lhs.code(), lhs.what()) <
        std::tuple<error_code, std::string_view>(rhs.code(), rhs.what());
}

/**
 * @related system_error
 * @brief system_error stream output operator
 * @param o output stream
 * @param v output object
 * @return  output stream
 */
inline std::ostream& operator<<(std::ostream& o, system_error const& v) {
    o << v.what();
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_EXCEPTION_HPP
