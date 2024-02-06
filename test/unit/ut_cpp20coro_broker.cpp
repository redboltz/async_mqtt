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
#include <async_mqtt/util/hex_dump.hpp>
#include <async_mqtt/broker/endpoint_variant.hpp>
#include <async_mqtt/broker/broker.hpp>

#include "cpp20coro_stub_socket.hpp"

BOOST_AUTO_TEST_SUITE(ut_broker)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(tc1) {
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [](
            auto& ioc
        ) -> as::awaitable<void> {
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
                    ioc.get_executor()
                );
            brk.handle_accept(epv_t{ep1});

            // underlying connect
            auto ep2 =
                am::endpoint<am::role::server, am::cpp20coro_stub_socket>::create(
                    am::protocol_version::v5,
                    //am::protocol_version::undetermined,
                    // for cpp20coro_stub_socket args
                    am::protocol_version::v5,
                    ioc.get_executor()
                );
            brk.handle_accept(epv_t{ep2});

            {
                // connect ep1
                auto connect = am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    am::allocate_buffer("cid1")
                };
                co_await ep1->next_layer().emulate_recv(connect, as::deferred);

                // connack ep1
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                auto r_connack = co_await ep1->next_layer().wait_response(as::deferred);
                BOOST_TEST(connack == r_connack);

                // subscribe ep1
                auto subscribe = am::v3_1_1::subscribe_packet{
                    0x1234,         // packet_id
                    {
                        {am::allocate_buffer("topic1"), am::qos::at_most_once},
                        {am::allocate_buffer("topic2"), am::qos::exactly_once},
                    }
                };
                co_await ep1->next_layer().emulate_recv(subscribe, as::deferred);

                // suback ep1
                auto suback = am::v3_1_1::suback_packet{
                    0x1234,         // packet_id
                    {
                        am::suback_return_code::success_maximum_qos_0,
                        am::suback_return_code::success_maximum_qos_2,
                    }
                };
                auto r_suback = co_await ep1->next_layer().wait_response(as::deferred);
                BOOST_TEST(suback == r_suback);
            }
            {
                // connect ep2
                auto connect = am::v5::connect_packet{
                    true,   // clean_start
                    0x1234, // keep_alive
                    am::allocate_buffer("cid2")
                };
                co_await ep2->next_layer().emulate_recv(connect, as::deferred);

                // connack ep2
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success,
                    {
                        am::property::topic_alias_maximum{am::topic_alias_max},
                        am::property::receive_maximum{am::receive_maximum_max},
                    }
                };
                auto r_connack = co_await ep2->next_layer().wait_response(as::deferred);
                BOOST_TEST(connack == r_connack);

                // subscribe ep2
                auto subscribe = am::v5::subscribe_packet{
                    0x1234,         // packet_id
                    {
                        {am::allocate_buffer("topic1"), am::qos::at_most_once}
                    }
                };
                co_await ep2->next_layer().emulate_recv(subscribe, as::deferred);

                // suback ep2
                auto suback = am::v5::suback_packet{
                    0x1234,         // packet_id
                    {
                        am::suback_reason_code::granted_qos_0
                    }
                };
                auto r_suback = co_await ep2->next_layer().wait_response(as::deferred);
                BOOST_TEST(suback == r_suback);
            }
            {
                // publish ep2
                auto publish = am::v5::publish_packet{
                    0x1234, // packet_id
                    am::allocate_buffer("topic1"),
                    am::allocate_buffer("payload1"),
                    am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::no,
                    am::properties{
                        am::property::content_type("json")
                    }
                };
                co_await ep2->next_layer().emulate_recv(publish, as::deferred);
            }
            {
                // recv ep1
                auto publish = am::v3_1_1::publish_packet{
                    am::allocate_buffer("topic1"),
                    am::allocate_buffer("payload1"),
                    am::qos::at_most_once | am::pub::retain::no | am::pub::dup::no
                };
                auto r_publish = co_await ep1->next_layer().wait_response(as::deferred);
                BOOST_TEST(publish == r_publish);
            }
            {
                std::set<am::packet_variant> packets {
                    am::v5::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
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
                auto r_packet1 = co_await ep2->next_layer().wait_response(as::deferred);
                BOOST_ASSERT(packets.erase(r_packet1) == 1);
                auto r_packet2 = co_await ep2->next_layer().wait_response(as::deferred);
                BOOST_ASSERT(packets.erase(r_packet2) == 1);
            }
            {
                // close ep1
                co_await ep1->next_layer().emulate_close(as::deferred);
                // closed ep1
                auto r_close = co_await ep1->next_layer().wait_response(as::deferred);
                BOOST_TEST(am::is_close(r_close));
            }
            {
                // close ep2
                co_await ep2->next_layer().emulate_close(as::deferred);
                // closed ep2
                auto r_close = co_await ep2->next_layer().wait_response(as::deferred);
                BOOST_TEST(am::is_close(r_close));
            }

            co_return;
        }(ioc),
        as::detached
    );
    ioc.run();
}


BOOST_AUTO_TEST_SUITE_END()
