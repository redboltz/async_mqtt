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

int main() {
    as::io_context ioc;
    as::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    as::ip::tcp::endpoint endpoint{address, 1883};
    auto amep_sp = std::make_shared<
        async_mqtt::endpoint<async_mqtt::role::client, async_mqtt::protocol::mqtt>
    >(
        async_mqtt::protocol_version::v3_1_1,
        ioc
    );
    async_mqtt::endpoint_sp_variant<async_mqtt::role::client, async_mqtt::protocol::mqtt>
        amep1{amep_sp};
    //amep1{async_mqtt::force_move(amep_sp)};

    async_mqtt::endpoint_wp_variant<async_mqtt::role::client, async_mqtt::protocol::mqtt>
        wp1{amep1};
    async_mqtt::endpoint_wp_variant<async_mqtt::role::client, async_mqtt::protocol::mqtt>
        wp3{amep1};
    async_mqtt::endpoint_wp_variant<async_mqtt::role::client, async_mqtt::protocol::mqtt>
        wp2{amep_sp};
    async_mqtt::endpoint_wp_variant<async_mqtt::role::client, async_mqtt::protocol::mqtt>
        wp4{amep_sp};
    auto amep = wp1.lock();

    std::cout << wp1.owner_before(wp3) << std::endl;
    std::cout << wp3.owner_before(wp1) << std::endl;

    std::cout << wp1.owner_before(wp2) << std::endl;
    std::cout << wp2.owner_before(wp1) << std::endl; //*

    std::cout << wp4.owner_before(wp2) << std::endl;
    std::cout << wp2.owner_before(wp4) << std::endl;

    auto str = as::make_strand(ioc);

    amep.stream().next_layer().async_connect(
        endpoint,
        [&]
        (async_mqtt::error_code const& ec) {
            std::cout << "connect: " << ec.message() << std::endl;
            if (ec) return;

            amep.acquire_unique_packet_id(
                [&](auto pid_opt) {
                    if (pid_opt) {
                        auto packet =
                            async_mqtt::v5::publish_packet{
                                *pid_opt,
                                async_mqtt::allocate_buffer("topic1"),
                                async_mqtt::allocate_buffer("payload"),
                                async_mqtt::qos::at_least_once,
                                async_mqtt::properties{
                                    async_mqtt::property::message_expiry_interval{1234}
                                }
                            };

                        std::cout << async_mqtt::hex_dump(packet) << std::endl;;
                        amep.send(
                            async_mqtt::force_move(packet),
                            [&]
                            (async_mqtt::system_error const& ec) mutable {
                                std::cout << "write: " << ec.what() << std::endl;
                                if (ec) return;
                                amep.recv(
                                    as::bind_executor(
                                        str,
                                    [&]
                                    (async_mqtt::packet_variant pv) mutable {
                                        BOOST_ASSERT(str.running_in_this_thread());
                                        if (pv) {
                                            std::cout << async_mqtt::hex_dump(pv) << std::endl;
                                        }
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
