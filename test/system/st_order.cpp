// Copyright Takatoshi Kondo 2024
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

BOOST_AUTO_TEST_SUITE(st_order)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(v311_qos0) {
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
                count = 0;
                while (true) {
                    yield ep(pub).async_send(
                        am::v3_1_1::publish_packet{
                            "topic1",
                            "payload" + std::to_string(++count),
                            am::qos::at_most_once
                        },
                        *this
                    );
                    BOOST_TEST(!ec);
                    if (count == 100) break;
                }

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );

                // recv publish QoS0
                count = 0;
                while (true) {
                    yield ep(sub).async_recv(*this);
                    BOOST_TEST(
                        pv
                        ==
                        (am::v3_1_1::publish_packet{
                            "topic1",
                            "payload" + std::to_string(++count),
                            am::qos::at_most_once
                        })
                    );
                    if (count == 100) break;
                }
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

        int count;
    };

    tc t{{*amep_pub, *amep_sub}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_qos1) {
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

                // publish QoS1
                count = 0;
                while (true) {
                    yield ep(pub).async_send(
                        am::v3_1_1::publish_packet{
                            *ep(pub).acquire_unique_packet_id(),
                            "topic1",
                            "payload" + std::to_string(++count),
                            am::qos::at_least_once
                        },
                        *this
                    );
                    BOOST_TEST(!ec);
                    yield ep(pub).async_recv(*this); // recv puback
                    if (count == 100) break;
                }

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );

                // recv publish QoS1
                count = 0;
                while (true) {
                    yield ep(sub).async_recv(*this);
                    BOOST_ASSERT(pv.get_if<am::v3_1_1::publish_packet>());
                    BOOST_TEST(pv.get_if<am::v3_1_1::publish_packet>()->opts().get_qos() == am::qos::at_least_once);
                    BOOST_TEST(
                        pv.get_if<am::v3_1_1::publish_packet>()->payload()
                        ==
                        "payload" + std::to_string(++count)
                    );
                    if (count == 100) break;
                }
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

        int count;
    };

    tc t{{*amep_pub, *amep_sub}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_qos2) {
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

                // publish QoS2
                count = 0;
                while (true) {
                    yield ep(pub).async_send(
                        am::v3_1_1::publish_packet{
                            *ep(pub).acquire_unique_packet_id(),
                            "topic1",
                            "payload" + std::to_string(++count),
                            am::qos::exactly_once
                        },
                        *this
                    );
                    BOOST_TEST(!ec);
                    yield ep(pub).async_recv(*this); // recv pubrec
                    if (count == 100) break;
                }

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );

                // recv publish QoS2
                count = 0;
                while (true) {
                    yield ep(sub).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                    BOOST_ASSERT(pv.get_if<am::v3_1_1::publish_packet>());
                    BOOST_TEST(pv.get_if<am::v3_1_1::publish_packet>()->opts().get_qos() == am::qos::exactly_once);
                    BOOST_TEST(
                        pv.get_if<am::v3_1_1::publish_packet>()->payload()
                        ==
                        "payload" + std::to_string(++count)
                    );
                    if (count == 100) break;
                }
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

        int count;
    };

    tc t{{*amep_pub, *amep_sub}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_qos0) {
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
                count = 0;
                while (true) {
                    yield ep(pub).async_send(
                        am::v3_1_1::publish_packet{
                            "topic1",
                            "payload" + std::to_string(++count),
                            am::qos::at_most_once
                        },
                        *this
                    );
                    BOOST_TEST(!ec);
                    if (count == 100) break;
                }

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );

                // recv publish QoS0
                count = 0;
                while (true) {
                    yield ep(sub).async_recv(*this);
                    BOOST_TEST(
                        pv
                        ==
                        (am::v3_1_1::publish_packet{
                            "topic1",
                            "payload" + std::to_string(++count),
                            am::qos::at_most_once
                        })
                    );
                    if (count == 100) break;
                }
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

        int count;
    };

    tc t{{*amep_pub, *amep_sub}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_qos1) {
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
                BOOST_TEST(pv.get_if<am::v5::connack_packet>());

                // publish QoS1
                count = 0;
                while (true) {
                    yield ep(pub).async_send(
                        am::v5::publish_packet{
                            *ep(pub).acquire_unique_packet_id(),
                            "topic1",
                            "payload" + std::to_string(++count),
                            am::qos::at_least_once
                        },
                        *this
                    );
                    BOOST_TEST(!ec);
                    yield ep(pub).async_recv(*this); // recv puback
                    if (count == 100) break;
                }

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );

                // recv publish QoS1
                count = 0;
                while (true) {
                    yield ep(sub).async_recv(*this);
                    BOOST_ASSERT(pv.get_if<am::v5::publish_packet>());
                    BOOST_TEST(pv.get_if<am::v5::publish_packet>()->opts().get_qos() == am::qos::at_least_once);
                    BOOST_TEST(
                        pv.get_if<am::v5::publish_packet>()->payload()
                        ==
                        "payload" + std::to_string(++count)
                    );
                    if (count == 100) break;
                }
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

        int count;
    };

    tc t{{*amep_pub, *amep_sub}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_qos2) {
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
                BOOST_TEST(pv.get_if<am::v5::connack_packet>());

                // publish QoS2
                count = 0;
                while (true) {
                    yield ep(pub).async_send(
                        am::v5::publish_packet{
                            *ep(pub).acquire_unique_packet_id(),
                            "topic1",
                            "payload" + std::to_string(++count),
                            am::qos::exactly_once
                        },
                        *this
                    );
                    BOOST_TEST(!ec);
                    yield ep(pub).async_recv(*this); // recv pubrec
                    if (count == 100) break;
                }

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub).get_executor(),
                        *this
                    )
                );

                // recv publish QoS2
                count = 0;
                while (true) {
                    yield ep(sub).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                    BOOST_ASSERT(pv.get_if<am::v5::publish_packet>());
                    BOOST_TEST(pv.get_if<am::v5::publish_packet>()->opts().get_qos() == am::qos::exactly_once);
                    BOOST_TEST(
                        pv.get_if<am::v5::publish_packet>()->payload()
                        ==
                        "payload" + std::to_string(++count)
                    );
                    if (count == 100) break;
                }
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

        int count;
    };

    tc t{{*amep_pub, *amep_sub}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
