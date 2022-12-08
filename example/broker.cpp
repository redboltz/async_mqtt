// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <async_mqtt/predefined_underlying_layer.hpp>
#include <async_mqtt/endpoint_variant.hpp>

#include <async_mqtt/broker/broker.hpp>

namespace am = async_mqtt;
namespace as = boost::asio;


int main() {
    static_assert(
    sizeof(
        boost::beast::websocket::stream<
            boost::asio::basic_stream_socket<
                boost::asio::ip::tcp, boost::asio::io_context::basic_executor_type<
                    std::allocator<void>, 0
                >
            >,
            true
        >
    )
    != -1
    );

as::io_context ioc;
    as::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    as::ip::tcp::endpoint endpoint{address, 1883};
    as::ip::tcp::acceptor ac{ioc, endpoint};
    using epsp_t = am::endpoint_sp_variant<
        am::role::server,
        am::protocol::mqtt//,
        //am::protocol::ws
#if 0 //defined(ASYNC_MQTT_USE_TLS)
        ,
        am::protocol::mqtts,
        am::protocol::wss
#endif // defined(ASYNC_MQTT_USE_TLS)
    >;

    am::broker<
        am::protocol::mqtt//,
        //am::protocol::ws
#if 0 //defined(ASYNC_MQTT_USE_TLS)
        ,
        am::protocol::mqtts,
        am::protocol::wss
#endif // defined(ASYNC_MQTT_USE_TLS)
    > brk{ioc};

    std::function<void()> do_async_accept;
    do_async_accept =
        [&] {
            auto epsp = epsp_t::create(
                am::protocol_version::undetermined,
                am::protocol::mqtt{ioc.get_executor()}
            );
            epsp.strand();
            ac.async_accept(
                epsp.visit(
                    [](auto & ep) -> decltype(auto) {
                        return ep.stream().lowest_layer();
                    }
                ),
                [&do_async_accept, &brk, epsp]
                (boost::system::error_code const& ec) mutable {
                    std::cout << "accept: " << ec.message() << std::endl;
                    if (ec) return;
                    brk.handle_accept(force_move(epsp));
                    do_async_accept();
                }
            );
        };

    do_async_accept();
    ioc.run();
}
