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

template <typename NextLayer>
struct stream<NextLayer>::stream_close_op {
    using stream_type = this_type;
    using stream_type_sp = std::shared_ptr<stream_type>;

    stream_type& strm;
    enum {
        dispatch,
        close,
        complete
    } state = dispatch;
    stream_type_sp life_keeper = strm.shared_from_this();

    template <typename Self>
    void operator()(
        Self& self,
        error_code ec = error_code{}
    ) {
        if (state == dispatch) {
            state = close;
            auto& a_strm{strm};
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
            BOOST_ASSERT(state == complete);
            strm.storing_cbs_.clear();
            strm.sending_cbs_.clear();
            self.complete(ec);
        }
    }

    template <typename Self, typename Layer>
    void operator()(
        Self& self,
        error_code /* ec */,
        std::reference_wrapper<Layer> stream
    ) {
        BOOST_ASSERT(state == close);
        auto& a_strm{strm};
        if constexpr(has_async_close<Layer>::value) {
            if constexpr (has_next_layer<Layer>::value) {
                layer_customize<Layer>::async_close(
                    stream.get(),
                    as::append(
                        force_move(self),
                        std::ref(stream.get().next_layer())
                    )
                );
            }
            else {
                state = complete;
                layer_customize<Layer>::async_close(
                    stream.get(),
                    force_move(self)
                );
            }
        }
        else {
            if constexpr (has_next_layer<Layer>::value) {
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
                state = complete;
                as::dispatch(
                    a_strm.get_executor(),
                    force_move(self)
                );
            }
        }
    }
};

template <typename NextLayer>
template<typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void()
)
stream<NextLayer>::async_close(
    CompletionToken&& token
) {
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            stream_close_op{
                *this
            },
            token
        );
}


} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_IMPL_STREAM_CLOSE_HPP
