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

BOOST_AUTO_TEST_SUITE(st_inflight)

BOOST_AUTO_TEST_CASE(v311_to_broker) {
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
                // connect 1
                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
                        0, // keep_alive
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
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!*se);

                yield ep().close(*this);

                // connect 2
                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
                        0, // keep_alive
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
                // send pubrel
                yield ep().send(am::v3_1_1::pubrel_packet{2}, *this);

                yield ep().close(*this);

                // connect 3
                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
                        0, // keep_alive
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
                // recv pubcomp
                yield ep().recv(*this);
                BOOST_TEST(*pv == am::v3_1_1::pubcomp_packet{2});

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

BOOST_AUTO_TEST_CASE(v5_to_broker) {
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
                // connect 1
                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v5::connect_packet{
                        false,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("cid1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1"),
                        {am::property::session_expiry_interval{am::session_never_expire}}
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
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
                // publish QoS1
                yield ep().send(
                    am::v5::publish_packet{
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
                    am::v5::publish_packet{
                        *ep().acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!*se);

                yield ep().close(*this);

                // connect 2
                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v5::connect_packet{
                        false,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("cid1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1"),
                        {am::property::session_expiry_interval{am::session_never_expire}}
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
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
                // recv puback
                yield ep().recv(*this);
                BOOST_TEST(*pv == (am::v5::puback_packet{1}));
                // recv pubrec
                yield ep().recv(*this);
                BOOST_TEST(*pv == (am::v5::pubrec_packet{2}));

                // send pubrel
                yield ep().send(am::v5::pubrel_packet{2}, *this);

                yield ep().close(*this);

                // connect 3
                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v5::connect_packet{
                        false,   // clean_start
                        0, // keep_alive
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
                        [&](am::v5::connack_packet const& p) {
                            BOOST_TEST(p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );
                // recv pubcomp
                yield ep().recv(*this);
                BOOST_TEST(*pv == (am::v5::pubcomp_packet{2}));

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

                // wait for broker delivers publish packets.
                // if close sub too early, offline publish mechanism would work
                // instead of inflight packets processing.
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                yield ep(sub).close(*this);

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
                        1,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_least_once | am::pub::dup::yes
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
                        am::qos::exactly_once | am::pub::dup::yes
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

                // publish QoS1
                yield ep(pub).send(
                    am::v5::publish_packet{
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
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload2"),
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // wait for broker delivers publish packets.
                // if close sub too early, offline publish mechanism would work
                // instead of inflight packets processing.
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                yield ep(sub).close(*this);

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
                        1,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_least_once | am::pub::dup::yes
                    })
                );

                yield ep(sub).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        2,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload2"),
                        am::qos::exactly_once | am::pub::dup::yes
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

BOOST_AUTO_TEST_CASE(v5_from_broker_mei) {
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
                        false,   // clean_start
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

                // wait for broker QoS1 publish message will expire
                std::this_thread::sleep_for(std::chrono::seconds(2));
                yield ep(sub).close(*this);

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
                        2,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload2"),
                        am::qos::exactly_once | am::pub::dup::yes,
                        {am::property::message_expiry_interval{9}}
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

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
