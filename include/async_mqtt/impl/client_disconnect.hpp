// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_DISCONNECT_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_DISCONNECT_HPP

#include <boost/hana/tuple.hpp>
#include <boost/hana/back.hpp>
#include <boost/hana/drop_back.hpp>
#include <boost/hana/unpack.hpp>

#include <async_mqtt/client.hpp>
#include <async_mqtt/impl/client_impl.hpp>
#include <async_mqtt/util/log.hpp>

namespace async_mqtt {

namespace hana = boost::hana;

namespace detail {

template <protocol_version Version, typename NextLayer>
struct client_impl<Version, NextLayer>::
disconnect_op {
    this_type_sp cl;
    error_code ec;
    std::optional<typename client_type::disconnect_packet> packet;

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
client_impl<Version, NextLayer>::async_disconnect_impl(
    this_type_sp impl,
    error_code ec,
    std::optional<typename client_type::disconnect_packet> packet,
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
            disconnect_op{
                force_move(impl),
                ec,
                force_move(packet)
            },
            token,
            exe
        );
}

} // namespacec detail

template <protocol_version Version, typename NextLayer>
template <typename... Args>
auto
client<Version, NextLayer>::async_disconnect(Args&&... args) {
    BOOST_ASSERT(impl_);
    if constexpr (std::is_constructible_v<disconnect_packet, decltype(std::forward<Args>(args))...>) {
        try {
            return impl_type::async_disconnect_impl(
                impl_,
                error_code{},
                disconnect_packet{std::forward<Args>(args)...}
            );
        }
        catch (system_error const& se) {
            return impl_type::async_disconnect_impl(
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
                        disconnect_packet,
                        decltype(rest_args)...
                    >,
                    "disconnect_packet is not constructible"
                );
                try {
                    return impl_type::async_disconnect_impl(
                        impl_,
                        error_code{},
                        disconnect_packet{std::forward<decltype(rest_args)>(rest_args)...},
                        force_move(back)
                    );
                }
                catch (system_error const& se) {
                    return impl_type::async_disconnect_impl(
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

#endif // ASYNC_MQTT_IMPL_CLIENT_DISCONNECT_HPP
