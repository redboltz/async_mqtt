// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>

#include <boost/asio.hpp>

#include <async_mqtt/all.hpp>

namespace as = boost::asio;
namespace am = async_mqtt;

as::awaitable<void>
proc(
    auto& amep,
    std::string_view host,
    std::string_view port) {

    auto exe = co_await as::this_coro::executor;
    std::cout << "start" << std::endl;

    try {
        // Handshake undlerying layer (Name resolution and TCP handshaking)
        co_await am::underlying_handshake(amep->next_layer(), host, port, as::use_awaitable);
        std::cout << "Underlying layer handshaked" << std::endl;

        // Send MQTT CONNECT
        if (auto se = co_await amep->send(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1",
                    std::nullopt, // will
                    std::nullopt, // username set like "user1",
                    std::nullopt  // password set like "pass1"
                },
                as::use_awaitable
            )
        ) {
            std::cout << "MQTT CONNECT send error:" << se.what() << std::endl;
            co_return;
        }

        // Recv MQTT CONNACK
        if (am::packet_variant pv = co_await amep->recv(as::use_awaitable)) {
            pv.visit(
                am::overload {
                    [&](am::v3_1_1::connack_packet const& p) {
                        std::cout
                            << "MQTT CONNACK recv"
                            << " sp:" << p.session_present()
                            << std::endl;
                    },
                    [](auto const&) {}
                }
            );
        }
        else {
            std::cout
                << "MQTT CONNACK recv error:"
                << pv.get<am::system_error>().what()
                << std::endl;
            co_return;
        }

        // Send MQTT SUBSCRIBE
        std::vector<am::topic_subopts> sub_entry{
            {"topic1", am::qos::at_most_once}
        };
        if (auto se = co_await amep->send(
                am::v3_1_1::subscribe_packet{
                    *amep->acquire_unique_packet_id(),
                    am::force_move(sub_entry) // sub_entry variable is required to avoid g++ bug
                },
                as::use_awaitable
            )
        ) {
            std::cout << "MQTT SUBSCRIBE send error:" << se.what() << std::endl;
            co_return;
        }
        // Recv MQTT SUBACK
        if (am::packet_variant pv = co_await amep->recv(as::use_awaitable)) {
            pv.visit(
                am::overload {
                    [&](am::v3_1_1::suback_packet const& p) {
                        std::cout
                            << "MQTT SUBACK recv"
                            << " pid:" << p.packet_id()
                            << " entries:";
                        for (auto const& e : p.entries()) {
                            std::cout << e << " ";
                        }
                        std::cout << std::endl;
                    },
                    [](auto const&) {}
                }
            );
        }
        else {
            std::cout
                << "MQTT SUBACK recv error:"
                << pv.get<am::system_error>().what()
                << std::endl;
            co_return;
        }
        // Send MQTT PUBLISH
        if (auto se = co_await amep->send(
                am::v3_1_1::publish_packet{
                    *amep->acquire_unique_packet_id(),
                    "topic1",
                    "payload1",
                    am::qos::at_least_once
                },
                as::use_awaitable
            )
        ) {
            std::cout << "MQTT PUBLISH send error:" << se.what() << std::endl;
            co_return;
        }
        // Recv MQTT PUBLISH and PUBACK (order depends on broker)
        for (std::size_t count = 0; count != 2; ++count) {
            if (am::packet_variant pv = co_await amep->recv(as::use_awaitable)) {
                pv.visit(
                    am::overload {
                        [&](am::v3_1_1::publish_packet const& p) {
                            std::cout
                                << "MQTT PUBLISH recv"
                                << " pid:" << p.packet_id()
                                << " topic:" << p.topic()
                                << " payload:" << p.payload()
                                << " qos:" << p.opts().get_qos()
                                << " retain:" << p.opts().get_retain()
                                << " dup:" << p.opts().get_dup()
                                << std::endl;
                        },
                        [&](am::v3_1_1::puback_packet const& p) {
                            std::cout
                                << "MQTT PUBACK recv"
                                << " pid:" << p.packet_id()
                                << std::endl;
                        },
                        [](auto const&) {}
                    }
                );
            }
            else {
                std::cout
                    << "MQTT recv error:"
                    << pv.get<am::system_error>().what()
                    << std::endl;
                co_return;
            }
        }
        std::cout << "close" << std::endl;
        co_await amep->close(as::use_awaitable);
    }
    catch (boost::system::system_error const& se) {
        std::cout << se.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    am::setup_log(am::severity_level::info);
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }
    as::io_context ioc;
    auto amep = am::endpoint<am::role::client, am::protocol::mqtt>::create(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );
    as::co_spawn(amep->get_executor(), proc(amep, argv[1], argv[2]), as::detached);
    ioc.run();
}
