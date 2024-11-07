// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_IMPL_STREAM_UNDERLYING_HANDSHAKE_HPP)
#define ASYNC_MQTT_UTIL_IMPL_STREAM_UNDERLYING_HANDSHAKE_HPP

#include <async_mqtt/error.hpp>
#include <async_mqtt/util/stream.hpp>
#include <async_mqtt/util/shared_ptr_array.hpp>

#include <boost/hana/tuple.hpp>
#include <boost/hana/unpack.hpp>

namespace async_mqtt {

namespace hana = boost::hana;

namespace detail {

template <typename NextLayer>
template <typename ArgsTuple>
struct stream_impl<NextLayer>::stream_underlying_handshake_op {
    using stream_type = this_type;
    using stream_type_sp = std::shared_ptr<stream_type>;
    using next_layer_type = stream_type::next_layer_type;

    std::shared_ptr<stream_type> strm;
    ArgsTuple args;

    enum { dispatch, work, complete } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_strm{*strm};
        switch (state) {
        case dispatch: {
            state = work;
            as::dispatch(
                a_strm.get_executor(),
                force_move(self)
            );
        } break;
        case work: {
            state = complete;
            hana::unpack(
                std::move(args),
                [&](auto&&... rest_args) {
                    layer_customize<next_layer_type>::async_handshake(
                        a_strm.nl_,
                        std::forward<decltype(rest_args)>(rest_args)...,
                        force_move(self)
                    );
                }
            );
            break;
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    // finish underlying_handshake
    template <typename Self>
    void operator()(
        Self& self,
        error_code const& ec
    ) {
        self.complete(ec);
    }
};

} // namespace detail

template <typename NextLayer>
template <
    typename ArgsTuple,
    typename CompletionToken
>
auto
stream<NextLayer>::async_underlying_handshake(
    ArgsTuple&& args_tuple,
    CompletionToken&& token
) {
    BOOST_ASSERT(impl_);
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            typename impl_type::template stream_underlying_handshake_op<ArgsTuple>{
                impl_,
                std::forward<ArgsTuple>(args_tuple)
            },
            token,
            get_executor()
        );
}


} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_IMPL_STREAM_UNDERLYING_HANDSHAKE_HPP
