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

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }
    am::setup_log(am::severity_level::trace);
    as::io_context ioc;
    as::ip::tcp::socket resolve_sock{ioc};
    as::ip::tcp::resolver res{resolve_sock.get_executor()};
    am::endpoint<am::role::client, am::protocol::mqtt> amep {
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };

    std::cout << "start" << std::endl;
    std::size_t count = 0;

    // Resolve hostname
    res.async_resolve(
        argv[1],
        argv[2],
        [&]
        (boost::system::error_code ec, as::ip::tcp::resolver::results_type eps) {
            std::cout << "async_resolve:" << ec.message() << std::endl;
            if (ec) return;
            // Layer
            // am::stream -> TCP

            // Underlying TCP connect
            as::async_connect(
                amep.next_layer(),
                eps,
                [&]
                (boost::system::error_code ec, as::ip::tcp::endpoint /*unused*/) {
                    std::cout
                        << "TCP connected ec:"
                        << ec.message()
                        << std::endl;
                    if (ec) return;
                    // Send MQTT CONNECT
                    amep.send(
                        am::v3_1_1::connect_packet{
                            true,   // clean_session
                            0x1234, // keep_alive
                            am::allocate_buffer("cid1"),
                            am::nullopt, // will
                            am::nullopt, // username set like am::allocate_buffer("user1"),
                            am::nullopt  // password set like am::allocate_buffer("pass1")
                        },
                        [&]
                        (am::system_error const& se) {
                            if (se) {
                                std::cout << "MQTT CONNECT send error:" << se.what() << std::endl;
                                return;
                            }
                            // Recv MQTT CONNACK
                            amep.recv(
                                [&]
                                (am::packet_variant pv) {
                                    if (pv) {
                                        pv.visit(
                                            am::overload {
                                                [&](am::v3_1_1::connack_packet const& p) {
                                                    std::cout
                                                        << "MQTT CONNACK recv"
                                                        << " sp:" << p.session_present()
                                                        << std::endl;
                                                    // Send MQTT SUBSCRIBE
                                                    amep.send(
                                                        am::v3_1_1::subscribe_packet{
                                                            *amep.acquire_unique_packet_id(),
                                                            { {am::allocate_buffer("topic1"), am::qos::at_most_once} }
                                                        },
                                                        [&]
                                                        (am::system_error const& se) {
                                                            if (se) {
                                                                std::cout << "MQTT SUBSCRIBE send error:" << se.what() << std::endl;
                                                                return;
                                                            }
                                                            // Recv MQTT SUBACK
                                                            amep.recv(
                                                                [&]
                                                                (am::packet_variant pv) {
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
                                                                                    // Send MQTT PUBLISH
                                                                                    amep.send(
                                                                                        am::v3_1_1::publish_packet{
                                                                                            *amep.acquire_unique_packet_id(),
                                                                                            am::allocate_buffer("topic1"),
                                                                                            am::allocate_buffer("payload1"),
                                                                                            am::qos::at_least_once
                                                                                        },
                                                                                        [&]
                                                                                        (am::system_error const& se) {
                                                                                            if (se) {
                                                                                                std::cout << "MQTT PUBLISH send error:" << se.what() << std::endl;
                                                                                                return;
                                                                                            }
                                                                                            // Recv MQTT PUBLISH and PUBACK (order depends on broker)
                                                                                            auto recv_handler = std::make_shared<std::function<void(am::packet_variant pv)>>();
                                                                                            *recv_handler =
                                                                                                [&, recv_handler]
                                                                                                (am::packet_variant pv) {
                                                                                                    if (pv) {
                                                                                                        pv.visit(
                                                                                                            am::overload {
                                                                                                                [&](am::v3_1_1::publish_packet const& p) {
                                                                                                                    std::cout
                                                                                                                        << "MQTT PUBLISH recv"
                                                                                                                        << " pid:" << p.packet_id()
                                                                                                                        << " topic:" << p.topic()
                                                                                                                        << " payload:" << am::to_string(p.payload())
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
                                                                                                        if (++count < 2) {
                                                                                                            amep.recv(*recv_handler);
                                                                                                        }
                                                                                                        else {
                                                                                                            std::cout << "close" << std::endl;
                                                                                                            amep.close([]{});
                                                                                                        }
                                                                                                    }
                                                                                                    else {
                                                                                                        std::cout
                                                                                                            << "MQTT recv error:"
                                                                                                            << pv.get<am::system_error>().what()
                                                                                                            << std::endl;
                                                                                                        return;
                                                                                                    }
                                                                                                };
                                                                                            amep.recv(*recv_handler);
                                                                                        }
                                                                                    );
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
                                                                        return;
                                                                    }
                                                                }
                                                            );
                                                        }
                                                    );
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
                                        return;
                                    }
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
