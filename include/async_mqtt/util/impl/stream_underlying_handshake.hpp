// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_IMPL_STREAM_UNDERLYING_HANDSHAKE_HPP)
#define ASYNC_MQTT_UTIL_IMPL_STREAM_UNDERLYING_HANDSHAKE_HPP

#include <async_mqtt/error.hpp>
#include <async_mqtt/util/stream.hpp>
#include <async_mqtt/util/shared_ptr_array.hpp>

#include <boost/hana/tuple.hpp>
#include <boost/hana/unpack.hpp>

namespace async_mqtt {

namespace hana = boost::hana;

namespace detail {

template <typename NextLayer>
template <
    typename... Args
>
auto
stream_impl<NextLayer>::
async_underlying_handshake(
    this_type_sp impl,
    Args&&... args
) {
    return layer_customize<next_layer_type>::async_handshake(
        impl->next_layer(),
        std::forward<Args>(args)...
    );
}

} // namespace detail

template <typename NextLayer>
template <
    typename... Args
>
auto
stream<NextLayer>::async_underlying_handshake(
    Args&&... args
) {
    BOOST_ASSERT(impl_);

    return impl_type::async_underlying_handshake(
        impl_,
        std::forward<Args>(args)...
    );
}


} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_IMPL_STREAM_UNDERLYING_HANDSHAKE_HPP
