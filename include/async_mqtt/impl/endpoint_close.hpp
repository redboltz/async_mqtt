// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_CLOSE_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_CLOSE_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint<Role, PacketIdBytes, NextLayer>::
close_op {
    this_type& ep;
    enum { dispatch, close, complete } state = dispatch;
    this_type_sp life_keeper = ep.shared_from_this();

    template <typename Self>
    void operator()(
        Self& self,
        error_code = error_code{}
    ) {
        switch (state) {
        case dispatch: {
            state = close;
            auto& a_ep{ep};
            as::dispatch(
                as::bind_executor(
                    a_ep.get_executor(),
                    force_move(self)
                )
            );
        } break;
        case close:
            switch (ep.status_) {
            case connection_status::connecting:
            case connection_status::connected:
            case connection_status::disconnecting: {
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                        << "close initiate status:" << static_cast<int>(ep.status_);
                state = complete;
                ep.status_ = connection_status::closing;
                auto& a_ep{ep};
                a_ep.stream_->async_close(
                    as::bind_executor(
                        a_ep.get_executor(),
                        force_move(self)
                    )
                );
            } break;
            case connection_status::closing: {
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                    << "already close requested";
                auto& a_ep{ep};
                auto exe = as::get_associated_executor(self);
                a_ep.close_queue_.post(
                    as::bind_executor(
                        exe,
                        force_move(self)
                    )
                );
            } break;
            case connection_status::closed:
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                    << "already closed";
                self.complete();
            } break;
        case complete:
            ASYNC_MQTT_LOG("mqtt_impl", trace)
                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                << "close complete status:" << static_cast<int>(ep.status_);
            ep.tim_pingreq_send_->cancel();
            ep.tim_pingreq_recv_->cancel();
            ep.tim_pingresp_recv_->cancel();
            ep.status_ = connection_status::closed;
            ASYNC_MQTT_LOG("mqtt_impl", trace)
                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                << "process enqueued close";
            ep.close_queue_.poll();
            self.complete();
            break;
        }
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void()
)
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_close(CompletionToken&& token) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "close";
    return
        as::async_compose<
            CompletionToken,
            void()
        >(
            close_op{
                *this
            },
            token
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_CLOSE_HPP
