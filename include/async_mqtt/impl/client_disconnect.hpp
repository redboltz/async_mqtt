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
#include <async_mqtt/util/log.hpp>

namespace async_mqtt {

namespace hana = boost::hana;

template <protocol_version Version, typename NextLayer>
struct client<Version, NextLayer>::
disconnect_op {
    this_type& cl;
    error_code ec;
    std::optional<disconnect_packet> packet;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        if (ec) {
            self.complete(ec);
            return;
        }
        auto& a_cl{cl};
        auto a_packet{force_move(*packet)};
        a_cl.ep_->async_send(
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
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code)
)
client<Version, NextLayer>::async_disconnect_impl(
    error_code ec,
    std::optional<disconnect_packet> packet,
    CompletionToken&& token
) {
    if (packet) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << *packet;
    }
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            disconnect_op{
                *this,
                ec,
                force_move(packet)
            },
            token,
            get_executor()
        );
}

template <protocol_version Version, typename NextLayer>
template <typename... Args>
auto
client<Version, NextLayer>::async_disconnect(Args&&... args) {
    if constexpr (std::is_constructible_v<disconnect_packet, decltype(std::forward<Args>(args))...>) {
        try {
            return async_disconnect_impl(
                error_code{},
                disconnect_packet{std::forward<Args>(args)...}
            );
        }
        catch (system_error const& se) {
            return async_disconnect_impl(
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
                        disconnect_packet,
                        decltype(rest_args)...
                    >,
                    "disconnect_packet is not constructible"
                );
                try {
                    return async_disconnect_impl(
                        error_code{},
                        disconnect_packet{std::forward<decltype(rest_args)>(rest_args)...},
                        std::forward<decltype(back)>(back)
                    );
                }
                catch (system_error const& se) {
                    return async_disconnect_impl(
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

#endif // ASYNC_MQTT_IMPL_CLIENT_DISCONNECT_HPP
