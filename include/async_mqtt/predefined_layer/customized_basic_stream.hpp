// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_BASIC_STREAM_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_BASIC_STREAM_HPP

#include <boost/asio.hpp>

#include <async_mqtt/util/stream_traits.hpp>
#include <async_mqtt/util/log.hpp>

namespace async_mqtt {

namespace as = boost::asio;

/**
 * @defgroup predefined_customize predefined underlying customize
 * @ingroup underlying_customize
 */

/**
 * @ingroup predefined_customize
 * @brief customization class template specialization for boost::asio::basic_stream_socket
 */
template <typename Protocol, typename Executor>
struct layer_customize<as::basic_stream_socket<Protocol, Executor>> {
    template <
        typename CompletionToken
    >
    static auto
    async_close(
        as::basic_stream_socket<Protocol, Executor>& stream,
        CompletionToken&& token
    ) {
        return as::async_compose<
            CompletionToken,
            void(error_code const& ec)
        > (
            [&stream](auto& self) {
                error_code ec;
                if (stream.is_open()) {
                    ASYNC_MQTT_LOG("mqtt_impl", info)
                        << "TCP close";
                    stream.close(ec);
                }
                else {
                    ASYNC_MQTT_LOG("mqtt_impl", info)
                        << "TCP already closed";
                }
                self.complete(ec);
            },
            token,
            stream
        );
    }
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_BASIC_STREAM_HPP
