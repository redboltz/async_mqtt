// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_STREAM_TRAITS_HPP)
#define ASYNC_MQTT_STREAM_TRAITS_HPP

#include <type_traits>

namespace async_mqtt {

namespace detail {

template <typename T>
std::false_type has_next_layer_impl(void*);

template <typename T>
auto has_next_layer_impl(decltype(nullptr)) ->
    decltype(std::declval<T&>().next_layer(), std::true_type{});

template <typename T>
using has_next_layer = decltype(has_next_layer_impl<T>(nullptr));

template<typename T, bool = has_next_layer<T>::value>
struct lowest_layer_type_impl {
    using type = typename std::remove_reference<T>::type;
};

template<typename T>
struct lowest_layer_type_impl<T, true> {
    using type = typename lowest_layer_type_impl<
        decltype(std::declval<T&>().next_layer())>::type;
};

template<typename T>
using lowest_layer_type = typename lowest_layer_type_impl<T>::type;

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
using lowest_layer_type = detail::lowest_layer_type<T>;

template<typename T>
lowest_layer_type<T>& get_lowest_layer(T& t) noexcept {
    return
        detail::get_lowest_layer_impl(
            t,
            detail::has_next_layer<T>{}
        );
}


} // namespace async_mqtt

#endif // ASYNC_MQTT_STREAM_TRAITS_HPP
