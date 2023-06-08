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
    am::setup_log(am::severity_level::trace);

    as::io_context ioc;
    as::ip::tcp::socket resolve_sock{ioc};
    as::ip::tcp::resolver res{resolve_sock.get_executor()};
    am::endpoint<am::role::client, am::protocol::mqtt> amep {
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

        // Resolve hostname
        std::string host{argv[1]};
        std::string port{argv[2]};
        auto f_res = res.async_resolve(host, port, as::use_future);
        auto eps = f_res.get();

        auto f_con = as::async_connect(
            amep.next_layer(),
            eps,
            as::use_future
        );
        f_con.get();
        std::cout << "TCP connected" << std::endl;

        // Send MQTT CONNECT
        {
            auto fut = amep.send(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    am::allocate_buffer("cid1"),
                    am::nullopt, // will
                    am::nullopt, // username set like am::allocate_buffer("user1"),
                    am::nullopt  // password set like am::allocate_buffer("pass1")
                },
                as::use_future
            );
            auto se = fut.get(); // get am::system_error
            if (se) {
                std::cout << "MQTT CONNECT send error:" << se.what() << std::endl;
                return -1;
            }
        }

        // Recv MQTT CONNACK
        {
            auto fut = amep.recv(as::use_future);
            auto pv = fut.get(); // get am::packet_variant
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
            else {
                std::cout
                    << "MQTT CONNACK recv error:"
                    << pv.get<am::system_error>().what()
                    << std::endl;
                return -1;
            }
        }

        // Send MQTT SUBSCRIBE
        {
            auto fut_id = amep.acquire_unique_packet_id(as::use_future);
            auto pid = fut_id.get();
            auto fut = amep.send(
                am::v3_1_1::subscribe_packet{
                    *pid,
                    { {am::allocate_buffer("topic1"), am::qos::at_most_once} }
                },
                as::use_future
            );
            auto se = fut.get();
            if (se) {
                std::cout << "MQTT SUBSCRIBE send error:" << se.what() << std::endl;
                return -1;
            }
        }

        // Recv MQTT SUBACK
        {
            auto fut = amep.recv(as::use_future);
            auto pv = fut.get();
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
            else {
                std::cout
                    << "MQTT SUBACK recv error:"
                    << pv.get<am::system_error>().what()
                    << std::endl;
                return -1;
            }
        }

        // Send MQTT PUBLISH
        {
            auto fut_id = amep.acquire_unique_packet_id(as::use_future);
            auto pid = fut_id.get();
            auto fut = amep.send(
                am::v3_1_1::publish_packet{
                    *pid,
                    am::allocate_buffer("topic1"),
                    am::allocate_buffer("payload1"),
                    am::qos::at_least_once
                },
                as::use_future
            );
            auto se = fut.get();
            if (se) {
                std::cout << "MQTT PUBLISH send error:" << se.what() << std::endl;
                return -1;
            }
        }

        // Recv MQTT PUBLISH and PUBACK (order depends on broker)
        {
            for (std::size_t count = 0; count != 2; ++count) {
                auto fut =  amep.recv(as::use_future);
                auto pv = fut.get();
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
                }
                else {
                    std::cout
                        << "MQTT recv error:"
                        << pv.get<am::system_error>().what()
                        << std::endl;
                    return -1;
                }
            }
        }
        {
            std::cout << "close" << std::endl;
            auto fut = amep.close(as::use_future);
            fut.get();
        }
    }
    catch (std::exception const& e) {
        std::cout << e.what() << std::endl;
    }

}
