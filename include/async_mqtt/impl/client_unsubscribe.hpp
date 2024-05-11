// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_UNSUBSCRIBE_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_UNSUBSCRIBE_HPP

#include <async_mqtt/impl/client_impl.hpp>
#include <async_mqtt/exception.hpp>
#include <async_mqtt/log.hpp>

namespace async_mqtt {

template <protocol_version Version, typename NextLayer>
struct client<Version, NextLayer>::
unsubscribe_op {
    this_type& cl;
    unsubscribe_packet packet;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_cl{cl};
        auto pid = packet.packet_id();
        auto a_packet{packet};
        a_cl.ep_->send(
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
        packet_id_t pid
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
            if (auto *p = pv->template get_if<unsuback_packet>()) {
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
    void(error_code, std::optional<unsuback_packet>)
)
client<Version, NextLayer>::unsubscribe(
    unsubscribe_packet packet,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << packet;
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::optional<unsuback_packet>)
        >(
            unsubscribe_op{
                *this,
                force_move(packet)
            },
            token
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_UNSUBSCRIBE_HPP
