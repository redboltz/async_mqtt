// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_EXCEPTION_HPP)
#define ASYNC_MQTT_EXCEPTION_HPP

#include <exception>
#include <sstream>

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/assert.hpp>
#include <boost/operators.hpp>

#include <async_mqtt/util/string_view.hpp>

namespace async_mqtt {

namespace sys = boost::system;
using error_code = sys::error_code;
namespace errc = sys::errc;

//using system_error = sys::system_error;

struct system_error : sys::system_error, private boost::totally_ordered<system_error> {
    using base_type = sys::system_error;
    using base_type::base_type;
    system_error(error_code const& ec)
        : base_type{ec}
    {}
    operator bool() const {
        return code() != errc::success;
    }
};


template <typename WhatArg>
inline system_error make_error(errc::errc_t ec, WhatArg&& wa) {
    return system_error(make_error_code(ec), std::forward<WhatArg>(wa));
}

inline bool operator==(system_error const& lhs, system_error const& rhs) {
    return
        std::tuple<error_code, string_view>(lhs.code(), lhs.what()) ==
        std::tuple<error_code, string_view>(rhs.code(), rhs.what());
}

inline bool operator<(system_error const& lhs, system_error const& rhs) {
    return
        std::tuple<error_code, string_view>(lhs.code(), lhs.what()) <
        std::tuple<error_code, string_view>(rhs.code(), rhs.what());
}

inline std::ostream& operator<<(std::ostream& o, system_error const& v) {
    o << v.what();
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_EXCEPTION_HPP
