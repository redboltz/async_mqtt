// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/asio.hpp>

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/predefined_underlying_layer.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

namespace as = boost::asio;

int main() {
    as::io_context ioc;
    as::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    as::ip::tcp::endpoint endpoint{address, 1883};
    async_mqtt::endpoint<async_mqtt::protocol::mqtt, async_mqtt::role::client> amep{
        async_mqtt::protocol_version::v3_1_1,
        ioc
    };

    auto packet =
        async_mqtt::v3_1_1::publish_packet{
            42,
            async_mqtt::allocate_buffer("topic1"),
            async_mqtt::allocate_buffer("payload"),
            async_mqtt::pub::opts{}
        };
    amep.stream().next_layer().async_connect(
        endpoint,
        [&]
        (async_mqtt::error_code const& ec) {
            std::cout << "connect: " << ec.message() << std::endl;
            if (ec) return;
            auto cbs = packet.const_buffer_sequence();
            auto r = async_mqtt::make_packet_range(cbs);
            std::cout << async_mqtt::hex_fump(std::cout, r.first, r.second);
            amep.send(
                async_mqtt::force_move(packet),
                [&]
                (async_mqtt::error_code const& ec) mutable {
                    std::cout << "write: " << ec.message() << std::endl;
                    if (ec) return;
                    amep.recv(
                        [&]
                        (async_mqtt::packet_variant pv) mutable {
                            auto cbs = pv.const_buffer_sequence();
                            auto r = async_mqtt::make_packet_range(cbs);
                            std::cout << async_mqtt::hex_fump(std::cout, r.first, r.second);
                            pv.visit(
                                async_mqtt::overload {
                                    [&](async_mqtt::v3_1_1::publish_packet const& p) {
                                        std::cout << "size:" << p.size() << std::endl;
                                        std::cout << "topic:" << p.topic() << std::endl;
                                        std::cout << "payload:" << p.payload_as_buffer() << std::endl;
                                    },
                                    [](auto const&) {}
                                }
                            );
                        }
                    );
                }
            );
        }
    );

    ioc.run();
}
