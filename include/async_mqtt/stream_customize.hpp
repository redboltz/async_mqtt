// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_STREAM_CUSTOMIZE_HPP)
#define ASYNC_MQTT_IMPL_STREAM_CUSTOMIZE_HPP

#include <type_traits>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/any_completion_handler.hpp>

#include <async_mqtt/protocol/error.hpp>

namespace async_mqtt {

namespace as = boost::asio;

/**
 * @defgroup underlying_customize underlying layer customization
 * @ingroup underlying_layer
 */

/**
 * @ingroup underlying_customize
 * @brief customization class template for underlying layer
 * In order to adapt your layer to async_mqtt, specialize the class template.
 * @tparam Layer Specialized parameter for your own layer
 */
template <typename Layer>
struct layer_customize;

// initialze

template <typename Layer, typename = void>
struct has_initialize : std::false_type {};

template <typename Layer>
struct has_initialize<
    Layer,
    std::void_t<
        decltype(layer_customize<Layer>::initialize(std::declval<Layer&>()))
    >
> : std::true_type {};

// async_read_some

template <typename Layer, typename = void>
struct has_async_read_some : std::false_type {};

template <typename Layer>
struct has_async_read_some<
    Layer,
    std::void_t<
        decltype(
            layer_customize<Layer>::async_read_some(
                std::declval<Layer&>(),
                std::declval<as::mutable_buffer const&>(),
                std::declval<as::any_completion_handler<void(error_code const&, std::size_t)>>()
            )
        )
    >
> : std::true_type {};

// async_write

template <typename Layer, typename = void>
struct has_async_write : std::false_type {};

template <typename Layer>
struct has_async_write<
    Layer,
    std::void_t<
        decltype(
            layer_customize<Layer>::async_write(
                std::declval<Layer&>(),
                std::declval<std::vector<as::const_buffer> const&>(),
                std::declval<as::any_completion_handler<void(error_code const&, std::size_t)>>()
            )
        )
    >
> : std::true_type {};

// async_close

template <typename Layer, typename = void>
struct has_async_close : std::false_type {};

template <typename Layer>
struct has_async_close<
    Layer,
    std::void_t<
        decltype(
            layer_customize<Layer>::async_close(
                std::declval<Layer&>(),
                std::declval<as::any_completion_handler<void(error_code const&)>>()
            )
        )
    >
> : std::true_type {};

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_STREAM_CUSTOMIZE_HPP
