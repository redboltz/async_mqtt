// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_GET_INNER_EXECUTOR_HPP)
#define ASYNC_MQTT_GET_INNER_EXECUTOR_HPP

#include <type_traits>

#include <boost/asio/any_io_executor.hpp>

#include <async_mqtt/is_strand.hpp>

namespace async_mqtt {

namespace as = boost::asio;

template <typename Strand>
as::any_io_executor get_inner_executor(Strand& str) {
    if constexpr (
        is_strand_template<Strand>::value
#if 0
        ||
        is_null_strand_template<Strand>::value
#endif
    ) {
        return str.get_inner_executor();
    }
    else {
        static_assert(std::is_same_v<Strand, as::io_context::strand>);
        return str.context().get_executor();
    }
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_GET_INNER_EXECUTOR_HPP
