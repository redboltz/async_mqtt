// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <thread>
#include <iostream>

#include <boost/asio.hpp>

#include <async_mqtt/all.hpp>

namespace as = boost::asio;
namespace am = async_mqtt;

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }
    am::setup_log(
        am::severity_level::trace,
        true // log colored
    );

    as::io_context ioc;
    auto amep = am::endpoint<am::role::client, am::protocol::mqtt>{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };

    // async_mqtt thread
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };
    auto on_finish = am::unique_scope_guard(
        [&] {
            guard.reset();
            th.join();
            std::cout << "thread joined" << std::endl;
        }
    );

    try {
        // If CompletionToken has boost::system::error_code as the
        // first parameter and it is not success then exception would
        // be thrown.

        std::string host{argv[1]};
        std::string port{argv[2]};
        // Handshake undlerying layer (Name resolution and TCP handshaking)
        am::async_underlying_handshake(amep.next_layer(), host, port, as::use_future).get();
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
        {
            auto fut = amep.async_send(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "ClientIdentifier1",
                    will,   // you can pass std::nullopt if you don't want to set the will message
                    "UserName1",
                    "Password1"
                },
                as::use_future
            );
            fut.get(); // throw if error_code is not success
        }

        // Recv MQTT CONNACK
        {
            auto fut = amep.async_recv(as::use_future);
            auto pv = fut.get(); // get am::packet_variant, throw if error_code is not success
            if (pv) {
                pv.visit(
                    am::overload {
                        [&](am::v3_1_1::connack_packet const& p) {
                            std::cout
                                << "MQTT CONNACK recv "
                                << "sp:" << p.session_present()
                                << std::endl;
                        },
                        [](auto const&) {}
                    }
                );
                std::cout << am::hex_dump(pv) << std::endl;
            }
        }

        // Send MQTT SUBSCRIBE
        {
            auto fut_id = amep.async_acquire_unique_packet_id(as::use_future);
            auto pid = fut_id.get(); // throw if error_code is not success
            auto fut = amep.async_send(
                am::v3_1_1::subscribe_packet{
                    pid,
                    { {"topic1", am::qos::at_most_once} }
                },
                as::use_future
            );
            fut.get(); // throw if error_code is not success
        }

        // Recv MQTT SUBACK
        {
            auto fut = amep.async_recv(as::use_future);
            auto pv = fut.get(); // get am::packet_variant, throw if error_code is not success
            if (pv) {
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
        }

        // Send MQTT PUBLISH
        {
            auto fut_id = amep.async_acquire_unique_packet_id(as::use_future);
            auto pid = fut_id.get(); // throw if error_code is not success
            auto fut = amep.async_send(
                am::v3_1_1::publish_packet{
                    pid,
                    "topic1",
                    "payload1",
                    am::qos::at_least_once
                },
                as::use_future
            );
            fut.get(); // throw if error_code is not success
        }

        // Recv MQTT PUBLISH and PUBACK (order depends on broker)
        {
            for (std::size_t count = 0; count != 2; ++count) {
                auto fut =  amep.async_recv(as::use_future);
                auto pv = fut.get(); // get am::packet_variant, throw if error_code is not success
                if (pv) {
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
            }
        }
        {
            std::cout << "close" << std::endl;
            auto fut = amep.async_close(as::use_future);
            fut.get();
        }
    }
    catch (std::exception const& e) {
        std::cout << e.what() << std::endl;
    }

}
