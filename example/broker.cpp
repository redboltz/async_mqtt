// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <async_mqtt/predefined_underlying_layer.hpp>
#include <async_mqtt/broker/endpoint_variant.hpp>

#include <async_mqtt/broker/broker.hpp>

namespace am = async_mqtt;
namespace as = boost::asio;

int main() {

as::io_context ioc;
    as::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");

    as::ip::tcp::endpoint mqtt_endpoint{address, 1883};
    as::ip::tcp::acceptor mqtt_ac{ioc, mqtt_endpoint};
#if 0
    as::ip::tcp::endpoint ws_endpoint{address, 80};
    as::ip::tcp::acceptor ws_ac{ioc, ws_endpoint};
#endif

    using ep_t = am::endpoint_variant<
        am::role::server,
        am::protocol::mqtt
#if 0
        ,
        am::protocol::ws
#endif
#if defined(ASYNC_MQTT_USE_TLS)
        ,
        am::protocol::mqtts,
        am::protocol::wss
#endif // defined(ASYNC_MQTT_USE_TLS)
    >;

    using epsp_t = am::endpoint_variant_sp<
        am::role::server,
        am::protocol::mqtt
#if 0
        ,
        am::protocol::ws
#endif
#if defined(ASYNC_MQTT_USE_TLS)
        ,
        am::protocol::mqtts,
        am::protocol::wss
#endif // defined(ASYNC_MQTT_USE_TLS)
    >;

    am::broker<
        2,
        am::protocol::mqtt
#if 0
        ,
        am::protocol::ws
#endif
#if defined(ASYNC_MQTT_USE_TLS)
        ,
        am::protocol::mqtts,
        am::protocol::wss
#endif // defined(ASYNC_MQTT_USE_TLS)
    > brk{ioc};
    std::function<void()> mqtt_async_accept;
    mqtt_async_accept =
        [&] {
            auto epsp =
                std::make_shared<ep_t>(
                    am::endpoint<am::role::server, am::protocol::mqtt>{
                        am::protocol_version::undetermined,
                        am::protocol::mqtt{ioc.get_executor()}
                    }
                );
            mqtt_ac.async_accept(
                std::get<am::endpoint<am::role::server, am::protocol::mqtt>>(*epsp).stream().lowest_layer(),
                [&mqtt_async_accept, &brk, epsp]
                (boost::system::error_code const& ec) mutable {

                    std::cout << "accept: " << ec.message() << std::endl;
                    if (ec) return;
                    brk.handle_accept(epsp);
                    mqtt_async_accept();
                }
            );
        };

    mqtt_async_accept();

    std::function<void()> ws_async_accept;
#if 0
    ws_async_accept =
        [&] {
            auto epsp = epsp_t::create(
                am::protocol_version::undetermined,
                am::protocol::ws{ioc.get_executor()}
            );
            ws_ac.async_accept(
                epsp.visit(
                    [](auto & ep) -> decltype(auto) {
                        return ep.stream().lowest_layer();
                    }
                ),
                [&ws_async_accept, &brk, epsp]
                (boost::system::error_code const& ec) mutable {
                    std::cout << "accept: " << ec.message() << std::endl;
                    if (ec) {
                        ws_async_accept();
                        return;
                    }
                    epsp.visit(
                        am::overload {
                            [&](am::endpoint<am::role::server, am::protocol::ws>& ep) {
                                ep.stream().next_layer().async_accept(
                                    [&ws_async_accept, &brk, epsp]
                                    (boost::system::error_code const& ec) mutable {
                                        std::cout << "accept: " << ec.message() << std::endl;
                                        if (ec) return;
                                        brk.handle_accept(force_move(epsp));
                                        ws_async_accept();
                                    }
                                );
                            },
                            [&](auto&){}
                        }
                    );
                }
            );
        };

    ws_async_accept();
#endif

    ioc.run();
}
