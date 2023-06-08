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

BOOST_AUTO_TEST_SUITE(st_offline)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(v311_cs1_sp0) {
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
                yield ep().send(
                    am::v3_1_1::publish_packet{
                        *ep().acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish QoS2
                yield ep().send(
                    am::v3_1_1::publish_packet{
                        *ep().acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::yes
                    },
                    *this
                );
                BOOST_TEST(!*se);

                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0,
                        am::allocate_buffer("cid1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);

                yield ep().recv(*this);
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

                // not recv puback
                yield {
                    ep().recv(*this); // 1st async call
                    auto tim = std::make_shared<as::steady_timer>(ep().strand());
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
                        ep().close(*this);
                    }
                    else {
                        // 2nd
                        BOOST_TEST(!*pv); // recv error due to close
                    }
                }
                yield set_finish();
            }
        }
    };

    tc t{amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_cs0_sp0) {
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
                yield ep().send(
                    am::v3_1_1::publish_packet{
                        *ep().acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish QoS2
                yield ep().send(
                    am::v3_1_1::publish_packet{
                        *ep().acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::yes
                    },
                    *this
                );
                BOOST_TEST(!*se);

                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
                        0,
                        am::allocate_buffer("cid1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);

                yield ep().recv(*this);
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

                // if session present is false, offline publish is not sent but erased
                // not recv puback
                yield {
                    ep().recv(*this); // 1st async call
                    auto tim = std::make_shared<as::steady_timer>(ep().strand());
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
                        ep().close(*this);
                    }
                    else {
                        // 2nd
                        BOOST_TEST(!*pv); // recv error due to close
                    }
                }
                yield set_finish();
            }
        }
    };

    tc t{amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_cs0_sp1) {
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
                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
                        0,
                        am::allocate_buffer("cid1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);

                yield ep().recv(*this);
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
                yield ep().close(*this);

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
                yield ep().send(
                    am::v3_1_1::publish_packet{
                        *ep().acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish QoS2
                yield ep().send(
                    am::v3_1_1::publish_packet{
                        *ep().acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::yes
                    },
                    *this
                );
                BOOST_TEST(!*se);

                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
                        0,
                        am::allocate_buffer("cid1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);

                yield ep().recv(*this);
                pv->visit(
                    am::overload {
                        [&](am::v3_1_1::connack_packet const& p) {
                            BOOST_TEST(p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );

                // recv puback
                yield ep().recv(*this);
                BOOST_TEST(*pv == am::v3_1_1::puback_packet{1});
                // recv pubrec
                yield ep().recv(*this);
                BOOST_TEST(*pv == am::v3_1_1::pubrec_packet{2});
                yield ep().close(*this);
                yield set_finish();
            }
        }
    };

    tc t{amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_cs0_sp1_from_broker) {
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
                // connect sub
                yield ep(sub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub).send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
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
                yield ep(sub).send(am::v3_1_1::disconnect_packet{}, *this);
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
                BOOST_TEST(!*pv); // wait and check close by broker

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
                        am::allocate_buffer("payload0"),
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
                        am::allocate_buffer("payload1"),
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
                        am::allocate_buffer("payload2"),
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(pub).send(am::v3_1_1::disconnect_packet{}, *this);
                BOOST_TEST(!*se);
                yield ep(pub).recv(am::filter::match, {}, *this);
                BOOST_TEST(!*pv); // wait and check close by broker

                // connect sub
                yield ep(sub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub).send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
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
                            BOOST_TEST(p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );

                yield ep(sub).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v3_1_1::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload0"),
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
                        am::allocate_buffer("payload1"),
                        am::qos::at_least_once | am::pub::dup::no
                    })
                );
                yield ep(sub).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v3_1_1::publish_packet{
                        2,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload2"),
                        am::qos::exactly_once | am::pub::dup::no
                    })
                );
                yield ep(sub).close(*this);
                yield set_finish();
            }
        }
    };

    tc t{{amep_pub, amep_sub}, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_cs0_sp1_from_broker_mei) {
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
                        am::allocate_buffer("passforu1"),
                        {am::property::session_expiry_interval{am::session_never_expire}}
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
                yield ep(sub).send(am::v5::disconnect_packet{}, *this);
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
                BOOST_TEST(!*pv); // wait and check close by broker

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
                        am::allocate_buffer("payload0"),
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
                        am::allocate_buffer("payload1"),
                        am::qos::at_least_once,
                        {am::property::message_expiry_interval{1}}
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish QoS2
                yield ep(pub).send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload2"),
                        am::qos::exactly_once,
                        {am::property::message_expiry_interval{10}}
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(pub).send(am::v5::disconnect_packet{}, *this);
                BOOST_TEST(!*se);
                yield ep(pub).recv(am::filter::match, {}, *this);
                BOOST_TEST(!*pv); // wait and check close by broker

                // wait for broker QoS1 publish message will expire
                std::this_thread::sleep_for(std::chrono::seconds(2));

                // connect sub
                yield ep(sub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub).send(
                    am::v5::connect_packet{
                        false,   // clean_start
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
                            BOOST_TEST(p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );

                yield ep(sub).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload0"),
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
                        am::qos::exactly_once | am::pub::dup::no,
                        {am::property::message_expiry_interval{7}}
                    })
                );
                yield ep(sub).close(*this);
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
