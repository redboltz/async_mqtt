// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_UNDERLYING_HANDSHAKE_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_UNDERLYING_HANDSHAKE_HPP

#include <boost/asio/any_completion_handler.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/back.hpp>
#include <boost/hana/drop_back.hpp>
#include <boost/hana/type.hpp>
#include <boost/hana/unpack.hpp>
#include <boost/hana/or.hpp>

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

namespace hana = boost::hana;

namespace detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename ArgsTuple>
struct basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
underlying_handshake_op {
    this_type_sp ep;
    ArgsTuple args;
    enum { dispatch, underlying_handshake, complete } state = dispatch;
    template <typename Self>
    void operator()(
        Self& self,
        error_code ec = error_code{}
    ) {
        auto& a_ep{*ep};
        switch (state) {
        case dispatch: {
            state = underlying_handshake;
            as::dispatch(
                a_ep.get_executor(),
                force_move(self)
            );
        } break;
        case underlying_handshake:
            state = complete;
            ASYNC_MQTT_LOG("mqtt_api", info)
                << ASYNC_MQTT_ADD_VALUE(address, this)
                << "underlying_handshake";
            a_ep.stream_.async_underlying_handshake(
                force_move(args),
                force_move(self)
            );
            break;
        case complete:
            if (!ec) {
                a_ep.status_ = close_status::open;
            }
            self.complete(ec);
            break;
        }
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <
    typename TupleArgs,
    typename CompletionToken
>
auto
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
async_underlying_handshake_impl(
    this_type_sp impl,
    TupleArgs&& tuple_args,
    CompletionToken&& token
) {
    auto exe = impl->get_executor();
    return as::async_compose<
        CompletionToken,
        void(error_code)
    >(
        underlying_handshake_op<TupleArgs>{
            force_move(impl),
            std::forward<TupleArgs>(tuple_args)
        },
        token,
        exe
    );
}

} // namespace detail

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename... Args>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_underlying_handshake(
    Args&&... args
) {
    BOOST_ASSERT(impl_);
    auto t = hana::make_tuple(std::forward<Args>(args)...);
    auto back = hana::back(t);
    auto rest = hana::drop_back(t, hana::size_c<1>);
#if 0
    auto const is_callable =
        [](auto&& callable, auto&& args) {
            return hana::unpack(
                args,
                hana::is_valid(
                    [](auto&&... args)
                    -> decltype(callable(args...))
                    {}
                )
            );
        };
#endif

    constexpr auto const is_callable2 =
        hana::is_valid(
            [](auto&& token)
            -> decltype(token(error_code{}))
            {}
        );
    constexpr auto const is_callable3 =
        hana::is_valid(
            [](auto&& as_tuple)
            -> decltype(as_tuple.token_(error_code{}))
            {}
        );
#if 0
    auto const is_callable_with_nl =
        [](auto&& callable, auto&& args) {
            return hana::unpack(
                args,
                hana::is_valid(
                    [](auto&&... args)
                    -> decltype(callable(std::declval<next_layer_type&>(), args...))
                    {}
                )
            );
        };
#endif
    if constexpr(
        hana::or_(
        is_callable2(
            back
        )
        ,
        is_callable3(
            back
        )
        )
        || std::
#if 0
        is_callable(
            layer_customize<next_layer_type>::template async_handshake<decltype(back)>,
            t
        )
#endif
    ) {
        return impl_type::async_underlying_handshake_impl(
            impl_,
            force_move(rest),
            force_move(back)
        );
    }
    else {
        return impl_type::async_underlying_handshake_impl(
            impl_,
            force_move(t)
        );
    }
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_UNDERLYING_HANDSHAKE_HPP
