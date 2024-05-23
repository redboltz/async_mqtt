// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <thread>

#include <boost/asio.hpp>

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/util/packet_variant_operator.hpp>
#include <async_mqtt/packet/packet_helper.hpp>
#include <broker/endpoint_variant.hpp>
#include <broker/broker.hpp>

#include "cpp20coro_stub_socket.hpp"

BOOST_AUTO_TEST_SUITE(ut_broker)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(tc1) {
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            try {
                auto exe = co_await as::this_coro::executor;

                using epv_t = am::endpoint_variant<
                    am::role::server,
                    am::cpp20coro_stub_socket
                >;
                am::broker<
                    epv_t
                > brk{ioc};

                // underlying connect
                auto ep1 =
                    am::endpoint<am::role::server, am::cpp20coro_stub_socket>::create(
                        am::protocol_version::undetermined, // for broker decided by connect packet
                        // for cpp20coro_stub_socket args
                        am::protocol_version::v3_1_1,       // for emulated client so should already be known
                        exe
                    );
                brk.handle_accept(epv_t{ep1});

                // underlying connect
                auto ep2 =
                    am::endpoint<am::role::server, am::cpp20coro_stub_socket>::create(
                        am::protocol_version::v5,
                        //am::protocol_version::undetermined,
                        // for cpp20coro_stub_socket args
                        am::protocol_version::v5,
                        exe
                    );
                brk.handle_accept(epv_t{ep2});

                {
                    // connect ep1
                    auto c2b_packet = am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0x1234, // keep_alive
                        "cid1"
                    };
                    co_await ep1->next_layer().emulate_recv(
                        am::force_move(c2b_packet),
                        as::deferred
                    );

                    // connack ep1
                    auto exp_packet = am::v3_1_1::connack_packet{
                        false,   // session_present
                        am::connect_return_code::accepted
                    };
                    auto b2c_packet = co_await ep1->next_layer().wait_response(as::deferred);
                    BOOST_TEST(b2c_packet == exp_packet);
                }
                {
                    // subscribe ep1
                    auto c2b_packet = am::v3_1_1::subscribe_packet{
                        0x1234,         // packet_id
                        {
                            {"topic1", am::qos::at_most_once},
                            {"topic2", am::qos::exactly_once},
                        }
                    };
                    co_await ep1->next_layer().emulate_recv(
                        am::force_move(c2b_packet),
                        as::deferred
                    );

                    // suback ep1
                    auto exp_packet = am::v3_1_1::suback_packet{
                        0x1234,         // packet_id
                        {
                            am::suback_return_code::success_maximum_qos_0,
                            am::suback_return_code::success_maximum_qos_2,
                        }
                    };
                    auto b2c_packet = co_await ep1->next_layer().wait_response(as::deferred);
                    BOOST_TEST(b2c_packet == exp_packet);
                }
                {
                    // connect ep2
                    auto c2b_packet = am::v5::connect_packet{
                        true,   // clean_start
                        0x1234, // keep_alive
                        "cid2"
                    };
                    co_await ep2->next_layer().emulate_recv(
                        am::force_move(c2b_packet),
                        as::deferred
                    );

                    // connack ep2
                    auto exp_packet = am::v5::connack_packet{
                        false,   // session_present
                        am::connect_reason_code::success,
                        {
                            am::property::topic_alias_maximum{am::topic_alias_max},
                            am::property::receive_maximum{am::receive_maximum_max},
                        }
                    };
                    auto b2c_packet = co_await ep2->next_layer().wait_response(as::deferred);
                    BOOST_TEST(b2c_packet == exp_packet);
                }
                {
                    // subscribe ep2
                    auto c2b_packet = am::v5::subscribe_packet{
                        0x1234,         // packet_id
                        {
                            {"topic1", am::qos::at_most_once}
                        }
                    };
                    co_await ep2->next_layer().emulate_recv(
                        am::force_move(c2b_packet),
                        as::deferred
                    );

                    // suback ep2
                    auto exp_packet = am::v5::suback_packet{
                        0x1234,         // packet_id
                        {
                            am::suback_reason_code::granted_qos_0
                        }
                    };
                    auto b2c_packet = co_await ep2->next_layer().wait_response(as::deferred);
                    BOOST_TEST(b2c_packet == exp_packet);
                }
                {
                    // publish ep2
                    auto c2b_packet = am::v5::publish_packet{
                        0x1234, // packet_id
                        "topic1",
                        "payload1",
                        am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::no,
                        am::properties{
                            am::property::content_type("json")
                        }
                    };
                    co_await ep2->next_layer().emulate_recv(
                        am::force_move(c2b_packet),
                        as::deferred
                    );
                }
                {
                    // recv ep1
                    auto exp_packet = am::v3_1_1::publish_packet{
                        "topic1",
                        "payload1",
                        am::qos::at_most_once | am::pub::retain::no | am::pub::dup::no
                    };
                    auto b2c_packet = co_await ep1->next_layer().wait_response(as::deferred);
                    BOOST_TEST(b2c_packet == exp_packet);
                }
                {
                    std::set<am::packet_variant> exp_packets {
                        am::v5::publish_packet{
                            "topic1",
                            "payload1",
                            am::qos::at_most_once | am::pub::retain::no | am::pub::dup::no,
                            am::properties{
                                am::property::content_type("json")
                            }
                        },
                        am::v5::puback_packet{
                            0x1234
                        },
                    };
                    // recv ep2
                    auto b2c_packet1 = co_await ep2->next_layer().wait_response(as::deferred);
                    BOOST_ASSERT(exp_packets.erase(b2c_packet1) == 1);
                    auto b2c_packet2 = co_await ep2->next_layer().wait_response(as::deferred);
                    BOOST_ASSERT(exp_packets.erase(b2c_packet2) == 1);
                }
                {
                    // close ep1
                    co_await ep1->next_layer().emulate_close(as::deferred);
                    // closed ep1
                    auto [ec, b2c_close] = co_await ep1->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::errc::connection_reset);
                }
                {
                    // close ep2
                    co_await ep2->next_layer().emulate_close(as::deferred);
                    // closed ep2
                    auto [ec, b2c_close] = co_await ep2->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::errc::connection_reset);
                }
            }
            catch (...) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}


BOOST_AUTO_TEST_SUITE_END()
