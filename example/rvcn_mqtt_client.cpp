// Copyright Takatoshi Kondo 2025
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/asio.hpp>
#include <async_mqtt/protocol/rv_connection.hpp>

namespace as = boost::asio;
namespace am = async_mqtt;

class mqtt_connection {
public:
    mqtt_connection(as::ip::tcp::socket socket)
        :socket_{am::force_move(socket)}
    {}

    as::ip::tcp::socket& socket() {
        return socket_;
    }

    void start() {
        // prepare will message if you need.
        am::will will{
            "WillTopic1",
            "WillMessage1",
            am::qos::at_most_once
        };
        {
            auto events = mc_.send(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0,      // keep_alive (because no on_timer_op implementation)
                    "ClientIdentifier1",
                    will,   // you can pass std::nullopt if you don't want to set the will message
                    "UserName1",
                    "Password1"
                }
            );
            handle_events(events);
        }
    }

    void read() {
        as::streambuf read_buf;
        while (true) {
            auto read_size = socket_.read_some(read_buf.prepare(1024));
            read_buf.commit(read_size);
            std::istream is{&read_buf};
            auto events = mc_.recv(is);
            handle_events(events);
        }
    }

private:
    void handle_events(std::vector<am::event_variant> const& events) {
        for (auto const& event : events) {
            handle_event(event);
        }
    }
    void handle_event(am::event_variant const& ev) {
        std::visit(
            am::overload {
                [&](am::error_code const& ec) {
                    std::cout << "error: " << ec.message() << std::endl;
                    socket_.close();
                    mc_.notify_closed();
                },
                [&](am::event::send const& ev) {
                    try {
                        as::write(socket_, ev.get().const_buffer_sequence());
                    }
                    catch (am::system_error const& se) {
                        if (auto pid_opt = ev.get_release_packet_id_if_send_error()) {
                            mc_.release_packet_id(*pid_opt);
                        }
                        throw;
                    }
                },
                [&](am::event::packet_id_released const& /*ev*/) {
                },
                [&](am::event::packet_received const& ev) {
                    auto packet = ev.get();
                    std::cout << "on_receive: " << packet << std::endl;
                    packet.visit(
                        am::overload {
                            [&](am::v3_1_1::connack_packet const& p) {
                                if (make_error_code(p.code())) {
                                    std::cout << p.code() << std::endl;
                                    socket_.close();
                                    mc_.notify_closed();
                                }
                                else {
                                    // Send MQTT SUBSCRIBE
                                    std::vector<am::topic_subopts> sub_entry{
                                        {"topic1", am::qos::at_most_once}
                                    };
                                    auto events = mc_.send(
                                        am::v3_1_1::subscribe_packet{
                                            *mc_.acquire_unique_packet_id(),
                                            am::force_move(sub_entry)
                                        }
                                    );
                                    handle_events(events);
                                }
                            },
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
                                auto events = mc_.send(
                                    am::v3_1_1::publish_packet{
                                        *mc_.acquire_unique_packet_id(),
                                        "topic1",
                                        "payload1",
                                        am::qos::at_least_once
                                    }
                                );
                                handle_events(events);
                            },
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
                                publish_received_ = true;
                                if (puback_received_) {
                                    socket_.close();
                                    mc_.notify_closed();
                                }
                            },
                            [&](am::v3_1_1::puback_packet const& p) {
                                std::cout
                                    << "MQTT PUBACK recv"
                                    << " pid:" << p.packet_id()
                                    << std::endl;
                                puback_received_ = true;
                                if (publish_received_) {
                                    socket_.close();
                                    mc_.notify_closed();
                                }
                            },
                            [](auto const&) {
                            }
                        }
                    );
                },
                [&](am::event::timer const& /*ev*/) {
                },
                [&](am::event::close const& /*ev*/) {
                    std::cout << "close" << std::endl;
                    socket_.close();
                    mc_.notify_closed();
                }
            },
            ev
        );
    }

private:
    as::ip::tcp::socket socket_;
    am::rv_connection<am::role::client> mc_{am::protocol_version::v3_1_1};
    bool publish_received_ = false;
    bool puback_received_ = false;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }
    as::io_context ioc;

    try {
        as::ip::tcp::resolver res{ioc.get_executor()};
        auto eps = res.resolve(argv[1], argv[2]);
        as::ip::tcp::socket socket{ioc.get_executor()};
        as::connect(socket, eps);
        mqtt_connection mc{am::force_move(socket)};
        mc.start();
        mc.read();
    }
    catch (am::system_error const& se) {
        std::cout << "Exception:" << se.what() << std::endl;
        return -1;
    }
}
