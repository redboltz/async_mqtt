// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/asio.hpp>

#include <async_mqtt/stream.hpp>
#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/predefined_underlying_layer.hpp>

namespace as = boost::asio;

int main() {
    as::io_context ioc;
    as::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    as::ip::tcp::endpoint endpoint{address, 1883};
    async_mqtt::stream<async_mqtt::role_type::client, async_mqtt::protocol::mqtt> ams{ioc.get_executor()};

    auto packet =
        async_mqtt::v3_1_1::publish_packet{
            42,
            async_mqtt::allocate_buffer("topic1"),
            async_mqtt::allocate_buffer("payload"),
            async_mqtt::pub::opts{}
        };
    ams.next_layer().async_connect(
        endpoint,
        [&]
        (boost::system::error_code const& ec) {
            std::cout << "connect: " << ec.message() << std::endl;
            if (ec) return;
            ams.write_packet(
                async_mqtt::force_move(packet),
                [&]
                (boost::system::error_code const& ec, std::size_t bytes_transferred) mutable {
                    std::cout << "write: " << ec.message() << " " << bytes_transferred << std::endl;
                    if (ec) return;
                    ams.read_packet(
                        [&]
                        (boost::system::error_code const& ec, async_mqtt::buffer buf) mutable {
                            std::cout << "read: " << ec.message() << " " << buf.size() << std::endl;
                            if (ec) return;
                            if (auto pv = async_mqtt::buffer_to_packet_variant(
                                    force_move(buf),
                                    async_mqtt::protocol_version::v3_1_1
                                )
                            ) {
                                pv.visit(
                                    async_mqtt::overload {
                                        [&](async_mqtt::v3_1_1::publish_packet const& p) {
                                            std::cout << "size:" << p.size() << std::endl;
                                            std::cout << "topic:" << p.topic() << std::endl;
                                            std::cout << "payload:";
                                            for (auto const& p : p.payload()) {
                                                std::cout << p;
                                            }
                                            std::cout << std::endl;
                                        },
                                        [](auto const&) {}
                                    }

                                );
                            }
                            else {
                                std::cout << "protocol error" << std::endl;
                            }
                        }
                    );
                }
            );
        }
    );

    ioc.run();
}
