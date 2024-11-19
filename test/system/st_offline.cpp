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
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };

    static int count = 0;
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

                ep().set_offline_publish(true);

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
                yield ep().async_send(
                    am::v3_1_1::publish_packet{
                        *ep().acquire_unique_packet_id(),
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
                        *ep().acquire_unique_packet_id(),
                        "topic1",
                        "payload1",
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::yes
                    },
                    *this
                );
                BOOST_TEST(!ec);

                yield ep().async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0,
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

                // not recv puback
                yield {
                    ep().async_recv(*this); // 1st async call
                    auto tim = std::make_shared<as::steady_timer>(ep().get_executor());
                    tim->expires_after(std::chrono::seconds(1));
                    tim->async_wait(  // 2nd async call
                        as::consign(
                            *this,
                            tim
                        )
                    );
                }
                yield { // this yield is called twice
                    if (count++ == 0) {
                        // 1st
                        BOOST_TEST(ec == am::errc::success); // timeout
                        BOOST_TEST(!pv_opt); // not recv
                        ep().async_close(*this);
                    }
                    else {
                        // 2nd
                        BOOST_TEST(!pv_opt); // recv error due to close
                    }
                }
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{amep};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_cs0_sp0) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };

    static int count = 0;
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

                ep().set_offline_publish(true);

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
                yield ep().async_send(
                    am::v3_1_1::publish_packet{
                        *ep().acquire_unique_packet_id(),
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
                        *ep().acquire_unique_packet_id(),
                        "topic1",
                        "payload1",
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::yes
                    },
                    *this
                );
                BOOST_TEST(!ec);

                yield ep().async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
                        0,
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

                // if session present is false, offline publish is not sent but erased
                // not recv puback
                yield {
                    ep().async_recv(*this); // 1st async call
                    auto tim = std::make_shared<as::steady_timer>(ep().get_executor());
                    tim->expires_after(std::chrono::seconds(1));
                    tim->async_wait(  // 2nd async call
                        as::consign(
                            *this,
                            tim
                        )
                    );
                }
                yield { // this yield is called twice
                    if (count++ == 0) {
                        // 1st
                        BOOST_TEST(ec == am::errc::success); // timeout
                        BOOST_TEST(!pv_opt); // not recv
                        ep().async_close(*this);
                    }
                    else {
                        // 2nd
                        BOOST_TEST(!pv_opt); // recv error due to close
                    }
                }
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{amep};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_cs0_sp1) {
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

                ep().set_offline_publish(true);

                yield ep().async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
                        0,
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
                yield ep().async_close(*this);

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
                yield ep().async_send(
                    am::v3_1_1::publish_packet{
                        *ep().acquire_unique_packet_id(),
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
                        *ep().acquire_unique_packet_id(),
                        "topic1",
                        "payload1",
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::yes
                    },
                    *this
                );
                BOOST_TEST(!ec);

                yield ep().async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v3_1_1::connect_packet{
                        false,   // clean_session
                        0,
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

                // recv puback
                yield ep().async_recv(*this);
                BOOST_TEST(*pv_opt == am::v3_1_1::puback_packet{1});
                // recv pubrec
                yield ep().async_recv(*this);
                BOOST_TEST(*pv_opt == am::v3_1_1::pubrec_packet{2});
                yield ep().async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{amep};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_cs0_sp1_from_broker) {
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
                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );

                ep(pub).set_offline_publish(true);
                // To avoid windows environment close just after sent problem.
                ep(pub).set_close_delay_after_disconnect_sent(std::chrono::milliseconds{100});

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
                yield ep(sub).async_send(am::v3_1_1::disconnect_packet{}, *this);
                BOOST_TEST(!ec);
                yield ep(sub).async_recv(*this);
                BOOST_TEST(!pv_opt); // wait and check close by broker

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
                // To avoid windows environment close just after sent problem.
                ep(pub).set_close_delay_after_disconnect_sent(std::chrono::milliseconds{100});
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

                // publish QoS0
                yield ep(pub).async_send(
                    am::v3_1_1::publish_packet{
                        "topic1",
                        "payload0",
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
                yield ep(pub).async_send(am::v3_1_1::disconnect_packet{}, *this);
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(am::filter::match, {}, *this);
                BOOST_TEST(!pv_opt); // wait and check close by broker

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
                        "topic1",
                        "payload0",
                        am::qos::at_most_once
                    })
                );
                yield ep(sub).async_recv(*this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v3_1_1::publish_packet{
                        1,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once | am::pub::dup::no
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
                        am::qos::exactly_once | am::pub::dup::no
                    })
                );
                yield ep(sub).async_close(*this);
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

BOOST_AUTO_TEST_CASE(v5_cs0_sp1_from_broker_mei) {
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
                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );

                ep(pub).set_offline_publish(true);
                // To avoid windows environment close just after sent problem.
                ep(pub).set_close_delay_after_disconnect_sent(std::chrono::milliseconds{100});

                // connect sub
                yield ep(sub).async_underlying_handshake(
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
                yield ep(sub).async_send(am::v5::disconnect_packet{}, *this);
                BOOST_TEST(!ec);
                yield ep(sub).async_recv(*this);
                BOOST_TEST(!pv_opt); // wait and check close by broker

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

                // publish QoS0
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        "topic1",
                        "payload0",
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
                yield ep(pub).async_send(am::v5::disconnect_packet{}, *this);
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(am::filter::match, {}, *this);
                BOOST_TEST(!pv_opt); // wait and check close by broker

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );

                // wait for broker QoS1 publish message will expire
                std::this_thread::sleep_for(std::chrono::seconds(2));

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
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        "topic1",
                        "payload0",
                        am::qos::at_most_once
                    })
                );
                yield ep(sub).async_recv(*this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        1,
                        "topic1",
                        "payload2",
                        am::qos::exactly_once | am::pub::dup::no,
                        {am::property::message_expiry_interval{7}}
                    })
                );
                yield ep(sub).async_close(*this);
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
