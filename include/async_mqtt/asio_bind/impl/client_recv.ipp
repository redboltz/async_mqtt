// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_RECV_IPP)
#define ASYNC_MQTT_IMPL_CLIENT_RECV_IPP

#include <boost/asio/dispatch.hpp>

#include <async_mqtt/client.hpp>
#include <async_mqtt/asio_bind/impl/client_impl.hpp>
#include <async_mqtt/protocol/packet/packet_variant.hpp>
#include <async_mqtt/util/log.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

template <protocol_version Version, typename NextLayer>
struct client_impl<Version, NextLayer>::
recv_op {
    this_type_sp cl;
    enum { dispatch, recv, complete } state = dispatch;
    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_cl{*cl};
        if (state == dispatch) {
            state = recv;
            as::dispatch(
                a_cl.ep_.get_executor(),
                force_move(self)
            );
        }
        else {
            BOOST_ASSERT(state == recv);
            state = complete;
            if (a_cl.recv_queue_.empty()) {
                a_cl.recv_queue_inserted_ = false;
                auto tim = std::make_shared<as::steady_timer>(
                    a_cl.ep_.get_executor()
                );
                a_cl.tim_notify_publish_recv_.expires_at(
                    std::chrono::steady_clock::time_point::max()
                );
                a_cl.tim_notify_publish_recv_.async_wait(
                    force_move(self)
                );
            }
            else {
                auto [ec, pv] = force_move(a_cl.recv_queue_.front());
                a_cl.recv_queue_.pop_front();
                self.complete(
                    ec,
                    force_move(pv)
                );
            }
        }
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code /* ec */
    ) {
        BOOST_ASSERT(state == complete);
        auto& a_cl{*cl};
        if (a_cl.recv_queue_inserted_) {
            auto [ec, pv] = force_move(a_cl.recv_queue_.front());
            a_cl.recv_queue_.pop_front();
            self.complete(
                ec,
                force_move(pv)
            );
        }
        else {
            self.complete(
                make_error_code(as::error::operation_aborted),
                std::nullopt
            );
        }
    }
};

template <protocol_version Version, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
client_impl<Version, NextLayer>::async_recv(
    this_type_sp impl,
    as::any_completion_handler<
        void(error_code, std::optional<packet_variant>)
    > handler
) {
    BOOST_ASSERT(impl);
    auto exe = impl->get_executor();
    as::async_compose<
        as::any_completion_handler<
            void(error_code, std::optional<packet_variant>)
        >,
        void(error_code, std::optional<packet_variant>)
    >(
        recv_op{
            force_move(impl),
        },
        handler,
        exe
    );
}

} // namespace async_mqtt::detail

#include <async_mqtt/asio_bind/impl/client_instantiate.hpp>

#endif // ASYNC_MQTT_IMPL_CLIENT_RECV_IPP
