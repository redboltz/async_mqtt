// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_LOG_SEVERITY_HPP)
#define ASYNC_MQTT_UTIL_LOG_SEVERITY_HPP

#include <ostream>

namespace async_mqtt {

/**
 * @ingroup log
 * log severity level
 * warning is recommended for actual operation because there is no output except something important.
 *
 * #### Requirements
 * @li Header: async_mqtt/util/log.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
enum class severity_level {
    trace,   ///< trace level for detaied behavior and reporting issue
    debug,   ///< debug level not used in async_mqtt, so far
    info,    ///< info level api call is output
    warning, ///< warning level such as timeout
    error,   ///< error level error report such as connection is failed
    fatal    ///< fatal level it is logic error of async_mqtt
};

inline std::ostream& operator<<(std::ostream& o, severity_level sev) {
    constexpr char const* const str[] {
        "trace",
        "debug",
        "info",
        "warning",
        "error",
        "fatal"
    };
    o << str[static_cast<std::size_t>(sev)];
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_LOG_SEVERITY_HPP
