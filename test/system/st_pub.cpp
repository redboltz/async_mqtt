// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"
#include "broker_runner.hpp"
#include "coro_base.hpp"

#include <async_mqtt/all.hpp>
#include <boost/asio/yield.hpp>

BOOST_AUTO_TEST_SUITE(st_sub)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(v311_pub_to_broker) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep = ep_t::create(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        void proc(
            am::error_code ec,
            am::packet_variant pv,
            am::packet_id_type /*pid*/
        ) override {
            reenter(this) {
                ep().set_auto_pub_response(true);

                yield as::dispatch(
                    as::bind_executor(
                        ep().get_executor(),
                        *this
                    )
                );

                yield am::async_underlying_handshake(

                    ep().next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0,      // keep_alive none
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep().async_recv(*this);
                BOOST_TEST(pv.get_if<am::v3_1_1::connack_packet>());

                // publish QoS0
                yield ep().async_send(
                    am::v3_1_1::publish_packet{
                        "topic1",
                        "payload1",
                        am::qos::at_most_once | am::pub::retain::no | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!ec);

                // publish QoS1
                pid = *ep().acquire_unique_packet_id();
                yield ep().async_send(
                    am::v3_1_1::publish_packet{
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!ec);
                // recv puback
                yield ep().async_recv(*this);
                BOOST_TEST(pv == am::v3_1_1::puback_packet{pid});

                // publish QoS2
                pid = *ep().acquire_unique_packet_id();
                yield ep().async_send(
                    am::v3_1_1::publish_packet{
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::yes
                    },
                    *this
                );
                BOOST_TEST(!ec);
                // recv pubrec
                yield ep().async_recv(*this);
                BOOST_TEST(pv == am::v3_1_1::pubrec_packet{pid});
                // recv pubcomp
                yield ep().async_recv(*this);
                BOOST_TEST(pv == am::v3_1_1::pubcomp_packet{pid});
                yield ep().async_close(*this);
                set_finish();
                guard.reset();
            }
        }
        am::packet_id_type pid;
    };

    tc t{*amep};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_pub_to_broker) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep = ep_t::create(
        am::protocol_version::v5,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        void proc(
            am::error_code ec,
            am::packet_variant pv,
            am::packet_id_type /*pid*/
        ) override {
            reenter(this) {
                ep().set_auto_pub_response(true);

                yield as::dispatch(
                    as::bind_executor(
                        ep().get_executor(),
                        *this
                    )
                );

                yield am::async_underlying_handshake(

                    ep().next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0,      // keep_alive none
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep().async_recv(*this);
                BOOST_TEST(pv.get_if<am::v5::connack_packet>());

                // publish QoS0
                yield ep().async_send(
                    am::v5::publish_packet{
                        0, // packet_id
                        "topic1",
                        "payload1",
                        am::qos::at_most_once | am::pub::retain::no | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!ec);

                // publish QoS1
                pid = *ep().acquire_unique_packet_id();
                yield ep().async_send(
                    am::v5::publish_packet{
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!ec);
                // recv puback
                yield ep().async_recv(*this);
                BOOST_TEST(pv == (am::v5::puback_packet{pid, am::puback_reason_code::no_matching_subscribers}));

                // publish QoS2
                pid = *ep().acquire_unique_packet_id();
                yield ep().async_send(
                    am::v5::publish_packet{
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::yes
                    },
                    *this
                );
                BOOST_TEST(!ec);
                // recv pubrec
                yield ep().async_recv(*this);
                BOOST_CHECK(
                    pv == (am::v5::pubrec_packet{pid, am::pubrec_reason_code::no_matching_subscribers}) ||
                    pv == (am::v5::pubrec_packet{pid})
                );
                // recv pubcomp
                yield ep().async_recv(*this);
                BOOST_TEST(pv == am::v5::pubcomp_packet{pid});
                yield ep().async_close(*this);
                set_finish();
                guard.reset();
            }
        }
        am::packet_id_type pid;
    };

    tc t{*amep};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_from_broker) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t::create(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );
    auto amep_sub = ep_t::create(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            pub,
            sub
        };
        void proc(
            am::error_code ec,
            am::packet_variant pv,
            am::packet_id_type /*pid*/
        ) override {
            reenter(this) {
                ep(pub).set_auto_pub_response(true);
                ep(sub).set_auto_pub_response(true);

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );

                // connect sub
                yield am::async_underlying_handshake(
                    ep(sub).next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(sub).async_send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0, // keep_alive
                        "sub",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub).async_recv(*this);
                pv.visit(
                    am::overload {
                        [&](am::v3_1_1::connack_packet const& p) {
                            BOOST_TEST(!p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );

                yield ep(sub).async_send(
                    am::v3_1_1::subscribe_packet{
                        *ep(sub).acquire_unique_packet_id(),
                        {
                            {"topic1", am::qos::exactly_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub).async_recv(*this);
                BOOST_TEST(pv.get_if<am::v3_1_1::suback_packet>());


                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );

                // connect pub
                yield am::async_underlying_handshake(
                    ep(pub).next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(pub).async_send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0, // keep_alive
                        "pub",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(*this);
                BOOST_TEST(pv.get_if<am::v3_1_1::connack_packet>());

                // publish QoS0
                yield ep(pub).async_send(
                    am::v3_1_1::publish_packet{
                        "topic1",
                        "payload1",
                        am::qos::at_most_once
                    },
                    *this
                );
                BOOST_TEST(!ec);

                // publish QoS1
                yield ep(pub).async_send(
                    am::v3_1_1::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload2",
                        am::qos::at_least_once
                    },
                    *this
                );
                BOOST_TEST(!ec);

                // publish QoS2
                yield ep(pub).async_send(
                    am::v3_1_1::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload3",
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!ec);

                yield ep(pub).async_recv(*this); // recv puback
                yield ep(pub).async_recv(*this); // recv pubrec

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );

                // wait for broker delivers publish packets.
                // to adjust expected recv packet_id
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                yield ep(sub).async_recv(*this);
                BOOST_TEST(
                    pv
                    ==
                    (am::v3_1_1::publish_packet{
                        "topic1",
                        "payload1",
                        am::qos::at_most_once
                    })
                );
                yield ep(sub).async_recv(*this);
                BOOST_TEST(
                    pv
                    ==
                    (am::v3_1_1::publish_packet{
                        1,
                        "topic1",
                        "payload2",
                        am::qos::at_least_once
                    })
                );
                yield ep(sub).async_recv(*this);
                BOOST_TEST(
                    pv
                    ==
                    (am::v3_1_1::publish_packet{
                        2,
                        "topic1",
                        "payload3",
                        am::qos::exactly_once
                    })
                );

                yield ep(sub).async_recv(*this);
                BOOST_TEST(pv.get_if<am::v3_1_1::pubrel_packet>());
                yield ep(sub).async_send(am::v3_1_1::disconnect_packet{}, *this);
                BOOST_TEST(!ec);
                yield ep(sub).async_recv(am::filter::match, {}, *this);
                BOOST_TEST(!pv); // wait and check close by broker

                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );
                yield ep(pub).async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{{*amep_pub, *amep_sub}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_from_broker) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t::create(
        am::protocol_version::v5,
        ioc.get_executor()
    );
    auto amep_sub = ep_t::create(
        am::protocol_version::v5,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            pub,
            sub
        };
        void proc(
            am::error_code ec,
            am::packet_variant pv,
            am::packet_id_type /*pid*/
        ) override {
            reenter(this) {
                ep(pub).set_auto_pub_response(true);
                ep(sub).set_auto_pub_response(true);

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );

                // connect sub
                yield am::async_underlying_handshake(
                    ep(sub).next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(sub).async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        "sub",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub).async_recv(*this);
                pv.visit(
                    am::overload {
                        [&](am::v5::connack_packet const& p) {
                            BOOST_TEST(!p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );

                yield ep(sub).async_send(
                    am::v5::subscribe_packet{
                        *ep(sub).acquire_unique_packet_id(),
                        {
                            {"topic1", am::qos::exactly_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub).async_recv(*this);
                BOOST_TEST(pv.get_if<am::v5::suback_packet>());


                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );

                // connect pub
                yield am::async_underlying_handshake(
                    ep(pub).next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(pub).async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        "pub",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(*this);
                BOOST_TEST(pv.get_if<am::v5::connack_packet>());

                // publish QoS0
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        "topic1",
                        "payload1",
                        am::qos::at_most_once
                    },
                    *this
                );
                BOOST_TEST(!ec);

                // publish QoS1
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload2",
                        am::qos::at_least_once
                    },
                    *this
                );
                BOOST_TEST(!ec);

                // publish QoS2
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload3",
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!ec);

                yield ep(pub).async_recv(*this); // recv puback
                yield ep(pub).async_recv(*this); // recv pubrec

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );

                // wait for broker delivers publish packets.
                // to adjust expected recv packet_id
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                yield ep(sub).async_recv(*this);
                BOOST_TEST(
                    pv
                    ==
                    (am::v5::publish_packet{
                        "topic1",
                        "payload1",
                        am::qos::at_most_once
                    })
                );
                yield ep(sub).async_recv(*this);
                BOOST_TEST(
                    pv
                    ==
                    (am::v5::publish_packet{
                        1,
                        "topic1",
                        "payload2",
                        am::qos::at_least_once
                    })
                );
                yield ep(sub).async_recv(*this);
                BOOST_TEST(
                    pv
                    ==
                    (am::v5::publish_packet{
                        2,
                        "topic1",
                        "payload3",
                        am::qos::exactly_once
                    })
                );

                yield ep(sub).async_recv(*this);
                BOOST_TEST(pv.get_if<am::v5::pubrel_packet>());
                yield ep(sub).async_send(am::v5::disconnect_packet{}, *this);
                BOOST_TEST(!ec);
                yield ep(sub).async_recv(am::filter::match, {}, *this);
                BOOST_TEST(!pv); // wait and check close by broker

                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );
                yield ep(pub).async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{{*amep_pub, *amep_sub}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
