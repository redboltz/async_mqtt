// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_ADD_RETRY_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_ADD_RETRY_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint<Role, PacketIdBytes, NextLayer>::
add_retry_op {
    this_type& ep;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto tim = std::make_shared<as::steady_timer>(ep.stream_->get_executor());
        tim->expires_at(std::chrono::steady_clock::time_point::max());
        auto& a_ep{ep};
        tim->async_wait(
            as::bind_executor(
                a_ep.get_executor(),
                force_move(self)
            )
        );
        a_ep.tim_retry_acq_pid_queue_.emplace_back(force_move(tim));
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code ec
    ) {
        self.complete(ec);
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code)
)
basic_endpoint<Role, PacketIdBytes, NextLayer>::add_retry(
    CompletionToken&& token
) {
    return
        as::async_compose<
            CompletionToken,
            void(error_code ec)
        >(
            add_retry_op{
                *this
            },
            token
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_ADD_RETRY_HPP
