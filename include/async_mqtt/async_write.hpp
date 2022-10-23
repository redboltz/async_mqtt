// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASYNC_WRITE_HPP)
#define ASYNC_MQTT_ASYNC_WRITE_HPP

#include <async_mqtt/move.hpp>

namespace async_mqtt {

namespace as = boost::asio;

template <
    typename Stream,
    typename Packet, // add concept later TBD
    typename CompletionToken,
    typename std::enable_if_t<
        std::is_invocable<CompletionToken, boost::system::error_code, std::size_t>::value
    >* = nullptr
>
auto async_write_packet(
    Stream& stream,
    Packet&& packet,
    CompletionToken&& token
) {
    auto cbs = packet.const_buffer_sequence();
    return
        async_write(
            stream,
            force_move(cbs),
            [token = std::forward<CompletionToken>(token), packet = std::forward<Packet>(packet)]
            (boost::system::error_code const& ec, std::size_t bytes_transferred) mutable {
                force_move(token)(ec, bytes_transferred);
            }
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_ASYNC_WRITE_HPP
