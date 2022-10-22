// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_VARIANT_HPP)
#define ASYNC_MQTT_UTIL_VARIANT_HPP

#include <variant>

namespace async_mqtt {

using std::variant;
using std::monostate;
using std::visit;

// overload for std::visit lambda expressions
template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_VARIANT_HPP
