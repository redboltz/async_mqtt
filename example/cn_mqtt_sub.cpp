// Copyright Takatoshi Kondo 2025
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/asio.hpp>
#include <async_mqtt/protocol/connection.hpp>
#include <async_mqtt/util/setup_log.hpp>

namespace as = boost::asio;
namespace am = async_mqtt;

class mqtt_connection : public am::connection<am::role::client> {
public:
    mqtt_connection(as::ip::tcp::socket socket)
        :
        am::connection<am::role::client>{am::protocol_version::v5},
        socket_{am::force_move(socket)}
    {
        set_auto_pub_response(true);
    }

    as::ip::tcp::socket& socket() {
        return socket_;
    }

private:
    void on_error(am::error_code ec) override final {
        std::cout << "on_error: " << ec.message() << std::endl;
        socket_.close();
        notify_closed();
    }

    void on_close() override final {
        std::cout << "on_close" << std::endl;
        socket_.close();
        notify_closed();
    }

    void on_send(
        am::packet_variant packet,
        std::optional<am::packet_id_type>
        release_packet_id_if_send_error = std::nullopt
    ) override final {
        try {
            as::write(socket_, packet.const_buffer_sequence());
        }
        catch (am::system_error const& se) {
            if (release_packet_id_if_send_error) {
                release_packet_id(*release_packet_id_if_send_error);
            }
            throw;
        }
    }

    void on_packet_id_release(
        am::packet_id_type /*packet_id*/
    ) override final {
    }

    void on_receive(am::packet_variant packet) override final {
        std::cout << "on_receive: " << packet << std::endl;
        packet.visit(
            am::overload {
                [&](am::v5::connack_packet const& p) {
                    if (make_error_code(p.code())) {
                        std::cout << p.code() << std::endl;
                        socket_.close();
                        notify_closed();
                    }
                    else {
                        // subscribe
                        // MQTT send subscribe and wait suback
                        std::vector<am::topic_subopts> sub_entry{
                            {"topic1", am::qos::at_most_once},
                            {"topic2", am::qos::at_least_once},
                            {"topic3", am::qos::exactly_once},
                        };
                        // Send MQTT SUBSCRIBE
                        send(
                            am::v5::subscribe_packet{
                                *acquire_unique_packet_id(),
                                am::force_move(sub_entry)
                            }
                        );
                    }
                },
                [&](am::v5::suback_packet const& p) {
                    std::cout
                        << "MQTT SUBACK recv"
                        << " pid:" << p.packet_id()
                        << " entries:";
                    for (auto const& e : p.entries()) {
                        std::cout << e << " ";
                    }
                    std::cout << std::endl;
                },
                [&](am::v5::publish_packet const& p) {
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
                [&](am::v5::puback_packet const& p) {
                    std::cout
                        << "MQTT PUBACK recv"
                        << " pid:" << p.packet_id()
                        << std::endl;
                },
                [&](am::v5::pubrec_packet const& p) {
                    std::cout
                        << "MQTT PUBREC recv"
                        << " pid:" << p.packet_id()
                        << std::endl;
                },
                [&](am::v5::pubrel_packet const& p) {
                    std::cout
                        << "MQTT PUBREL recv"
                        << " pid:" << p.packet_id()
                        << std::endl;
                },
                [&](am::v5::pubcomp_packet const& p) {
                    std::cout
                        << "MQTT PUBCOMP recv"
                        << " pid:" << p.packet_id()
                        << std::endl;
                },
                [](auto const&) {
                }
            }
        );
    }

    void on_timer_op(
        am::timer_op op,
        am::timer_kind kind,
        std::optional<std::chrono::milliseconds> ms
    ) override final {
        std::cout
            << "timer"
            << " op:" << op
            << " kind:" << kind;
        if (ms) {
            std::cout << " ms:" << ms->count();
        }
        std::cout << std::endl;
    }

private:
    as::ip::tcp::socket socket_;
};

int main(int argc, char* argv[]) {
    am::setup_log(
        am::severity_level::trace,
        true // log colored
    );
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

        // prepare will message if you need.
        am::will will{
            "WillTopic1",
            "WillMessage1",
            am::qos::at_most_once
        };
        mc.send(
            am::v5::connect_packet{
                true,         // clean_start
                0,            // keep_alive
                "",           // Client Identifier, empty means generated by the broker
                std::nullopt, // will
                "UserName1",
                "Password1"
            }
        );

        as::streambuf read_buf;
        std::istream is{&read_buf};
        while (true) {
            auto read_size = mc.socket().read_some(read_buf.prepare(1024));
            read_buf.commit(read_size);
            while (read_buf.size() != 0) mc.recv(is);
        }
    }
    catch (am::system_error const& se) {
        std::cout << "Exception:" << se.what() << std::endl;
        return -1;
    }
}
