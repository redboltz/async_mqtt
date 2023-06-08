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
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep = ep_t(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        void proc(
            am::optional<am::error_code> ec,
            am::optional<am::system_error> se,
            am::optional<am::packet_variant> pv,
            am::optional<packet_id_t> /*pid*/
        ) override {
            reenter(this) {
                ep().set_auto_pub_response(true);

                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0,      // keep_alive none
                        am::allocate_buffer("cid1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
                BOOST_TEST(pv->get_if<am::v3_1_1::connack_packet>());

                // publish QoS0
                yield ep().send(
                    am::v3_1_1::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_most_once | am::pub::retain::no | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish QoS1
                pid = *ep().acquire_unique_packet_id();
                yield ep().send(
                    am::v3_1_1::publish_packet{
                        pid,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!*se);
                // recv puback
                yield ep().recv(*this);
                BOOST_TEST(*pv == am::v3_1_1::puback_packet{pid});

                // publish QoS2
                pid = *ep().acquire_unique_packet_id();
                yield ep().send(
                    am::v3_1_1::publish_packet{
                        pid,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::yes
                    },
                    *this
                );
                BOOST_TEST(!*se);
                // recv pubrec
                yield ep().recv(*this);
                BOOST_TEST(*pv == am::v3_1_1::pubrec_packet{pid});
                // recv pubcomp
                yield ep().recv(*this);
                BOOST_TEST(*pv == am::v3_1_1::pubcomp_packet{pid});
                yield ep().close(*this);
                yield set_finish();
            }
        }
        ep_t::packet_id_t pid;
    };

    tc t{amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_pub_to_broker) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep = ep_t(
        am::protocol_version::v5,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        void proc(
            am::optional<am::error_code> ec,
            am::optional<am::system_error> se,
            am::optional<am::packet_variant> pv,
            am::optional<packet_id_t> /*pid*/
        ) override {
            reenter(this) {
                ep().set_auto_pub_response(true);

                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0,      // keep_alive none
                        am::allocate_buffer("cid1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
                BOOST_TEST(pv->get_if<am::v5::connack_packet>());

                // publish QoS0
                yield ep().send(
                    am::v5::publish_packet{
                        0, // packet_id
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_most_once | am::pub::retain::no | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish QoS1
                pid = *ep().acquire_unique_packet_id();
                yield ep().send(
                    am::v5::publish_packet{
                        pid,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!*se);
                // recv puback
                yield ep().recv(*this);
                BOOST_TEST(*pv == am::v5::puback_packet{pid});

                // publish QoS2
                pid = *ep().acquire_unique_packet_id();
                yield ep().send(
                    am::v5::publish_packet{
                        pid,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::yes
                    },
                    *this
                );
                BOOST_TEST(!*se);
                // recv pubrec
                yield ep().recv(*this);
                BOOST_TEST(*pv == am::v5::pubrec_packet{pid});
                // recv pubcomp
                yield ep().recv(*this);
                BOOST_TEST(*pv == am::v5::pubcomp_packet{pid});
                yield ep().close(*this);
                yield set_finish();
            }
        }
        ep_t::packet_id_t pid;
    };

    tc t{amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_from_broker) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );
    auto amep_sub = ep_t(
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
            am::optional<am::error_code> ec,
            am::optional<am::system_error> se,
            am::optional<am::packet_variant> pv,
            am::optional<packet_id_t> /*pid*/
        ) override {
            reenter(this) {
                ep(pub).set_auto_pub_response(true);
                ep(sub).set_auto_pub_response(true);
                // connect sub
                yield ep(sub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub).send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0, // keep_alive
                        am::allocate_buffer("sub"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
                pv->visit(
                    am::overload {
                        [&](am::v3_1_1::connack_packet const& p) {
                            BOOST_TEST(!p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );

                yield ep(sub).send(
                    am::v3_1_1::subscribe_packet{
                        *ep(sub).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("topic1"), am::qos::exactly_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
                BOOST_TEST(pv->get_if<am::v3_1_1::suback_packet>());


                // connect pub
                yield ep(pub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(pub).send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0, // keep_alive
                        am::allocate_buffer("pub"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(pub).recv(*this);
                BOOST_TEST(pv->get_if<am::v3_1_1::connack_packet>());

                // publish QoS0
                yield ep(pub).send(
                    am::v3_1_1::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_most_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish QoS1
                yield ep(pub).send(
                    am::v3_1_1::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload2"),
                        am::qos::at_least_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish QoS2
                yield ep(pub).send(
                    am::v3_1_1::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload3"),
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // wait for broker delivers publish packets.
                // to adjust expected recv packet_id
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                yield ep(sub).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v3_1_1::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_most_once
                    })
                );
                yield ep(sub).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v3_1_1::publish_packet{
                        1,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload2"),
                        am::qos::at_least_once
                    })
                );
                yield ep(sub).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v3_1_1::publish_packet{
                        2,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload3"),
                        am::qos::exactly_once
                    })
                );

                yield ep(sub).recv(*this);
                BOOST_TEST(pv->get_if<am::v3_1_1::pubrel_packet>());
                yield ep(sub).send(am::v3_1_1::disconnect_packet{}, *this);
                BOOST_TEST(!*se);
                yield ep(sub).recv(am::filter::match, {}, *this);
                BOOST_TEST(!*pv); // wait and check close by broker

                yield ep(pub).close(*this);
                yield set_finish();
            }
        }
    };

    tc t{{amep_pub, amep_sub}, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_from_broker) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t(
        am::protocol_version::v5,
        ioc.get_executor()
    );
    auto amep_sub = ep_t(
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
            am::optional<am::error_code> ec,
            am::optional<am::system_error> se,
            am::optional<am::packet_variant> pv,
            am::optional<packet_id_t> /*pid*/
        ) override {
            reenter(this) {
                ep(pub).set_auto_pub_response(true);
                ep(sub).set_auto_pub_response(true);
                // connect sub
                yield ep(sub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub).send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("sub"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
                pv->visit(
                    am::overload {
                        [&](am::v5::connack_packet const& p) {
                            BOOST_TEST(!p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );

                yield ep(sub).send(
                    am::v5::subscribe_packet{
                        *ep(sub).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("topic1"), am::qos::exactly_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::suback_packet>());


                // connect pub
                yield ep(pub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(pub).send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("pub"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(pub).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::connack_packet>());

                // publish QoS0
                yield ep(pub).send(
                    am::v5::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_most_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish QoS1
                yield ep(pub).send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload2"),
                        am::qos::at_least_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish QoS2
                yield ep(pub).send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload3"),
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // wait for broker delivers publish packets.
                // to adjust expected recv packet_id
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                yield ep(sub).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_most_once
                    })
                );
                yield ep(sub).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        1,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload2"),
                        am::qos::at_least_once
                    })
                );
                yield ep(sub).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        2,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload3"),
                        am::qos::exactly_once
                    })
                );

                yield ep(sub).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::pubrel_packet>());
                yield ep(sub).send(am::v5::disconnect_packet{}, *this);
                BOOST_TEST(!*se);
                yield ep(sub).recv(am::filter::match, {}, *this);
                BOOST_TEST(!*pv); // wait and check close by broker

                yield ep(pub).close(*this);
                yield set_finish();
            }
        }
    };

    tc t{{amep_pub, amep_sub}, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
