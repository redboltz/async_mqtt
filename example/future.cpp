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
#include <async_mqtt/util/hex_dump.hpp>

namespace as = boost::asio;

int main() {
    as::io_context ioc;
    as::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    as::ip::tcp::endpoint endpoint{address, 1883};
    async_mqtt::endpoint<async_mqtt::role::client, async_mqtt::protocol::mqtt> amep{
        async_mqtt::protocol_version::v3_1_1,
        ioc
    };

    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    try {
        auto f_con = amep.stream().next_layer().async_connect(
            endpoint,
            as::use_future
        );
        f_con.get();
        std::cout << "connected" << std::endl;

        auto f_acpid = amep.acquire_unique_packet_id(as::use_future);
        auto pid_opt = f_acpid.get();
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
            auto f_send = amep.send(
                async_mqtt::force_move(packet),
                as::use_future);
            auto se_send = f_send.get();
            std::cout << "write: " << se_send.what() << std::endl;
            if (!se_send) {
                auto f_recv = amep.recv(as::use_future);
                auto pv = f_recv.get();
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
        }
    }
    catch (std::exception const& e) {
        std::cout << e.what() << std::endl;
    }
    guard.reset();

    th.join();
}
