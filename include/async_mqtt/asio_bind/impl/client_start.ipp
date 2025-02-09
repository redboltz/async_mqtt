// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_START_IPP)
#define ASYNC_MQTT_IMPL_CLIENT_START_IPP

#include <boost/asio/append.hpp>
#include <boost/asio/compose.hpp>

#include <async_mqtt/client.hpp>
#include <async_mqtt/asio_bind/impl/client_impl.hpp>
#include <async_mqtt/util/log.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

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
ASYNC_MQTT_HEADER_ONLY_INLINE
void
client_impl<Version, NextLayer>::async_start_impl(
    this_type_sp impl,
    error_code ec,
    std::optional<typename client_type::connect_packet> packet,
    as::any_completion_handler<
        void(error_code, std::optional<typename client_type::connack_packet>)
    > handler
) {
    BOOST_ASSERT(impl);
    if (packet) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, impl.get())
            << *packet;
    }
    auto exe = impl->get_executor();
    as::async_compose<
        as::any_completion_handler<
            void(error_code, std::optional<typename client_type::connack_packet>)
        >,
        void(error_code, std::optional<typename client_type::connack_packet>)
    >(
        start_op{
            force_move(impl),
            ec,
            force_move(packet)
        },
        handler,
        exe
    );
}

} // namespace async_mqtt::detail

#include <async_mqtt/asio_bind/impl/client_instantiate.hpp>

#endif // ASYNC_MQTT_IMPL_CLIENT_START_IPP
