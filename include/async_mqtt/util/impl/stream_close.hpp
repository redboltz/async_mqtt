// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_IMPL_STREAM_CLOSE_HPP)
#define ASYNC_MQTT_UTIL_IMPL_STREAM_CLOSE_HPP

#include <async_mqtt/util/stream.hpp>
#include <async_mqtt/protocol_version.hpp>

namespace async_mqtt {

namespace detail {

template <typename NextLayer>
struct stream_impl<NextLayer>::stream_close_op {
    using stream_type = this_type;
    using stream_type_sp = std::shared_ptr<stream_type>;

    std::shared_ptr<stream_type> strm;
    enum {
        dispatch,
        close,
        complete
    } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self,
        error_code ec = error_code{}
    ) {
        auto& a_strm{*strm};
        if (state == dispatch) {
            ASYNC_MQTT_LOG("mqtt_impl", trace)
                << ASYNC_MQTT_ADD_VALUE(address, strm.get())
                << "async operation start. state: dispatch -> close";
            state = close;
            as::dispatch(
                a_strm.get_executor(),
                as::append(
                    force_move(self),
                    error_code{},
                    std::ref(a_strm.nl_)
                )
            );
        }
        else {
            ASYNC_MQTT_LOG("mqtt_impl", trace)
                << ASYNC_MQTT_ADD_VALUE(address, strm.get())
                << "async operation finish. state: complete";
            BOOST_ASSERT(state == complete);
            a_strm.storing_cbs_.clear();
            a_strm.sending_cbs_.clear();
            self.complete(ec);
        }
    }

    template <typename Self, typename Layer>
    void operator()(
        Self& self,
        error_code /* ec */,
        std::reference_wrapper<Layer> stream
    ) {
        auto& a_strm{*strm};
        BOOST_ASSERT(state == close);
        if constexpr(has_async_close<Layer>::value) {
            ASYNC_MQTT_LOG("mqtt_impl", trace)
                << ASYNC_MQTT_ADD_VALUE(address, strm.get())
                << "has_async_close";
            if constexpr (has_next_layer<Layer>::value) {
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, strm.get())
                    << "call async_close";
                layer_customize<Layer>::async_close(
                    stream.get(),
                    as::append(
                        force_move(self),
                        std::ref(stream.get().next_layer())
                    )
                );
            }
            else {
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, strm.get())
                    << "NOT has_next_layer (lowest layer (TCP)). call async_close. "
                    << "state: close -> complete";
                state = complete;
                layer_customize<Layer>::async_close(
                    stream.get(),
                    force_move(self)
                );
            }
        }
        else {
            ASYNC_MQTT_LOG("mqtt_impl", trace)
                << ASYNC_MQTT_ADD_VALUE(address, strm.get())
                << "NOT has_async_close";
            if constexpr (has_next_layer<Layer>::value) {
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, strm.get())
                    << "skip this layer";
                as::dispatch(
                    a_strm.get_executor(),
                    as::append(
                        force_move(self),
                        error_code{},
                        std::ref(a_strm.nl_)
                    )
                );
            }
            else {
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, strm.get())
                    << "NOT has_next_layer (lowest layer (TCP)) state: close -> complete";
                state = complete;
                as::dispatch(
                    a_strm.get_executor(),
                    force_move(self)
                );
            }
        }
    }
};

} // namespace detail

template <typename NextLayer>
template<typename CompletionToken>
auto
stream<NextLayer>::async_close(
    CompletionToken&& token
) {
    BOOST_ASSERT(impl_);
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            typename impl_type::stream_close_op{
                impl_
            },
            token,
            get_executor()
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_IMPL_STREAM_CLOSE_HPP
