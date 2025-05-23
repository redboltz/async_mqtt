// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASIO_BIND_IMPL_CLIENT_SUBSCRIBE_HPP)
#define ASYNC_MQTT_ASIO_BIND_IMPL_CLIENT_SUBSCRIBE_HPP

#include <boost/hana/tuple.hpp>
#include <boost/hana/back.hpp>
#include <boost/hana/drop_back.hpp>
#include <boost/hana/unpack.hpp>

#include <async_mqtt/asio_bind/client.hpp>
#include <async_mqtt/asio_bind/impl/client_impl.hpp>
#include <async_mqtt/util/log.hpp>

namespace async_mqtt {

namespace hana = boost::hana;

namespace detail {

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
auto
client_impl<Version, NextLayer>::async_subscribe(
    this_type_sp impl,
    error_code ec,
    std::optional<typename client_type::subscribe_packet> packet,
    CompletionToken&& token
) {
    if (packet) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, impl.get())
            << *packet;
    }
    BOOST_ASSERT(impl);
    auto exe = impl->get_executor();
    return
        as::async_initiate<
            CompletionToken,
            void(error_code, std::optional<typename client_type::suback_packet>)
        >(
            [](
                auto handler,
                this_type_sp impl,
                error_code ec,
                std::optional<typename client_type::subscribe_packet> packet
            ) {
                async_subscribe_impl(
                    force_move(impl),
                    ec,
                    force_move(packet),
                    force_move(handler)
                );
            },
            token,
            force_move(impl),
            ec,
            force_move(packet)
        );
}

} // namespace detail

template <protocol_version Version, typename NextLayer>
template <typename... Args>
auto
client<Version, NextLayer>::async_subscribe(Args&&... args) {
    BOOST_ASSERT(impl_);
    if constexpr (std::is_constructible_v<subscribe_packet, decltype(std::forward<Args>(args))...>) {
        try {
            return impl_type::async_subscribe(
                impl_,
                error_code{},
                subscribe_packet{std::forward<Args>(args)...}
            );
        }
        catch (system_error const& se) {
            return impl_type::async_subscribe(
                impl_,
                se.code(),
                std::nullopt
            );
        }
    }
    else {
        auto all = hana::tuple<Args...>(std::forward<Args>(args)...);
        auto back = hana::back(all);
        auto rest = hana::drop_back(all, hana::size_c<1>);
        return hana::unpack(
            std::move(rest),
            [&](auto&&... rest_args) {
                static_assert(
                    std::is_constructible_v<
                        subscribe_packet,
                        decltype(rest_args)...
                    >,
                    "subscribe_packet is not constructible"
                );
                try {
                    return impl_type::async_subscribe(
                        impl_,
                        error_code{},
                        subscribe_packet{std::forward<decltype(rest_args)>(rest_args)...},
                        force_move(back)
                    );
                }
                catch (system_error const& se) {
                    return impl_type::async_subscribe(
                        impl_,
                        se.code(),
                        std::nullopt,
                        force_move(back)
                    );
                }
            }
        );
    }
}

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/asio_bind/impl/client_subscribe.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_ASIO_BIND_IMPL_CLIENT_SUBSCRIBE_HPP
