// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_START_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_START_HPP

#include <boost/asio/append.hpp>
#include <boost/asio/compose.hpp>

#include <boost/hana/tuple.hpp>
#include <boost/hana/back.hpp>
#include <boost/hana/drop_back.hpp>
#include <boost/hana/unpack.hpp>

#include <async_mqtt/impl/client_impl.hpp>
#include <async_mqtt/util/log.hpp>

namespace async_mqtt {

namespace hana = boost::hana;

namespace detail {
template <protocol_version Version, typename NextLayer>
struct client_impl<Version, NextLayer>::
start_op {
    this_type_sp cl;
    error_code ec;
    std::optional<typename client_type::connect_packet> packet;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_cl{*cl};
        if (ec) {
            self.complete(ec, std::nullopt);
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
        auto& a_cl{*cl};
        if (ec) {
            self.complete(ec, std::nullopt);
            return;
        }

        auto tim = std::make_shared<as::steady_timer>(a_cl.ep_.get_executor());
        tim->expires_at(std::chrono::steady_clock::time_point::max());
        a_cl.pid_tim_pv_res_col_.get_tim_idx().emplace(tim);
        recv_loop(cl);
        tim->async_wait(
            as::append(
                force_move(self),
                tim
            )
        );
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code /* ec */,
        std::shared_ptr<as::steady_timer> tim
    ) {
        auto& a_cl{*cl};
        auto& idx = a_cl.pid_tim_pv_res_col_.get_tim_idx();
        auto it = idx.find(tim);
        if (it == idx.end()) {
            self.complete(
                make_error_code(as::error::operation_aborted),
                std::nullopt
            );
        }
        else {
            auto pv = it->pv;
            idx.erase(it);
            if (auto *p = pv->template get_if<typename client_type::connack_packet>()) {
                self.complete(make_error_code(p->code()), *p);
            }
            else {
                self.complete(
                    make_error_code(disconnect_reason_code::protocol_error),
                    std::nullopt
                );
            }
        }
    }
};


template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
auto
client_impl<Version, NextLayer>::async_start_impl(
    this_type_sp impl,
    error_code ec,
    std::optional<typename client_type::connect_packet> packet,
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
            void(error_code, std::optional<typename client_type::connack_packet>)
        >(
            start_op{
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
client<Version, NextLayer>::async_start(Args&&... args) {
    BOOST_ASSERT(impl_);
    if constexpr (std::is_constructible_v<connect_packet, decltype(std::forward<Args>(args))...>) {
        try {
            return impl_type::async_start_impl(
                impl_,
                error_code{},
                connect_packet{std::forward<Args>(args)...}
            );
        }
        catch (system_error const& se) {
            return impl_type::async_start_impl(
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
                        connect_packet,
                        decltype(rest_args)...
                    >,
                    "connect_packet is not constructible"
                );
                try {
                    return impl_type::async_start_impl(
                        impl_,
                        error_code{},
                        connect_packet{
                            std::forward<std::remove_reference_t<decltype(rest_args)>>(rest_args)...
                        },
                        std::forward<decltype(back)>(back)
                    );
                }
                catch (system_error const& se) {
                    return impl_type::async_start_impl(
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

#endif // ASYNC_MQTT_IMPL_CLIENT_START_HPP
