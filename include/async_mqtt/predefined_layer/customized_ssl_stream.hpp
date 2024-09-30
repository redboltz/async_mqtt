// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_SSL_STREAM_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_SSL_STREAM_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <async_mqtt/util/stream_traits.hpp>
#include <async_mqtt/util/log.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;
namespace tls = as::ssl; // for backword compatilibity

static constexpr auto shutdown_timeout = std::chrono::seconds(3);

/**
 * @ingroup predefined_customize
 * @brief customization class template specialization for boost::asio::ssl::stream
 *
 * #### Requirements
 * - Header: async_mqtt/predefined_layer/customized_ssl_stream.hpp
 *
 */
template <typename NextLayer>
struct layer_customize<as::ssl::stream<NextLayer>> {
    template <
        typename CompletionToken
    >
    static auto
    async_close(
        as::ssl::stream<NextLayer>& stream,
        CompletionToken&& token
    ) {
        return as::async_compose<
            CompletionToken,
            void(error_code const& ec)
        > (
            async_close_impl{
                stream
            },
            token,
            stream
        );
    }

    struct async_close_impl {
        as::ssl::stream<NextLayer>& stream;
        enum {
            shutdown,
            complete
        } state = shutdown;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            BOOST_ASSERT(state == shutdown);
            auto tim = std::make_shared<as::steady_timer>(
                stream.get_executor(),
                shutdown_timeout
            );
            auto self_sp = std::make_shared<Self>(force_move(self));
            tim->async_wait(
                as::consign(
                    as::append(
                        std::ref(*self_sp),
                        std::weak_ptr<as::steady_timer>(tim)
                    ),
                    self_sp
                )
            );
            stream.async_shutdown(
                as::consign(
                    std::ref(*self_sp),
                    self_sp,
                    tim
                )
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec,
            std::weak_ptr<as::steady_timer> wp
        ) {
            if (!ec) {
                if (auto sp = wp.lock()) {
                    ASYNC_MQTT_LOG("mqtt_impl", info)
                        << "TLS async_shutdown timeout";
                    BOOST_ASSERT(state == shutdown);
                    state = complete;
                    self.complete(ec);
                    return;
                }
            }
            ASYNC_MQTT_LOG("mqtt_impl", info)
                << "TLS async_shutdown timeout doesn't processed. ec:" << ec.message();
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec
        ) {
            if (state == complete) {
                ASYNC_MQTT_LOG("mqtt_impl", info)
                    << "TLS async_shutdown already timeout";
            }
            else {
                ASYNC_MQTT_LOG("mqtt_impl", info)
                    << "TLS async_shutdown ec:" << ec.message();
                state = complete;
                self.complete(ec);
            }
        }
    };
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_SSL_STREAM_HPP
