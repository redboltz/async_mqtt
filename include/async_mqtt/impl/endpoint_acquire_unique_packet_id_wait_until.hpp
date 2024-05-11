// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_ACQUIRE_UNIQUE_PACKET_ID_WAIT_UNTIL_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_ACQUIRE_UNIQUE_PACKET_ID_WAIT_UNTIL_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint<Role, PacketIdBytes, NextLayer>::
acquire_unique_packet_id_wait_until_op {
    this_type& ep;
    this_type_wp retry_wp = ep.weak_from_this();
    std::optional<packet_id_t> pid_opt = std::nullopt;
    enum { dispatch, complete } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self,
        error_code ec = error_code{}
    ) {
        if (retry_wp.expired()) return;
        switch (state) {
        case dispatch: {
            state = complete;
            auto& a_ep{ep};
            as::dispatch(
                as::bind_executor(
                    a_ep.get_executor(),
                    force_move(self)
                )
            );
        } break;
        case complete: {
            auto acq_proc =
                [&] {
                    pid_opt = ep.pid_man_.acquire_unique_id();
                    if (pid_opt) {
                        self.complete(*pid_opt);
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", warning)
                            << ASYNC_MQTT_ADD_VALUE(address, &ep)
                            << "packet_id is fully allocated. waiting release";
                        // infinity timer. cancel is retry trigger.
                        auto& a_ep{ep};
                        a_ep.add_retry(
                            as::bind_executor(
                                a_ep.get_executor(),
                                force_move(self)
                            )
                        );
                    }
                };

            if (ec == errc::operation_canceled) {
                ep.complete_retry_one();
                acq_proc();
            }
            else if (ep.has_retry()) {
                ASYNC_MQTT_LOG("mqtt_impl", warning)
                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                    << "packet_id waiter exists. add the end of waiter queue";
                // infinity timer. cancel is retry trigger.
                auto& a_ep{ep};
                a_ep.add_retry(
                    as::bind_executor(
                        a_ep.get_executor(),
                        force_move(self)
                    )
                );
            }
            else {
                acq_proc();
            }
        } break;
        }
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::acquire_unique_packet_id_wait_until(
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "acquire_unique_packet_id_wait_until";
    return
        as::async_compose<
            CompletionToken,
            void(packet_id_t)
        >(
            acquire_unique_packet_id_wait_until_op{
                *this
            },
            token
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_ACQUIRE_UNIQUE_PACKET_ID_WAIT_UNTIL_HPP
