// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_CLOSE_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_CLOSE_HPP

#include <async_mqtt/client.hpp>

namespace async_mqtt {

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void()
)
client<Version, NextLayer>::async_close(
    CompletionToken&& token
) {
    return ep_->async_close(std::forward<CompletionToken>(token));
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_CLOSE_HPP
