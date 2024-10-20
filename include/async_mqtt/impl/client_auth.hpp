// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_AUTH_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_AUTH_HPP

#include <boost/hana/tuple.hpp>
#include <boost/hana/back.hpp>
#include <boost/hana/drop_back.hpp>
#include <boost/hana/unpack.hpp>

#include <async_mqtt/client.hpp>
#include <async_mqtt/util/log.hpp>
#include <async_mqtt/packet/v5_auth.hpp>

namespace async_mqtt {

namespace hana = boost::hana;

namespace detail {

template <protocol_version Version, typename NextLayer>
struct client_impl<Version, NextLayer>::
auth_op {
    this_type_sp cl;
    error_code ec;
    std::optional<v5::auth_packet> packet;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_cl{*cl};
        if (ec) {
            self.complete(ec);
            return;
        }
        auto a_packet{force_move(*packet)};
        a_cl.ep_.async_send(
            force_move(a_packet),
            force_move(self)
        );
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code const& ec
    ) {
        self.complete(ec);
    }
};

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
auto
client_impl<Version, NextLayer>::async_auth_impl(
    this_type_sp impl,
    error_code ec,
    std::optional<v5::auth_packet> packet,
    CompletionToken&& token
) {
    BOOST_ASSERT(impl);
    if (packet) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, impl.get())
            << *packet;
    }
    auto exe = impl->get_executor();
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            auth_op{
                force_move(impl),
                ec,
                force_move(packet)
            },
            token,
            exe
        );
}

} // namespace detail

template <protocol_version Version, typename NextLayer>
template <typename... Args>
auto
client<Version, NextLayer>::async_auth(Args&&... args) {
    BOOST_ASSERT(impl_);
    if constexpr (std::is_constructible_v<v5::auth_packet, decltype(std::forward<Args>(args))...>) {
        try {
            return impl_type::async_auth_impl(
                impl_,
                error_code{},
                v5::auth_packet{std::forward<Args>(args)...}
            );
        }
        catch (system_error const& se) {
            return impl_type::async_auth_impl(
                impl_,
                se.code(),
                std::nullopt
            );
        }
    }
    else {
        auto t = hana::tuple<Args...>(std::forward<Args>(args)...);
        auto rest = hana::drop_back(std::move(t), hana::size_c<1>);
        auto&& back = hana::back(t);
        return hana::unpack(
            std::move(rest),
            [&](auto&&... rest_args) {
                static_assert(
                    std::is_constructible_v<
                        v5::auth_packet,
                        decltype(rest_args)...
                    >,
                    "v5::auth_packet is not constructible"
                );
                try {
                    return impl_type::async_auth_impl(
                        impl_,
                        error_code{},
                        v5::auth_packet{std::forward<decltype(rest_args)>(rest_args)...},
                        std::forward<decltype(back)>(back)
                    );
                }
                catch (system_error const& se) {
                    return impl_type::async_auth_impl(
                        impl_,
                        se.code(),
                        std::nullopt,
                        std::forward<decltype(back)>(back)
                    );
                }
            }
        );
    }
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_AUTH_HPP
