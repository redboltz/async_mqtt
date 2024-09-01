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

BOOST_AUTO_TEST_SUITE(st_conflict_cid)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(v311_cs1to1) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep1 = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };
    auto amep2 = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            c1,
            c2
        };
        void proc(
            am::error_code ec,
            am::packet_variant pv,
            am::packet_id_type /*pid*/
        ) override {
            reenter(this) {
                yield as::dispatch(
                    as::bind_executor(
                        ep(c1).get_executor(),
                        *this
                    )
                );
                yield am::async_underlying_handshake(
                    ep(c1).next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(c1).async_send(
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
                yield ep(c1).async_recv(*this);
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

                yield as::dispatch(
                    as::bind_executor(
                        ep(c2).get_executor(),
                        *this
                    )
                );
                yield am::async_underlying_handshake(
                    ep(c2).next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(c2).async_send(
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
                yield ep(c2).async_recv(*this);
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

                yield ep(c2).async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{{amep1, amep2}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_cs0to1) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep1 = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };
    auto amep2 = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            c1,
            c2
        };
        void proc(
            am::error_code ec,
            am::packet_variant pv,
            am::packet_id_type /*pid*/
        ) override {
            reenter(this) {
                yield as::dispatch(
                    as::bind_executor(
                        ep(c1).get_executor(),
                        *this
                    )
                );
                yield am::async_underlying_handshake(
                    ep(c1).next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(c1).async_send(
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
                yield ep(c1).async_recv(*this);
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

                yield as::dispatch(
                    as::bind_executor(
                        ep(c2).get_executor(),
                        *this
                    )
                );
                yield am::async_underlying_handshake(
                    ep(c2).next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(c2).async_send(
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
                yield ep(c2).async_recv(*this);
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

                yield ep(c2).async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{{amep1, amep2}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_cs0to0) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep1 = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };
    auto amep2 = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            c1,
            c2
        };
        void proc(
            am::error_code ec,
            am::packet_variant pv,
            am::packet_id_type /*pid*/
        ) override {
            reenter(this) {
                yield as::dispatch(
                    as::bind_executor(
                        ep(c1).get_executor(),
                        *this
                    )
                );
                yield am::async_underlying_handshake(
                    ep(c1).next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(c1).async_send(
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
                yield ep(c1).async_recv(*this);
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

                yield as::dispatch(
                    as::bind_executor(
                        ep(c2).get_executor(),
                        *this
                    )
                );
                yield am::async_underlying_handshake(
                    ep(c2).next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(c2).async_send(
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
                yield ep(c2).async_recv(*this);
                pv.visit(
                    am::overload {
                        [&](am::v3_1_1::connack_packet const& p) {
                            BOOST_TEST(p.session_present());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );

                yield ep(c2).async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{{amep1, amep2}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_cs0offto1) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep1 = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };
    auto amep2 = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            c1,
            c2
        };
        void proc(
            am::error_code ec,
            am::packet_variant pv,
            am::packet_id_type /*pid*/
        ) override {
            reenter(this) {
                yield as::dispatch(
                    as::bind_executor(
                        ep(c1).get_executor(),
                        *this
                    )
                );
                yield am::async_underlying_handshake(
                    ep(c1).next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(c1).async_send(
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
                yield ep(c1).async_recv(*this);
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
                yield ep(c1).async_close(*this);

                yield as::dispatch(
                    as::bind_executor(
                        ep(c2).get_executor(),
                        *this
                    )
                );
                yield am::async_underlying_handshake(
                    ep(c2).next_layer(),
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(c2).async_send(
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
                yield ep(c2).async_recv(*this);
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

                yield ep(c2).async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{{amep1, amep2}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
