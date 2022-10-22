// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_STREAM_HPP)
#define ASYNC_MQTT_STREAM_HPP

#include <utility>
#include <type_traits>

#include <async_mqtt/core/stream_traits.hpp>

namespace async_mqtt {

template <typename NextLayer>
class stream {
public:
    using next_layer_type = typename std::remove_reference<NextLayer>::type;
    using executor_type = async_mqtt::executor_type<next_layer_type>;

};

} // namespace async_mqtt

#endif // ASYNC_MQTT_STREAM_HPP
