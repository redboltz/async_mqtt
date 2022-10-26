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

namespace async_mqtt {

namespace sys = boost::system;

using sys::system_error;
using sys::error_code;
namespace errc = sys::errc;

template <typename WhatArg>
inline sys::system_error make_error(errc::errc_t ec, WhatArg&& wa) {
    return system_error(make_error_code(ec), std::forward<WhatArg>(wa));
}


} // namespace async_mqtt

#endif // ASYNC_MQTT_EXCEPTION_HPP
