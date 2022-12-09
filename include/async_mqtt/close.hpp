// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_CLOSE_HPP)
#define ASYNC_MQTT_CLOSE_HPP

#include <async_mqtt/teardown.hpp>

namespace async_mqtt {

template <
    typename NextLayer,
    typename CompletionToken
>
typename as::async_result<std::decay_t<CompletionToken>, void(error_code)>::return_type
async_close(
    role_type,
    bs::websocket::stream<NextLayer>& stream,
    CompletionToken&& token
) {
    return stream.async_close(
        bs::websocket::close_code::normal,
        std::forward<CompletionToken>(token)
    );
}

template <
    typename Stream,
    typename CompletionToken
>
typename as::async_result<std::decay_t<CompletionToken>, void(error_code)>::return_type
async_close(
    role_type role,
    Stream& stream,
    CompletionToken&& token
) {
    return async_teardown(
        role,
        stream,
        std::forward<CompletionToken>(token)
    );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_CLOSE_HPP
