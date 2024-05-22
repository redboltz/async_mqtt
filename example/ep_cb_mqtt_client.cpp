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

struct app {
    app(as::any_io_executor exe, std::string_view host, std::string_view port)
        :exe{am::force_move(exe)}, host{host}, port{port}
    {}

    void start() {
        am::setup_log(am::severity_level::trace);

        amep = am::endpoint<am::role::client, am::protocol::mqtt>::create(
            am::protocol_version::v3_1_1,
            exe
        );

        std::cout << "start" << std::endl;
        // Handshake undlerying layer (Name resolution and TCP handshaking)
        am::async_underlying_handshake(
            amep->next_layer(),
            host,
            port,
            [this]
            (boost::system::error_code ec) {
                handle_handshake(ec);
            }
        );
    }

    void handle_handshake(boost::system::error_code ec) {
        std::cout << "underlying handshake:" << ec.message() << std::endl;
        if (ec) return;
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
            amep->async_send(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                   "ClientIdentifier1",
                    will,   // you can pass std::nullopt if you don't want to set the will message
                    "UserName1",
                    "Password1"
                },
                [this]
                (am::system_error const& se) {
                    handle_send_connect(se);
                }
            );
    }

    void handle_send_connect(am::system_error const& se) {
        if (se) {
            std::cout << "MQTT CONNECT send error:" << se.what() << std::endl;
            return;
        }
        // Recv MQTT CONNACK
        amep->async_recv(
            [this]
            (am::packet_variant pv) {
                handle_recv_connack(am::force_move(pv));
            }
        );
    }

    void handle_recv_connack(am::packet_variant pv) {
        if (pv) {
            pv.visit(
                am::overload {
                    [&](am::v3_1_1::connack_packet const& p) {
                        std::cout
                            << "MQTT CONNACK recv"
                            << " sp:" << p.session_present()
                            << std::endl;
                        // Send MQTT SUBSCRIBE
                        amep->async_send(
                            am::v3_1_1::subscribe_packet{
                                *amep->acquire_unique_packet_id(), // sync version only works thread safe context
                                { {"topic1", am::qos::at_most_once} }
                            },
                            [this]
                            (am::system_error const& se) {
                                handle_send_subscribe(se);
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

    void handle_send_subscribe(am::system_error const& se) {
        if (se) {
            std::cout << "MQTT SUBSCRIBE send error:" << se.what() << std::endl;
            return;
        }
        // Recv MQTT SUBACK
        amep->async_recv(
            [this]
            (am::packet_variant pv) {
                handle_recv_suback(am::force_move(pv));
            }
        );
    }

    void handle_recv_suback(am::packet_variant pv) {
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
                        amep->async_send(
                            am::v3_1_1::publish_packet{
                                *amep->acquire_unique_packet_id(), // sync version only works thread safe context
                                "topic1",
                                "payload1",
                                am::qos::at_least_once
                            },
                            [this]
                            (am::system_error const& se) {
                                handle_send_publish(se);
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

    void handle_send_publish(am::system_error const& se) {
        if (se) {
            std::cout << "MQTT PUBLISH send error:" << se.what() << std::endl;
            return;
        }
        // Recv MQTT PUBACK or (echobacked) PUBLISH
        amep->async_recv(
            [this]
            (am::packet_variant pv) {
                handle_recv_puback_or_publish(am::force_move(pv));
            }
        );
    }

    void handle_recv_puback_or_publish(am::packet_variant pv) {
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
            if (++count < 2) {
                amep->async_recv(
                    [this]
                    (am::packet_variant pv) {
                        handle_recv_puback_or_publish(am::force_move(pv));
                    }
                );
            }
            else {
                std::cout << "close" << std::endl;
                amep->async_close([]{});
            }
        }
        else {
            std::cout
                << "MQTT recv error:"
                << pv.get<am::system_error>().what()
                << std::endl;
            return;
        }
    }

    as::any_io_executor exe;
    std::string_view host;
    std::string_view port;
    std::shared_ptr<am::endpoint<am::role::client, am::protocol::mqtt>> amep;
    int count = 0;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }
    as::io_context ioc;
    app a(ioc.get_executor(), argv[1], argv[2]);
    a.start();
    ioc.run();
}
