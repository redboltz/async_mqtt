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
        co_await amep.async_underlying_handshake(host, port, as::use_awaitable);
        std::cout << "Underlying layer handshaked" << std::endl;

        // prepare will message if you need.
        am::will will{
            "WillTopic1",
            "WillMessage1",
            am::qos::at_most_once,
            { // properties
                am::property::user_property{"key1", "val1"},
                am::property::content_type{"text"},
            }
        };

        // Send MQTT CONNECT
        co_await amep.async_send(
            am::v3_1_1::connect_packet{
                true,   // clean_session
                0x1234, // keep_alive
                "ClientIdentifier1",
                will,   // you can pass std::nullopt if you don't want to set the will message
                "UserName1",
                "Password1"
            },
            as::use_awaitable
        );

        // Recv MQTT CONNACK
        if (am::packet_variant pv_opt = co_await amep.async_recv(as::use_awaitable)) {
            pv_opt->visit(
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

        // Send MQTT SUBSCRIBE
        std::vector<am::topic_subopts> sub_entry{
            {"topic1", am::qos::at_most_once}
        };
        co_await amep.async_send(
            am::v3_1_1::subscribe_packet{
                *amep.acquire_unique_packet_id(), // sync version only works thread safe context
                am::force_move(sub_entry)
            },
            as::use_awaitable
        );

        // Recv MQTT SUBACK
        if (am::packet_variant pv_opt = co_await amep.async_recv(as::use_awaitable)) {
            pv_opt->visit(
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

        // Send MQTT PUBLISH
        co_await amep.async_send(
            am::v3_1_1::publish_packet{
                *amep.acquire_unique_packet_id(), // sync version only works thread safe context
                "topic1",
                "payload1",
                am::qos::at_least_once
            },
            as::use_awaitable
        );

        // Recv MQTT PUBLISH and PUBACK (order depends on broker)
        for (std::size_t count = 0; count != 2; ++count) {
            if (am::packet_variant pv_opt = co_await amep.async_recv(as::use_awaitable)) {
                pv_opt->visit(
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
        }
        std::cout << "close" << std::endl;
        co_await amep.async_close(as::use_awaitable);
    }
    catch (boost::system::system_error const& se) {
        std::cout << se.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    am::setup_log(
        am::severity_level::info,
        true // log colored
    );
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }
    as::io_context ioc;
    auto amep = am::endpoint<am::role::client, am::protocol::mqtt>{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };
    as::co_spawn(amep.get_executor(), proc(amep, argv[1], argv[2]), as::detached);
    ioc.run();
}
