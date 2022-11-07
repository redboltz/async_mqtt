// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_LOG_HPP)
#define ASYNC_MQTT_LOG_HPP

#include <cstddef>
#include <ostream>
#include <string>

#if defined(ASYNC_MQTT_USE_LOG)

#include <boost/log/core.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/preprocessor/if.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comparison/greater_equal.hpp>

#endif // defined(ASYNC_MQTT_USE_LOG)

namespace async_mqtt {

struct channel : std::string {
    using std::string::string;
};

enum class severity_level {
    trace,
    debug,
    info,
    warning,
    error,
    fatal
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

namespace detail {

struct null_log {
    template <typename... Params>
    constexpr null_log(Params&&...) {}
};

template <typename T>
inline constexpr null_log const& operator<<(null_log const& o, T const&) { return o; }

} // namespace detail

#if defined(ASYNC_MQTT_USE_LOG)

// template arguments are defined in async_mqtt
// filter and formatter can distinguish mqtt_cpp's channel and severity by their types
using global_logger_t = boost::log::sources::severity_channel_logger<severity_level, channel>;
inline global_logger_t& logger() {
    thread_local global_logger_t l;
    return l;
}

// Normal attributes
BOOST_LOG_ATTRIBUTE_KEYWORD(file, "MqttFile", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(line, "MqttLine", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(function, "MqttFunction", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(address, "MqttAddress", void const*)


// Take any filterable parameters (FP)
#define ASYNC_MQTT_LOG_FP(chan, sev)                                          \
    BOOST_LOG_STREAM_CHANNEL_SEV(async_mqtt::logger(), async_mqtt::channel(chan), sev) \
    << boost::log::add_value(async_mqtt::file, __FILE__)                   \
    << boost::log::add_value(async_mqtt::line, __LINE__)                   \
    << boost::log::add_value(async_mqtt::function, BOOST_CURRENT_FUNCTION)

#define ASYNC_MQTT_GET_LOG_SEV_NUM(lv) BOOST_PP_CAT(ASYNC_MQTT_, lv)

// Use can set preprocessor macro ASYNC_MQTT_LOG_SEV.
// For example, -DASYNC_MQTT_LOG_SEV=info, greater or equal to info log is generated at
// compiling time.

#if !defined(ASYNC_MQTT_LOG_SEV)
#define ASYNC_MQTT_LOG_SEV trace
#endif // !defined(ASYNC_MQTT_LOG_SEV)

#define ASYNC_MQTT_trace   0
#define ASYNC_MQTT_debug   1
#define ASYNC_MQTT_info    2
#define ASYNC_MQTT_warning 3
#define ASYNC_MQTT_error   4
#define ASYNC_MQTT_fatal   5

// User can define custom ASYNC_MQTT_LOG implementation
// By default ASYNC_MQTT_LOG_FP is used


#if !defined(ASYNC_MQTT_LOG)

#define ASYNC_MQTT_LOG(chan, sev)                                             \
    BOOST_PP_IF(                                                        \
        BOOST_PP_GREATER_EQUAL(ASYNC_MQTT_GET_LOG_SEV_NUM(sev), ASYNC_MQTT_GET_LOG_SEV_NUM(ASYNC_MQTT_LOG_SEV)), \
        ASYNC_MQTT_LOG_FP(chan, async_mqtt::severity_level::sev),                \
        async_mqtt::detail::null_log(chan, async_mqtt::severity_level::sev)   \
    )

#endif // !defined(ASYNC_MQTT_LOG)

#if !defined(ASYNC_MQTT_ADD_VALUE)

#define ASYNC_MQTT_ADD_VALUE(name, val) boost::log::add_value((async_mqtt::name), (val))

#endif // !defined(ASYNC_MQTT_ADD_VALUE)

#else  // defined(ASYNC_MQTT_USE_LOG)

#if !defined(ASYNC_MQTT_LOG)

#define ASYNC_MQTT_LOG(chan, sev) async_mqtt::detail::null_log(chan, async_mqtt::severity_level::sev)

#endif // !defined(ASYNC_MQTT_LOG)

#if !defined(ASYNC_MQTT_ADD_VALUE)

#define ASYNC_MQTT_ADD_VALUE(name, val) val

#endif // !defined(ASYNC_MQTT_ADD_VALUE)

#endif // defined(ASYNC_MQTT_USE_LOG)

} // namespace async_mqtt

#endif // ASYNC_MQTT_LOG_HPP
