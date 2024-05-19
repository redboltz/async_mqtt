// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_PUBLISH_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_PUBLISH_HPP

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
publish_op {
    this_type& cl;
    publish_packet packet;

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
            self.complete(se.code(), pubres_t{});
            return;
        }
        if (pid == 0) {
            // QoS: at_most_once
            self.complete(se.code(), pubres_t{});
            return;
        }
        auto tim = std::make_shared<as::steady_timer>(cl.ep_->get_executor());
        tim->expires_at(std::chrono::steady_clock::time_point::max());
        cl.pid_tim_pv_res_col_.get_tim_idx().emplace(pid, tim);
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
                pubres_t{}
            );
        }
        else {
            auto res = it->res;
            idx.erase(it);
            self.complete(error_code{}, force_move(res));
        }
    }
};

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code, pubres_t)
)
client<Version, NextLayer>::async_publish_impl(
    publish_packet packet,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << packet;
    return
        as::async_compose<
            CompletionToken,
            void(error_code, pubres_t)
        >(
            publish_op{
                *this,
                force_move(packet)
            },
            token
        );
}

template <protocol_version Version, typename NextLayer>
template <typename... Args>
auto
client<Version, NextLayer>::async_publish(Args&&... args) {
    if constexpr (std::is_constructible_v<publish_packet, decltype(std::forward<Args>(args))...>) {
        return async_publish_impl(publish_packet{std::forward<Args>(args)...});
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
                        publish_packet,
                        decltype(rest_args)...
                    >,
                    "publish_packet is not constructible"
                );
                return async_publish_impl(
                    publish_packet{std::forward<decltype(rest_args)>(rest_args)...},
                    std::forward<decltype(back)>(back)
                );
            }
        );
    }
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_PUBLISH_HPP
