// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_CLOSE_IPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_CLOSE_IPP

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/impl/endpoint_impl.hpp>
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
        case complete:
            ASYNC_MQTT_LOG("mqtt_impl", trace)
                << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                << "close complete status:" << static_cast<int>(a_ep.status_);
            a_ep.tim_pingreq_send_.cancel();
            a_ep.tim_pingreq_recv_.cancel();
            a_ep.tim_pingresp_recv_.cancel();
            a_ep.status_ = close_status::closed;
            ASYNC_MQTT_LOG("mqtt_impl", trace)
                << ASYNC_MQTT_ADD_VALUE(address, &a_ep)
                << "process enqueued close";
            a_ep.con_.notify_closed();
            a_ep.close_queue_.poll();
            self.complete();
            break;
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

#endif // ASYNC_MQTT_IMPL_ENDPOINT_CLOSE_IPP
