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

using client_t = am::client<am::protocol_version::v5, am::protocol::mqtt>;

as::awaitable<void>
proc(
    client_t& amcl,
    std::string_view host,
    std::string_view port) {

    auto exe = co_await as::this_coro::executor;

    std::cout << "start" << std::endl;

    try {
        // all underlying layer handshaking
        // (Resolve hostname, TCP handshake)
        co_await am::underlying_handshake(amcl.next_layer(), host, port, as::use_awaitable);

        std::cout << "mqtt undlerlying handshaked" << std::endl;

        am::will will{
            "WillTopic1",
            "WillMessage1",
            am::qos::at_most_once,
            { // properties
                am::property::user_property{"key1", "val1"},
                am::property::content_type{"text"},
            }
        };

        // MQTT connect and receive loop start
        auto connack_opt = co_await amcl.start(
            am::v5::connect_packet{
                true,   // clean_session
                0x1234, // keep_alive
                "ClientIdentifier1",
                will,
                "UserName1",
                "Password1"
            },
            as::use_awaitable
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
        auto suback_opt = co_await amcl.subscribe(
            am::v5::subscribe_packet{
                *amcl.acquire_unique_packet_id(), // sync version only works single threaded or in strand
                am::force_move(sub_entry) // sub_entry variable is required to avoid g++ bug
            },
            as::use_awaitable
        );
        if (suback_opt) {
            std::cout << *suback_opt << std::endl;
        }
        auto print_pubres =
            [](client_t::pubres_t const& pubres) {
                if (pubres.puback_opt) {
                    std::cout << *pubres.puback_opt << std::endl;
                }
                if (pubres.pubrec_opt) {
                    std::cout << *pubres.pubrec_opt << std::endl;
                }
                if (pubres.pubcomp_opt) {
                    std::cout << *pubres.pubcomp_opt << std::endl;
                }
            };

        // publish
        // MQTT publish QoS0 and wait response (socket write complete)
        auto pubres0 = co_await amcl.publish(
            am::v5::publish_packet{
                "topic1",
                "payload1",
                am::qos::at_most_once
            },
            as::use_awaitable
        );
        print_pubres(pubres0);

        // MQTT publish QoS1 and wait response (puback receive)
        auto pid_pub1_opt = co_await amcl.acquire_unique_packet_id(as::use_awaitable); // async version
        auto pubres1 = co_await amcl.publish(
            am::v5::publish_packet{
                *pid_pub1_opt,
                "topic2",
                "payload2",
                am::qos::at_least_once
            },
            as::use_awaitable
        );
        print_pubres(pubres1);

        // recv (coroutine)
        for (int i = 0; i != 2; ++i) {
            auto [publish_opt, disconnect_opt] = co_await amcl.recv(as::use_awaitable);
            if (publish_opt) {
                std::cout << *publish_opt << std::endl;
                std::cout << "topic   : " << publish_opt->topic() << std::endl;
                std::cout << "payload : " << publish_opt->payload() << std::endl;
            }
            if (disconnect_opt) {
                std::cout << *disconnect_opt << std::endl;
            }
        }
        // recv (callback) before sending
        amcl.recv(
            [] (auto ec, auto publish_opt, auto disconnect_opt) {
                std::cout << ec.message() << std::endl;
                if (publish_opt) {
                    std::cout << *publish_opt << std::endl;
                    std::cout << "topic   : " << publish_opt->topic() << std::endl;
                    std::cout << "payload : " << publish_opt->payload() << std::endl;
                }
                if (disconnect_opt) {
                    std::cout << *disconnect_opt << std::endl;
                }
            }
        );

        // MQTT publish QoS2 and wait response (pubrec, pubcomp receive)
        auto pid_pub2 = co_await amcl.acquire_unique_packet_id_wait_until(as::use_awaitable); // async version
        auto pubres2 = co_await amcl.publish(
            am::v5::publish_packet{
                pid_pub2,
                "topic3",
                "payload3",
                am::qos::exactly_once
            },
            as::use_awaitable
        );
        print_pubres(pubres2);

        // MQTT send unsubscribe and wait unsuback
        std::vector<am::topic_sharename> unsub_entry{
            {"topic1"},
            {"topic2"},
            {"topic3"},
        };

        auto unsuback_opt = co_await amcl.unsubscribe(
            am::v5::unsubscribe_packet{
                *amcl.acquire_unique_packet_id(), // sync version only works single threaded or in strand
                am::force_move(unsub_entry) // unsub_entry variable is required to avoid g++ bug
            },
            as::use_awaitable
        );
        if (unsuback_opt) {
            std::cout << *unsuback_opt << std::endl;
        }

        // disconnect
        co_await amcl.disconnect(
            am::v5::disconnect_packet{
                am::disconnect_reason_code::disconnect_with_will_message
            },
            as::use_awaitable
        );
        std::cout << "finished" << std::endl;
    }
    catch (boost::system::system_error const& se) {
        std::cout << se.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    am::setup_log(am::severity_level::warning);
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }
    as::io_context ioc;
    client_t amcl{ioc.get_executor()};
    as::co_spawn(amcl.get_executor(), proc(amcl, argv[1], argv[2]), as::detached);
    ioc.run();
}
