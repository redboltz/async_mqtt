// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This example connects to the specified MQTT broker.
// It then subscribes to topic1, topic2, and topic3.
// Finally, it waits for PUBLISH packets in an infinite loop until disconnected by the broker.
//
// Example:
// ./cl_cpp20coro_mqtt_sub mqtt.redboltz.net 1883

#include <iostream>
#include <string>

#include <boost/asio.hpp>

#include <async_mqtt/all.hpp>

namespace as = boost::asio;
namespace am = async_mqtt;

// Use as::use_awaitable as the default completion token for am::client
using awaitable_client =
    boost::asio::use_awaitable_t<>::as_default_on_t<
        am::client<am::protocol_version::v3_1_1, am::protocol::mqtt>
    >;

as::awaitable<void>
proc(
    awaitable_client& amcl,
    std::string_view host,
    std::string_view port) {

    auto exe = co_await as::this_coro::executor;

    std::cout << "start" << std::endl;

    try {
        // all underlying layer handshaking
        // (Resolve hostname, TCP handshake)
        co_await am::async_underlying_handshake(amcl.next_layer(), host, port);
        std::cout << "mqtt undlerlying handshaked" << std::endl;

        // MQTT connect and receive loop start
        auto connack_opt = co_await amcl.async_start(
            true,                // clean_start
            std::uint16_t(0),    // keep_alive
            "ClientIdentifierSub1",
            std::nullopt,        // will
            "UserName1",
            "Password1"
        );
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
        auto suback_opt = co_await amcl.async_subscribe(
            *amcl.acquire_unique_packet_id(), // sync version only works single threaded or in strand
            am::force_move(sub_entry)
        );
        if (suback_opt) {
            std::cout << *suback_opt << std::endl;
        }

        // recv (coroutine)
        while (true) {
            auto [publish_opt, disconnect_opt] = co_await amcl.async_recv(as::use_awaitable);
            if (publish_opt) {
                std::cout << *publish_opt << std::endl;
                std::cout << "topic   : " << publish_opt->topic() << std::endl;
                std::cout << "payload : " << publish_opt->payload() << std::endl;
            }
            if (disconnect_opt) {
                std::cout << *disconnect_opt << std::endl;
                break;
            }
        }
        std::cout << "finished" << std::endl;
    }
    catch (boost::system::system_error const& se) {
        std::cout << se.code().message() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    am::setup_log(am::severity_level::warning);
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }
    as::io_context ioc;
    awaitable_client amcl{ioc.get_executor()};
    as::co_spawn(amcl.get_executor(), proc(amcl, argv[1], argv[2]), as::detached);
    ioc.run();
}
