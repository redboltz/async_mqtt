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

template <protocol_version Version, typename NextLayer>
struct client<Version, NextLayer>::
start_op {
    this_type& cl;
    connect_packet packet;

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
        if (ec) {
            self.complete(ec, std::nullopt);
            return;
        }

        auto tim = std::make_shared<as::steady_timer>(cl.ep_->get_executor());
        tim->expires_at(std::chrono::steady_clock::time_point::max());
        cl.pid_tim_pv_res_col_.get_tim_idx().emplace(tim);
        cl.recv_loop();
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
        auto& idx = cl.pid_tim_pv_res_col_.get_tim_idx();
        auto it = idx.find(tim);
        if (it == idx.end()) {
            self.complete(
                errc::make_error_code(sys::errc::operation_canceled),
                std::nullopt
            );
        }
        else {
            auto pv = it->pv;
            idx.erase(it);
            if (auto *p = pv->template get_if<connack_packet>()) {
                self.complete(error_code{}, *p);
            }
            else {
                self.complete(
                    errc::make_error_code(sys::errc::protocol_error),
                    std::nullopt
                );
            }
        }
    }
};


template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code, std::optional<connack_packet>)
)
client<Version, NextLayer>::async_start_impl(
    connect_packet packet,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "start: " << packet;
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::optional<connack_packet>)
        >(
            start_op{
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
    void(error_code, std::optional<connack_packet>)
)
client<Version, NextLayer>::async_start_impl(
    error_code ec,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "start: " << ec.message();
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::optional<connack_packet>)
        >(
            [ec](auto& self) {
                self.complete(ec, std::nullopt);
            },
            token,
            get_executor()
        );
}

template <protocol_version Version, typename NextLayer>
template <typename... Args>
auto
client<Version, NextLayer>::async_start(Args&&... args) {
    if constexpr (std::is_constructible_v<connect_packet, decltype(std::forward<Args>(args))...>) {
        try {
            return async_start_impl(connect_packet{std::forward<Args>(args)...});
        }
        catch (system_error const& se) {
            return async_start_impl(
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
                        connect_packet,
                        decltype(rest_args)...
                    >,
                    "connect_packet is not constructible"
                );
                try {
                    return async_start_impl(
                        connect_packet{std::forward<std::remove_reference_t<decltype(rest_args)>>(rest_args)...},
                        std::forward<decltype(back)>(back)
                    );
                }
                catch (system_error const& se) {
                    return async_start_impl(
                        se.code(),
                        std::forward<decltype(back)>(back)
                    );
                }
            }
        );
    }
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_START_HPP
