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
        error_code const& ec,
        packet_id_type pid
    ) {
        if (ec) {
            self.complete(ec, std::nullopt);
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
                std::nullopt
            );
        }
        else {
            auto pv = it->pv;
            idx.erase(it);
            if (auto *p = pv->template get_if<suback_packet>()) {
                auto ec =
                    [&] {
                        switch (p->entries().size()) {
                        case 0:
                            return make_error_code(disconnect_reason_code::protocol_error);
                        case 1:
                            return make_error_code(p->entries().back());
                        default: {
                            bool all_error = true;
                            bool any_error = false;
                            for (auto code : p->entries()) {
                                auto ec = make_error_code(code);
                                all_error = all_error && ec;
                                any_error = any_error || ec;
                            }
                            if (all_error) return make_error_code(mqtt_error::all_error_detected);
                            if (any_error) return make_error_code(mqtt_error::partial_error_detected);
                            return error_code{};
                        } break;
                        }
                    }();
                self.complete(ec, *p);
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
            token,
            get_executor()
        );
}

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code, std::optional<suback_packet>)
)
client<Version, NextLayer>::async_subscribe_impl(
    error_code ec,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "subscribe: " << ec.message();
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::optional<suback_packet>)
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
client<Version, NextLayer>::async_subscribe(Args&&... args) {
    if constexpr (std::is_constructible_v<subscribe_packet, decltype(std::forward<Args>(args))...>) {
        try {
            return async_subscribe_impl(subscribe_packet{std::forward<Args>(args)...});
        }
        catch (system_error const& se) {
            return async_subscribe_impl(
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
                        subscribe_packet,
                        decltype(rest_args)...
                    >,
                    "subscribe_packet is not constructible"
                );
                try {
                    return async_subscribe_impl(
                        subscribe_packet{std::forward<decltype(rest_args)>(rest_args)...},
                        std::forward<decltype(back)>(back)
                    );
                }
                catch (system_error const& se) {
                    return async_subscribe_impl(
                        se.code(),
                        std::forward<decltype(back)>(back)
                    );
                }
            }
        );
    }
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_SUBSCRIBE_HPP
