// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_SUBSCRIBE_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_SUBSCRIBE_HPP

#include <boost/hana/tuple.hpp>
#include <boost/hana/back.hpp>
#include <boost/hana/drop_back.hpp>
#include <boost/hana/unpack.hpp>

#include <async_mqtt/impl/client_impl.hpp>
#include <async_mqtt/exception.hpp>
#include <async_mqtt/util/log.hpp>

namespace async_mqtt {

namespace hana = boost::hana;

template <protocol_version Version, typename NextLayer>
struct client<Version, NextLayer>::
subscribe_op {
    this_type& cl;
    subscribe_packet packet;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_cl{cl};
        auto pid = packet.packet_id();
        auto a_packet{packet};
        a_cl.ep_->async_send(
            force_move(a_packet),
            as::append(
                force_move(self),
                pid
            )
        );
    }

    template <typename Self>
    void operator()(
        Self& self,
        system_error const& se,
        packet_id_type pid
    ) {
        if (se) {
            self.complete(se.code(), std::nullopt);
            return;
        }

        auto tim = std::make_shared<as::steady_timer>(cl.ep_->get_executor());
        tim->expires_at(std::chrono::steady_clock::time_point::max());
        cl.pid_tim_pv_res_col_.get_tim_idx().emplace(pid, tim);
        auto& a_cl{cl};
        tim->async_wait(
            as::bind_executor(
                a_cl.get_executor(),
                as::append(
                    force_move(self),
                    tim
                )
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
            if (auto *p = pv->template get_if<suback_packet>()) {
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
    void(error_code, std::optional<suback_packet>)
)
client<Version, NextLayer>::async_subscribe_impl(
    subscribe_packet packet,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << packet;
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::optional<suback_packet>)
        >(
            subscribe_op{
                *this,
                force_move(packet)
            },
            token
        );
}

template <protocol_version Version, typename NextLayer>
template <typename... Args>
auto
client<Version, NextLayer>::async_subscribe(Args&&... args) {
    if constexpr (std::is_constructible_v<subscribe_packet, decltype(std::forward<Args>(args))...>) {
        return async_subscribe_impl(std::forward<Args>(args)...);
    }
    else {
        auto t = hana::tuple<Args...>(std::forward<Args>(args)...);
        auto rest = hana::drop_back(std::move(t), hana::size_c<1>);
        auto&& back = hana::back(t);
        return hana::unpack(
            std::move(rest),
            [&](auto&&... rest_args) {
                if constexpr(
                    std::is_constructible_v<
                        subscribe_packet,
                        decltype(rest_args)...
                    >
                ) {
                    return async_subscribe_impl(
                        subscribe_packet(std::forward<decltype(rest_args)>(rest_args)...),
                        std::forward<decltype(back)>(back)
                    );
                }
                else {
                    static_assert(false, "subscribe_packet is not constructible");
                    return async_subscribe_impl(
                        subscribe_packet(std::forward<decltype(rest_args)>(rest_args)...),
                        std::forward<decltype(back)>(back)
                    );
                }
            }
        );
    }
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_SUBSCRIBE_HPP
