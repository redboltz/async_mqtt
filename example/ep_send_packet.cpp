// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/asio.hpp>

#include <async_mqtt/endpoint_variant.hpp>
#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/predefined_underlying_layer.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>
#include <async_mqtt/util/hex_dump.hpp>

namespace as = boost::asio;
namespace am = async_mqtt;

int main() {
    as::io_context ioc;
    as::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    as::ip::tcp::endpoint endpoint{address, 1883};

    auto amep = am::endpoint<am::role::client, am::protocol::mqtt>(
        am::protocol_version::v3_1_1,
        am::protocol::mqtt{ioc.get_executor()}
    );

    auto str = as::make_strand(ioc);

    amep.stream().next_layer().async_connect(
        endpoint,
        [&]
        (am::error_code const& ec) {
            std::cout << "connect: " << ec.message() << std::endl;
            if (ec) return;

            amep.acquire_unique_packet_id(
                [&](auto pid_opt) {
                    if (pid_opt) {
                        auto packet =
                            am::v5::publish_packet{
                            *pid_opt,
                            am::allocate_buffer("topic1"),
                            am::allocate_buffer("payload"),
                            am::qos::at_least_once,
                            am::properties{
                                am::property::message_expiry_interval{1234}
                            }
                        };

                        std::cout << am::hex_dump(packet) << std::endl;;
                        amep.send(
                            am::force_move(packet),
                            [&]
                            (am::system_error const& ec) mutable {
                                std::cout << "write: " << ec.what() << std::endl;
                                if (ec) return;
                                amep.recv(
                                    as::bind_executor(
                                        str,
                                        [&]
                                        (am::packet_variant pv) mutable {
                                            BOOST_ASSERT(str.running_in_this_thread());
                                            if (pv) {
                                                std::cout << am::hex_dump(pv) << std::endl;
                                            }
                                            pv.visit(
                                                am::overload {
                                                    [&](am::v3_1_1::publish_packet const& p) {
                                                        std::cout << "size:" << p.size() << std::endl;
                                                        std::cout << "topic:" << p.topic() << std::endl;
                                                        std::cout << "payload:";
                                                        for (auto const& p : p.payload()) {
                                                            std::cout << p;
                                                        }
                                                        std::cout << std::endl;
                                                        amep.close(
                                                            [] {
                                                                std::cout << "closed" << std::endl;
                                                            }
                                                        );
                                                    },
                                                    [](auto const&) {}
                                                }
                                            );
                                        }
                                    )
                                );
                            }
                        );
                    }
                }
            );
        }
    );

    ioc.run();
}
