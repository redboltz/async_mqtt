// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_ADD_RETRY_IPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_ADD_RETRY_IPP

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/impl/endpoint_impl.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
add_retry_op {
    this_type_sp ep;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_ep{*ep};
        auto tim = std::make_shared<as::steady_timer>(a_ep.stream_.get_executor());
        tim->expires_at(std::chrono::steady_clock::time_point::max());
        tim->async_wait(
            as::append(
                force_move(self),
                tim
            )
        );
        a_ep.tim_retry_acq_pid_queue_.emplace_back(force_move(tim));
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code ec,
        std::shared_ptr<as::steady_timer> tim
    ) {
        auto& a_ep{*ep};
        if (!a_ep.packet_id_released_) {
            // intentional cancel
            auto it = std::find_if(
                a_ep.tim_retry_acq_pid_queue_.begin(),
                a_ep.tim_retry_acq_pid_queue_.end(),
                [&](auto const& elem) {
                    return elem.tim == tim;
                }
            );
            if (it != a_ep.tim_retry_acq_pid_queue_.end()) {
                a_ep.tim_retry_acq_pid_queue_.erase(it);
            }
        }
        self.complete(ec);
    }
};


template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::async_add_retry(
    this_type_sp impl,
    as::any_completion_handler<
        void(error_code)
    > handler
) {
    BOOST_ASSERT(impl);
    auto exe = impl->get_executor();
    return
        as::async_compose<
            as::any_completion_handler<
                void(error_code)
            >,
            void(error_code)
        >(
            add_retry_op{
                force_move(impl)
            },
            handler,
            exe
        );
}

} // namespace async_mqtt::detail

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#include <async_mqtt/detail/instantiate_helper.hpp>

#define ASYNC_MQTT_INSTANTIATE_EACH(a_role, a_size, a_protocol) \
namespace async_mqtt { \
namespace detail { \
template \
class basic_endpoint<a_role, a_size, a_protocol>; \
} // namespace async_mqtt

#define ASYNC_MQTT_PP_GENERATE(r, product) \
    BOOST_PP_EXPAND( \
        ASYNC_MQTT_INSTANTIATE_EACH \
        BOOST_PP_SEQ_TO_TUPLE( \
            product \
        ) \
    )

BOOST_PP_SEQ_FOR_EACH_PRODUCT(ASYNC_MQTT_PP_GENERATE, (ASYNC_MQTT_PP_ROLE)(ASYNC_MQTT_PP_SIZE)(ASYNC_MQTT_PP_PROTOCOL))

#undef ASYNC_MQTT_PP_GENERATE
#undef ASYNC_MQTT_INSTANTIATE_EACH

#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_ENDPOINT_ADD_RETRY_IPP
