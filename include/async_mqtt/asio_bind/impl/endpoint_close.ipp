// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_CLOSE_IPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_CLOSE_IPP

#include <async_mqtt/asio_bind/endpoint.hpp>
#include <async_mqtt/asio_bind/impl/endpoint_impl.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
close_op {
    this_type_sp ep;
    enum { dispatch, close, complete } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self,
        error_code = error_code{}
    ) {
        auto& a_ep{*ep};
        switch (state) {
        case dispatch: {
            state = close;
            as::dispatch(
                a_ep.get_executor(),
                force_move(self)
            );
        } break;
        case close:
            switch (a_ep.status_) {
            case close_status::open: {
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                        << "close initiate status:" << static_cast<int>(a_ep.status_);
                state = complete;
                a_ep.status_ = close_status::closing;
                a_ep.stream_.async_close(
                    force_move(self)
                );
            } break;
            case close_status::closing: {
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                    << "already close requested";
                a_ep.close_queue_.post(
                    force_move(self)
                );
            } break;
            case close_status::closed:
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                    << "already closed";
                self.complete();
            } break;
        case complete: {
            ASYNC_MQTT_LOG("mqtt_impl", trace)
                << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                << "close complete status:" << static_cast<int>(a_ep.status_);
            a_ep.status_ = close_status::closed;
            ASYNC_MQTT_LOG("mqtt_impl", trace)
                << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                << "process enqueued close";
            auto events = a_ep.con_.notify_closed();
            for (auto& event : events) {
                std::visit(
                    overload {
                        [&](async_mqtt::event::timer&& ev) {
                            auto op = ev.get_op();
                            BOOST_ASSERT(op == timer_op::cancel);
                            switch (ev.get_kind()) {
                            case timer_kind::pingreq_send:
                                cancel_pingreq_send_timer(ep);
                                break;
                            case timer_kind::pingreq_recv:
                                cancel_pingreq_recv_timer(ep);
                                break;
                            case timer_kind::pingresp_recv:
                                cancel_pingresp_recv_timer(ep);
                                break;
                            }
                        },
                        [&](async_mqtt::event::basic_packet_id_released<PacketIdBytes>&& ev) {
                            a_ep.notify_release_pid(ev.get());
                        },
                        [&](auto const&) {
                            BOOST_ASSERT(false);
                        }
                    },
                    force_move(event)
                );
            }
            a_ep.close_queue_.poll();
            self.complete();
        } break;
        }
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::async_close_impl(
    this_type_sp impl,
    as::any_completion_handler<
        void()
    > handler
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, impl.get())
        << "close";
    BOOST_ASSERT(impl);
    auto exe = impl->get_executor();
    as::async_compose<
        as::any_completion_handler<
            void()
        >,
        void()
    >(
        close_op{
            force_move(impl)
        },
        handler,
        exe
    );
}

} // namespace async_mqtt::detail

#include <async_mqtt/asio_bind/impl/endpoint_instantiate.hpp>

#endif // ASYNC_MQTT_IMPL_ENDPOINT_CLOSE_IPP
