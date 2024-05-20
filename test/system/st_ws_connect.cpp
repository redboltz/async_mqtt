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
#include <async_mqtt/predefined_layer/ws.hpp>

#include <boost/asio/yield.hpp>

BOOST_AUTO_TEST_SUITE(st_connect)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(cb) {
    broker_runner br;
    as::io_context ioc;

    using ep_t = am::endpoint<am::role::client, am::protocol::ws>;
    auto amep = ep_t::create(
        am::protocol_version::v3_1_1,
        am::protocol::ws{ioc.get_executor()}
    );

    am::async_underlying_handshake(
        amep->next_layer(),
        "127.0.0.1",
        "10080",
        [&](am::error_code const& ec) {
            BOOST_TEST(ec == am::error_code{});
            amep->async_send(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1",
                    std::nullopt, // will
                    "u1",
                    "passforu1"
                },
                [&](am::system_error const& se) {
                    BOOST_TEST(!se);
                    amep->async_recv(
                        [&](am::packet_variant pv) {
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
                            amep->async_close([]{});
                        }
                    );
                }
            );
        }
    );

    ioc.run();
}

BOOST_AUTO_TEST_CASE(fut) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::ws>;
    auto amep = ep_t::create(
        am::protocol_version::v3_1_1,
        am::protocol::ws{ioc.get_executor()}
    );

    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };
    auto on_finish = am::unique_scope_guard(
        [&] {
            guard.reset();
            th.join();
        }
    );

    {
        auto fut = am::async_underlying_handshake(
            amep->next_layer(),
            "127.0.0.1",
            "10080",
            "/",
            as::use_future
        );
        try {
            fut.get();
        }
        catch (am::error_code const&) {
            BOOST_TEST(false);
        }
    }
    {
        auto fut =
            amep->async_send(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1",
                    std::nullopt, // will
                    "u1",
                    "passforu1"
                },
                as::use_future
            );
        am::system_error se = fut.get();
        BOOST_TEST(!se);
    }
    {
        auto fut =
            amep->async_recv(as::use_future);
        auto pv = fut.get();
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
    }
    {
        auto fut = amep->async_close(as::use_future);
        fut.get();
    }
}

BOOST_AUTO_TEST_CASE(coro) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::ws>;
    auto amep = ep_t::create(
        am::protocol_version::v3_1_1,
        am::protocol::ws{ioc.get_executor()}
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        void proc(
            std::optional<am::error_code> ec,
            std::optional<am::system_error> se,
            std::optional<am::packet_variant> pv,
            std::optional<am::packet_id_type> /*pid*/
        ) override {
            reenter(this) {
                yield am::async_underlying_handshake(
                    ep().next_layer(),
                    "127.0.0.1",
                    "10080",
                    "/",
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().async_send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0x1234, // keep_alive
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().async_recv(*this);
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
                yield ep().async_close(*this);
                set_finish();
            }
        }
    };

    tc t{*amep};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}


BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
