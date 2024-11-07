// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_UNDERLYING_HANDSHAKE_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_UNDERLYING_HANDSHAKE_HPP

#include <boost/hana/tuple.hpp>
#include <boost/hana/back.hpp>
#include <boost/hana/drop_back.hpp>

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

} // namespace detail

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename... Args>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_underlying_handshake(
    Args&&... args
) {
    BOOST_ASSERT(impl_);
    auto t = hana::tuple<Args...>(std::forward<Args>(args)...);
    auto rest = hana::drop_back(force_move(t), hana::size_c<1>);
    auto&& back = hana::back(t);

    return as::async_compose<
        decltype(back),
        void(error_code)
    >(
        typename impl_type::template underlying_handshake_op<decltype(rest)>{
            impl_,
            force_move(rest)
        },
        back,
        get_executor()
    );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_UNDERLYING_HANDSHAKE_HPP
