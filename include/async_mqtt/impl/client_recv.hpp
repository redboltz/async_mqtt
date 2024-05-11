// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_RECV_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_RECV_HPP

#include <boost/asio/dispatch.hpp>

#include <async_mqtt/impl/client_impl.hpp>
#include <async_mqtt/exception.hpp>
#include <async_mqtt/log.hpp>

namespace async_mqtt {

template <protocol_version Version, typename NextLayer>
struct client<Version, NextLayer>::
recv_op {
    this_type& cl;
    enum { dispatch, recv, complete } state = dispatch;
    template <typename Self>
    void operator()(
        Self& self
    ) {
        if (state == dispatch) {
            state = recv;
            auto& a_cl{cl};
            as::dispatch(
                as::bind_executor(
                    a_cl.ep_->get_executor(),
                    force_move(self)
                )
            );
        }
        else {
            BOOST_ASSERT(state == recv);
            state = complete;
            if (cl.recv_queue_.empty()) {
                cl.recv_queue_inserted_ = false;
                auto tim = std::make_shared<as::steady_timer>(
                    cl.ep_->get_executor()
                );
                cl.tim_notify_publish_recv_.expires_at(
                    std::chrono::steady_clock::time_point::max()
                );
                auto& a_cl{cl};
                a_cl.tim_notify_publish_recv_.async_wait(
                    as::bind_executor(
                        a_cl.get_executor(),
                        as::append(
                            force_move(self),
                            a_cl.recv_queue_inserted_
                        )
                    )
                );
            }
            else {
                auto [ec, publish_opt, disconnect_opt] = cl.recv_queue_.front();
                cl.recv_queue_.pop_front();
                self.complete(
                    ec,
                    force_move(publish_opt),
                    force_move(disconnect_opt)
                );
            }
        }
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code /* ec */,
        bool get_queue
    ) {
        BOOST_ASSERT(state == complete);
        if (get_queue) {
            auto [ec, publish_opt, disconnect_opt] = cl.recv_queue_.front();
            cl.recv_queue_.pop_front();
            self.complete(
                ec,
                force_move(publish_opt),
                force_move(disconnect_opt)
            );
        }
        else {
            self.complete(
                errc::make_error_code(sys::errc::operation_canceled),
                std::nullopt,
                std::nullopt
            );
        }
    }
};

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code, std::optional<publish_packet>, std::optional<disconnect_packet>)
)
client<Version, NextLayer>::recv(
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "recv";
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::optional<publish_packet>, std::optional<disconnect_packet>)
        >(
            recv_op{
                *this
            },
            token
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_RECV_HPP
