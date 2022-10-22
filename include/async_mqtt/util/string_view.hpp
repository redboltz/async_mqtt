// Copyright Takatoshi Kondo 2016
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_STRING_VIEW_HPP)
#define ASYNC_MQTT_UTIL_STRING_VIEW_HPP

#include <iterator>
#include <string_view>
#include <type_traits>

namespace async_mqtt {

using std::string_view;
using std::basic_string_view;

namespace detail {

template<class T>
T* to_address(T* p) noexcept
{
    return p;
}

template<class T>
auto to_address(const T& p) noexcept
{
    return detail::to_address(p.operator->());
}

} // namespace detail

// Make a string_view from a pair of iterators.
template<typename Begin, typename End>
string_view make_string_view(Begin begin, End end) {
    static_assert(
        std::is_same_v<
            typename std::iterator_traits<Begin>::iterator_category,
            std::random_access_iterator_tag
        >
    );
    return string_view(detail::to_address(begin), static_cast<string_view::size_type>(std::distance(begin, end)));
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_STRING_VIEW_HPP
