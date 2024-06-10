// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_STREAM_TRAITS_HPP)
#define ASYNC_MQTT_UTIL_STREAM_TRAITS_HPP

#include <type_traits>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/any_completion_handler.hpp>
#include <boost/asio/any_io_executor.hpp>

#include <async_mqtt/error.hpp>

namespace async_mqtt {

namespace as = boost::asio;

namespace detail {

template <typename T>
std::false_type has_next_layer_impl(void*);

template <typename T>
auto has_next_layer_impl(decltype(nullptr)) ->
    decltype(std::declval<T&>().next_layer(), std::true_type{});

} // namespace detail

template <typename T>
using has_next_layer = decltype(detail::has_next_layer_impl<T>(nullptr));


namespace detail {

template<typename T, bool = has_next_layer<T>::value>
struct lowest_layer_type_impl {
    using type = typename std::remove_reference<T>::type;
};

template<typename T>
struct lowest_layer_type_impl<T, true> {
    using type = typename lowest_layer_type_impl<
        decltype(std::declval<T&>().next_layer())>::type;
};

} // namespace detail

template<typename T>
using lowest_layer_type = typename detail::lowest_layer_type_impl<T>::type;

namespace detail {

template<typename T>
T&
get_lowest_layer_impl(T& t, std::false_type) noexcept {
    return t;
}

template<typename T>
lowest_layer_type<T>&
get_lowest_layer_impl(T& t, std::true_type) noexcept {
    return
        get_lowest_layer_impl(
            t.next_layer(),
            has_next_layer<typename std::decay<decltype(t.next_layer())>::type>{}
        );
}

} // namespace detail


template<typename T>
using executor_type = decltype(std::declval<T>().get_executor());

template<typename T>
lowest_layer_type<T>& get_lowest_layer(T& t) noexcept {
    return
        detail::get_lowest_layer_impl(
            t,
            has_next_layer<T>{}
        );
}

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

// async_read

template <typename Layer, typename = void>
struct has_async_read : std::false_type {};

template <typename Layer>
struct has_async_read<
    Layer,
    std::void_t<
        decltype(
            layer_customize<Layer>::async_read(
                std::declval<Layer&>(),
                std::declval<as::mutable_buffer const&>(),
                std::declval<as::any_completion_handler<void(error_code const&, std::size_t)>>()
            )
        )
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

#endif // ASYNC_MQTT_UTIL_STREAM_TRAITS_HPP
