// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This example connects to the specified MQTT broker.
// It then publishes to topic1, topic2, and topic3, and receives the publish results.
// Finally, it disconnects from the broker.
//
// Example:
// ./cl_cpp20coro_mqtt_pub mqtt.redboltz.net 1883

#include <iostream>
#include <string>

#include <boost/asio.hpp>

#include <async_mqtt/all.hpp>

namespace as = boost::asio;
namespace am = async_mqtt;

using client_t = am::client<am::protocol_version::v5, am::protocol::mqtt>;

struct app {
    app(as::any_io_executor exe, std::string_view host, std::string_view port)
        : cli_{exe}, host_{host}, port_{port}
    {
        connect();
    }

private:
    void connect() {
        am::async_underlying_handshake(
            cli_.next_layer(),
            host_,
            port_,
            [this](auto&&... args) {
                handle_underlying_handshake(
                    std::forward<std::remove_reference_t<decltype(args)>>(args)...
                );
            }
        );
    }

    void reconnect() {
        tim_.expires_after(std::chrono::seconds{1});
        tim_.async_wait(
            [this](am::error_code const& ec) {
                if (!ec) {
                    connect();
                }
            }
        );
    }

    void handle_underlying_handshake(
        am::error_code ec
    ) {
        std::cout << "underlying_handshake:" << ec.message() << std::endl;
        if (ec) {
            reconnect();
            return;
        }
        cli_.async_start(
            true,                // clean_start
            std::uint16_t(0),    // keep_alive
            "",                  // Client Identifier, empty means generated by the broker
            std::nullopt,        // will
            "UserName1",
            "Password1",
            [this](auto&&... args) {
                handle_start_response(std::forward<decltype(args)>(args)...);
            }
        );
    }

    void handle_start_response(
        am::error_code ec,
        std::optional<client_t::connack_packet> connack_opt
    ) {
        std::cout << "start:" << ec.message() << std::endl;
        if (ec) {
            reconnect();
            return;
        }
        if (connack_opt) {
            std::cout << *connack_opt << std::endl;
        }

        // subscribe
        // MQTT send subscribe and wait suback
        std::vector<am::topic_subopts> sub_entry{
            {"topic1", am::qos::at_most_once},
            {"topic2", am::qos::at_least_once},
            {"topic3", am::qos::exactly_once},
        };
        cli_.async_subscribe(
            *cli_.acquire_unique_packet_id(), // sync version only works thread safe context
            am::force_move(sub_entry),
            [this](auto&&... args) {
                handle_subscribe_response(
                    std::forward<std::remove_reference_t<decltype(args)>>(args)...
                );
            }
        );
    }

    void handle_subscribe_response(
        am::error_code ec,
        std::optional<client_t::suback_packet> suback_opt
    ) {
        std::cout << "subscribe:" << ec.message() << std::endl;
        if (ec) {
            reconnect();
            return;
        }
        if (suback_opt) {
            std::cout << *suback_opt << std::endl;
        }
        cli_.async_recv(
            [this](auto&&... args) {
                handle_recv(
                    std::forward<std::remove_reference_t<decltype(args)>>(args)...
                );
            }
        );
    }

    void handle_recv(
        am::error_code ec,
        am::packet_variant pv
    ) {
        std::cout << "recv:" << ec.message() << std::endl;
        if (ec) {
            reconnect();
            return;
        }
        BOOST_ASSERT(pv);
        std::cout << pv << std::endl;
        // next receive
        cli_.async_recv(
            [this](auto&&... args) {
                handle_recv(
                    std::forward<std::remove_reference_t<decltype(args)>>(args)...
                );
            }
        );
    }

    client_t cli_;
    std::string host_;
    std::string port_;
    as::steady_timer tim_{cli_.get_executor()};
};

int main(int argc, char* argv[]) {
    am::setup_log(
        am::severity_level::warning,
        true // log colored
    );
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }
    as::io_context ioc;
    app a{ioc.get_executor(), argv[1], argv[2]};
    ioc.run();
}
