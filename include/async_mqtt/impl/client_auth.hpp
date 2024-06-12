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

template <protocol_version Version, typename NextLayer>
struct client<Version, NextLayer>::
auth_op {
    this_type& cl;
    v5::auth_packet packet;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_cl{cl};
        auto a_packet{packet};
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
client<Version, NextLayer>::async_auth_impl(
    v5::auth_packet packet,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << packet;
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            auth_op{
                *this,
                force_move(packet)
            },
            token,
            get_executor()
        );
}

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code)
)
client<Version, NextLayer>::async_auth_impl(
    error_code ec,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "auth: " << ec.message();
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            [ec](auto& self) {
                self.complete(ec);
            },
            token,
            get_executor()
        );
}

template <protocol_version Version, typename NextLayer>
template <typename... Args>
auto
client<Version, NextLayer>::async_auth(Args&&... args) {
    if constexpr (std::is_constructible_v<v5::auth_packet, decltype(std::forward<Args>(args))...>) {
        try {
            return async_auth_impl(v5::auth_packet{std::forward<Args>(args)...});
        }
        catch (system_error const& se) {
            return async_auth_impl(
                se.code()
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
                    return async_auth_impl(
                        v5::auth_packet{std::forward<decltype(rest_args)>(rest_args)...},
                        std::forward<decltype(back)>(back)
                    );
                }
                catch (system_error const& se) {
                    return async_auth_impl(
                        se.code(),
                        std::forward<decltype(back)>(back)
                    );
                }
            }
        );
    }
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_AUTH_HPP
