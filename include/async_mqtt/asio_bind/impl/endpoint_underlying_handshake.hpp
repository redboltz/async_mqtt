// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASIO_BIND_IMPL_ENDPOINT_UNDERLYING_HANDSHAKE_HPP)
#define ASYNC_MQTT_ASIO_BIND_IMPL_ENDPOINT_UNDERLYING_HANDSHAKE_HPP

#include <boost/asio/any_completion_handler.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/back.hpp>
#include <boost/hana/drop_back.hpp>
#include <boost/hana/unpack.hpp>

#include <async_mqtt/asio_bind/endpoint.hpp>
#include <async_mqtt/asio_bind/impl/endpoint_impl.hpp>

namespace async_mqtt {

namespace hana = boost::hana;

namespace detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename ArgsTuple>
struct basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
underlying_handshake_op {
    this_type_sp ep;
    ArgsTuple args_tuple;
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
            hana::unpack(
                force_move(args_tuple),
                [&](auto&&... args) {
                    a_ep.stream_.async_underlying_handshake(
                        std::forward<decltype(args)>(args)...,
                        force_move(self)
                    );
                }
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
    typename ArgsTuple,
    typename CompletionToken
>
auto
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
async_underlying_handshake_impl(
    this_type_sp impl,
    ArgsTuple&& args_tuple,
    CompletionToken&& token
) {
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            underlying_handshake_op<ArgsTuple>{
                impl,
                std::forward<ArgsTuple>(args_tuple)
            },
            token,
            impl->get_executor()
        );
}

struct is_customize_handshake_callable_impl {
    template <
        typename Impl,
        typename... Args
    >
    static
    auto
    check(Impl&& impl, Args&&... args) ->
        decltype(
            layer_customize<
                typename Impl::next_layer_type
            >::async_handshake(
                impl.next_layer(),
                std::forward<Args>(args)...
            ),
            std::true_type()
        );

    template <
        typename Impl,
        typename... Args
    >
    static auto check(...) -> std::false_type;
};

template <
    typename Impl,
    typename... Args
>
struct is_customize_handshake_callable
    : decltype(
        is_customize_handshake_callable_impl::template check<
            Impl,
            Args...
        >(
            std::declval<Impl>(),
            std::declval<Args>()...
        )
    )
{
};

} // namespace detail

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename... Args>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_underlying_handshake(
    Args&&... args
) {
    BOOST_ASSERT(impl_);

    if constexpr(
        detail::is_customize_handshake_callable<
            impl_type,
            Args...
        >::value
    ) {
        return [this](auto&&... all_args) {
            auto all = hana::make_tuple(std::forward<decltype(all_args)>(all_args)...);
            constexpr auto N = sizeof...(all_args);
            return impl_type::async_underlying_handshake_impl(
                impl_,
                hana::drop_back(force_move(all), hana::size_c<1>),
                hana::at_c<N - 1>(force_move(all)) // token
            );
        }(std::forward<Args>(args)...);
    }
    else {
        auto all = hana::tuple<Args...>(std::forward<Args>(args)...);
        return impl_type::async_underlying_handshake_impl(
            impl_,
            force_move(all)
        );
    }
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_ASIO_BIND_IMPL_ENDPOINT_UNDERLYING_HANDSHAKE_HPP
