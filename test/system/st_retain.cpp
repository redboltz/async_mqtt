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

BOOST_AUTO_TEST_SUITE(st_retain)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(v5_mei_none) {
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
                        am::qos::at_most_once | am::pub::retain::yes
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
                        am::qos::at_least_once | am::pub::retain::yes
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
                        am::qos::exactly_once | am::pub::retain::yes
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(pub).recv(am::filter::match, {am::control_packet_type::pubrec}, *this);

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

                yield ep(sub).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        1,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload3"),
                        am::qos::exactly_once | am::pub::retain::yes
                    })
                );

                yield ep(sub).close(*this);
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

BOOST_AUTO_TEST_CASE(v5_mei_no_exp) {
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
                        am::qos::at_most_once | am::pub::retain::yes
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
                        am::qos::at_least_once | am::pub::retain::yes
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
                        am::qos::exactly_once | am::pub::retain::yes,
                        {am::property::message_expiry_interval{1}}
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(pub).recv(am::filter::match, {am::control_packet_type::pubrec}, *this);

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

                yield ep(sub).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        1,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload3"),
                        am::qos::exactly_once | am::pub::retain::yes,
                        {am::property::message_expiry_interval{0}}
                    })
                );

                yield ep(sub).close(*this);
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

BOOST_AUTO_TEST_CASE(v5_mei_exp) {
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
                        am::qos::at_most_once | am::pub::retain::yes
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
                        am::qos::at_least_once | am::pub::retain::yes
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
                        am::qos::exactly_once | am::pub::retain::yes,
                        {am::property::message_expiry_interval{1}}
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(pub).recv(am::filter::match, {am::control_packet_type::pubrec}, *this);

                // wait for broker retain message will expire
                std::this_thread::sleep_for(std::chrono::seconds(2));

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

                // not recv publish (retain)
                yield {
                    ep(sub).recv(*this); // 1st async call
                    auto tim = std::make_shared<as::steady_timer>(ep(sub).strand());
                    tim->expires_after(std::chrono::seconds(1));
                    tim->async_wait(  // 2nd async call
                        as::consign(
                            *this,
                            tim
                        )
                    );
                }
                yield { // this yield is called twice
                    if (ec) {
                        // 1st
                        BOOST_TEST(!pv); // not recv
                        BOOST_TEST(*ec == am::errc::success); // timeout
                        ep(sub).close(*this);
                    }
                    else {
                        // 2nd
                        BOOST_TEST(!*pv); // recv error due to close
                    }
                }
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

BOOST_AUTO_TEST_CASE(v5_clear) {
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
                        am::qos::at_most_once | am::pub::retain::yes
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish QoS2
                yield ep(pub).send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer(""),
                        am::qos::exactly_once | am::pub::retain::yes
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(pub).recv(am::filter::match, {am::control_packet_type::pubrec}, *this);

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

                // not recv publish (retain)
                yield {
                    ep(sub).recv(*this); // 1st async call
                    auto tim = std::make_shared<as::steady_timer>(ep(sub).strand());
                    tim->expires_after(std::chrono::seconds(1));
                    tim->async_wait(  // 2nd async call
                        as::consign(
                            *this,
                            tim
                        )
                    );
                }
                yield { // this yield is called twice
                    if (ec) {
                        // 1st
                        BOOST_TEST(!pv); // not recv
                        BOOST_TEST(*ec == am::errc::success); // timeout
                        ep(sub).close(*this);
                    }
                    else {
                        // 2nd
                        BOOST_TEST(!*pv); // recv error due to close
                    }
                }
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
