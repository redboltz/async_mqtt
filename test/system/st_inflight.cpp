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
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        void proc(
            am::error_code ec,
            std::optional<am::packet_variant> pv_opt,
            am::packet_id_type /*pid*/
        ) override {
            reenter(this) {
                yield as::dispatch(
                    as::bind_executor(
                        ep().get_executor(),
                        *this
                    )
                );

                // connect 1
                yield ep().async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
                        0, // keep_alive
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep().async_recv(*this);
                pv_opt->visit(
                    am::overload {
                        [&](am::v3_1_1::connack_packet const& p) {
                            BOOST_TEST(!p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );

                BOOST_TEST(ep().register_packet_id(1));
                BOOST_TEST(ep().register_packet_id(2));
                // publish QoS1
                yield ep().async_send(
                    am::v3_1_1::publish_packet{
                        1,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!ec);

                // publish QoS2
                yield ep().async_send(
                    am::v3_1_1::publish_packet{
                        2,
                        "topic1",
                        "payload1",
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!ec);

                yield ep().async_close(*this);

                // connect 2
                yield ep().async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
                        0, // keep_alive
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep().async_recv(*this);
                pv_opt->visit(
                    am::overload {
                        [&](am::v3_1_1::connack_packet const& p) {
                            BOOST_TEST(p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );
                // recv puback or pubrec
                yield ep().async_recv(*this);
                BOOST_TEST(exp.erase(*pv_opt) == 1);
                // recv pubrec
                yield ep().async_recv(*this);
                BOOST_TEST(exp.erase(*pv_opt) == 1);
                // send pubrel
                yield ep().async_send(am::v3_1_1::pubrel_packet{2}, *this);

                yield ep().async_close(*this);

                // connect 3
                yield ep().async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
                        0, // keep_alive
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep().async_recv(*this);
                pv_opt->visit(
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
                yield ep().async_recv(*this);
                BOOST_TEST(*pv_opt == am::v3_1_1::pubcomp_packet{2});

                yield ep().async_close(*this);
                set_finish();
                guard.reset();
            }
        }

        std::set<am::packet_variant> exp {
            am::v3_1_1::puback_packet{1},
            am::v3_1_1::pubrec_packet{2}
        };
    };

    tc t{amep};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_to_broker) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep = ep_t{
        am::protocol_version::v5,
        ioc.get_executor()
    };

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        void proc(
            am::error_code ec,
            std::optional<am::packet_variant> pv_opt,
            am::packet_id_type /*pid*/
        ) override {
            reenter(this) {
                yield as::dispatch(
                    as::bind_executor(
                        ep().get_executor(),
                        *this
                    )
                );

                // connect 1
                yield ep().async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v5::connect_packet{
                        false,   // clean_start
                        0, // keep_alive
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1",
                        {am::property::session_expiry_interval{am::session_never_expire}}
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep().async_recv(*this);
                pv_opt->visit(
                    am::overload {
                        [&](am::v5::connack_packet const& p) {
                            BOOST_TEST(!p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );

                BOOST_TEST(ep().register_packet_id(1));
                BOOST_TEST(ep().register_packet_id(2));

                // publish QoS1
                yield ep().async_send(
                    am::v5::publish_packet{
                        1,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!ec);

                // publish QoS2
                yield ep().async_send(
                    am::v5::publish_packet{
                        2,
                        "topic1",
                        "payload1",
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!ec);

                yield ep().async_close(*this);

                // connect 2
                yield ep().async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v5::connect_packet{
                        false,   // clean_start
                        0, // keep_alive
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1",
                        {am::property::session_expiry_interval{am::session_never_expire}}
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep().async_recv(*this);
                pv_opt->visit(
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
                yield ep().async_recv(*this);
                BOOST_TEST(exp.erase(*pv_opt) == 1);
                // recv pubrec
                yield ep().async_recv(*this);
                BOOST_TEST(exp.erase(*pv_opt) == 1);

                // send pubrel
                yield ep().async_send(am::v5::pubrel_packet{2}, *this);

                yield ep().async_close(*this);

                // connect 3
                yield ep().async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v5::connect_packet{
                        false,   // clean_start
                        0, // keep_alive
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep().async_recv(*this);
                pv_opt->visit(
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
                yield ep().async_recv(*this);
                BOOST_TEST(*pv_opt == (am::v5::pubcomp_packet{2}));

                yield ep().async_close(*this);
                set_finish();
                guard.reset();
            }
        }

        std::set<am::packet_variant> exp {
            am::v5::puback_packet{1, am::puback_reason_code::no_matching_subscribers},
            am::v5::pubrec_packet{2, am::pubrec_reason_code::no_matching_subscribers},
            am::v5::pubrec_packet{2} // already handled in the broker
        };
    };

    tc t{amep};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_from_broker) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };
    auto amep_sub = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            pub,
            sub
        };
        void proc(
            am::error_code ec,
            std::optional<am::packet_variant> pv_opt,
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
                yield ep(sub).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(sub).async_send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
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
                pv_opt->visit(
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
                BOOST_TEST(pv_opt->get_if<am::v3_1_1::suback_packet>());



                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );

                // connect pub
                yield ep(pub).async_underlying_handshake(
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
                BOOST_TEST(pv_opt->get_if<am::v3_1_1::connack_packet>());

                // publish QoS1
                yield ep(pub).async_send(
                    am::v3_1_1::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload1",
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
                        "payload2",
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!ec);

                yield ep(pub).async_recv(*this); // recv puback
                yield ep(pub).async_recv(*this); // recv pubrec

                // wait for broker delivers publish packets.
                // if close sub too early, offline publish mechanism would work
                // instead of inflight packets processing.
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                yield ep(sub).async_close(*this);

                // connect sub
                yield ep(sub).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(sub).async_send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
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
                pv_opt->visit(
                    am::overload {
                        [&](am::v3_1_1::connack_packet const& p) {
                            BOOST_TEST(p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );

                yield ep(sub).async_recv(*this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v3_1_1::publish_packet{
                        1,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once | am::pub::dup::yes
                    })
                );

                yield ep(sub).async_recv(*this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v3_1_1::publish_packet{
                        2,
                        "topic1",
                        "payload2",
                        am::qos::exactly_once | am::pub::dup::yes
                    })
                );

                yield ep(sub).async_close(*this);

                yield as::dispatch(
                    as::bind_executor(
                        ep().get_executor(),
                        *this
                    )
                );
                yield ep(pub).async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{{amep_pub, amep_sub}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_from_broker) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t{
        am::protocol_version::v5,
        ioc.get_executor()
    };
    auto amep_sub = ep_t{
        am::protocol_version::v5,
        ioc.get_executor()
    };

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            pub,
            sub
        };
        void proc(
            am::error_code ec,
            std::optional<am::packet_variant> pv_opt,
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
                yield ep(sub).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(sub).async_send(
                    am::v5::connect_packet{
                        false,   // clean_start
                        0, // keep_alive
                        "sub",
                        std::nullopt, // will
                        "u1",
                        "passforu1",
                        {am::property::session_expiry_interval{am::session_never_expire}}
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub).async_recv(*this);
                pv_opt->visit(
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
                BOOST_TEST(pv_opt->get_if<am::v5::suback_packet>());


                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );
                // connect pub
                yield ep(pub).async_underlying_handshake(
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
                BOOST_TEST(pv_opt->get_if<am::v5::connack_packet>());

                // publish QoS1
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload1",
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
                        "payload2",
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!ec);

                yield ep(pub).async_recv(*this); // recv puback
                yield ep(pub).async_recv(*this); // recv pubrec

                // wait for broker delivers publish packets.
                // if close sub too early, offline publish mechanism would work
                // instead of inflight packets processing.
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                yield ep(sub).async_close(*this);

                // connect sub
                yield ep(sub).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(sub).async_send(
                    am::v5::connect_packet{
                        false,   // clean_start
                        0, // keep_alive
                        "sub",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );
                yield ep(sub).async_recv(*this);
                pv_opt->visit(
                    am::overload {
                        [&](am::v5::connack_packet const& p) {
                            BOOST_TEST(p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );

                yield ep(sub).async_recv(*this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        1,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once | am::pub::dup::yes
                    })
                );

                yield ep(sub).async_recv(*this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        2,
                        "topic1",
                        "payload2",
                        am::qos::exactly_once | am::pub::dup::yes
                    })
                );

                yield ep(sub).async_close(*this);

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

    tc t{{amep_pub, amep_sub}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_from_broker_mei) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t{
        am::protocol_version::v5,
        ioc.get_executor()
    };
    auto amep_sub = ep_t{
        am::protocol_version::v5,
        ioc.get_executor()
    };

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            pub,
            sub
        };
        void proc(
            am::error_code ec,
            std::optional<am::packet_variant> pv_opt,
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
                yield ep(sub).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(sub).async_send(
                    am::v5::connect_packet{
                        false,   // clean_start
                        0, // keep_alive
                        "sub",
                        std::nullopt, // will
                        "u1",
                        "passforu1",
                        {am::property::session_expiry_interval{am::session_never_expire}}
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub).async_recv(*this);
                pv_opt->visit(
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
                BOOST_TEST(pv_opt->get_if<am::v5::suback_packet>());


                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );

                // connect pub
                yield ep(pub).async_underlying_handshake(
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
                BOOST_TEST(pv_opt->get_if<am::v5::connack_packet>());

                // publish QoS1
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload1",
                        am::qos::at_least_once,
                        {am::property::message_expiry_interval{1}}
                    },
                    *this
                );
                BOOST_TEST(!ec);

                // publish QoS2
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload2",
                        am::qos::exactly_once,
                        {am::property::message_expiry_interval{10}}
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

                // 2 > 1 (QoS1 message_expiry_interval)
                // but PUBLISH has already been triggered
                // so message shouldn't be expired
                std::this_thread::sleep_for(std::chrono::seconds(2));
                yield ep(sub).async_close(*this);

                // connect sub
                yield ep(sub).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(sub).async_send(
                    am::v5::connect_packet{
                        false,   // clean_start
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
                pv_opt->visit(
                    am::overload {
                        [&](am::v5::connack_packet const& p) {
                            BOOST_TEST(p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );

                yield ep(sub).async_recv(*this);
                // message_expiry_interval shouldn't be updated
                // because PUBLISH has already been triggered
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        1,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once | am::pub::dup::yes,
                        {am::property::message_expiry_interval{1}}
                    })
                );

                yield ep(sub).async_recv(*this);
                // message_expiry_interval shouldn't be updated
                // because PUBLISH has already been triggered
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        2,
                        "topic1",
                        "payload2",
                        am::qos::exactly_once | am::pub::dup::yes,
                        {am::property::message_expiry_interval{10}}
                    })
                );

                yield ep(sub).async_close(*this);

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

    tc t{{amep_pub, amep_sub}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
