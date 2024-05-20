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

BOOST_AUTO_TEST_SUITE(st_auth)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(fail_plain) {
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
            std::optional<am::error_code> ec,
            std::optional<am::system_error> se,
            std::optional<am::packet_variant> pv,
            std::optional<am::packet_id_type> /*pid*/
        ) override {
            reenter(this) {
                yield as::dispatch(
                    as::bind_executor(
                        ep().get_executor(),
                        *this
                    )
                );
                yield ep().next_layer().async_connect(
                    dest(),
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
                        "invalid_pass"
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().async_recv(*this);
                pv->visit(
                    am::overload {
                        [&](am::v3_1_1::connack_packet const& p) {
                            BOOST_TEST(!p.session_present());
                            BOOST_TEST(p.code() == am::connect_return_code::not_authorized);
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );
                yield ep().async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{*amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(success_digest) {
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
            std::optional<am::error_code> ec,
            std::optional<am::system_error> se,
            std::optional<am::packet_variant> pv,
            std::optional<am::packet_id_type> /*pid*/
        ) override {
            reenter(this) {
                yield as::dispatch(
                    as::bind_executor(
                        ep().get_executor(),
                        *this
                    )
                );
                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().async_send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0x1234, // keep_alive
                        "cid1",
                        std::nullopt, // will
                        "u3",
                        "passforu3" // plain
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().async_recv(*this);
                pv->visit(
                    am::overload {
                        [&](am::v3_1_1::connack_packet const& p) {
                            BOOST_TEST(!p.session_present());
                            BOOST_TEST(p.code() == am::connect_return_code::accepted);
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );
                yield ep().async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{*amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(fail_digest) {
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
            std::optional<am::error_code> ec,
            std::optional<am::system_error> se,
            std::optional<am::packet_variant> pv,
            std::optional<am::packet_id_type> /*pid*/
        ) override {
            reenter(this) {
                yield as::dispatch(
                    as::bind_executor(
                        ep().get_executor(),
                        *this
                    )
                );
                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().async_send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0x1234, // keep_alive
                        "cid1",
                        std::nullopt, // will
                        "u3",
                        "invalid_pass"
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().async_recv(*this);
                pv->visit(
                    am::overload {
                        [&](am::v3_1_1::connack_packet const& p) {
                            BOOST_TEST(!p.session_present());
                            BOOST_TEST(p.code() == am::connect_return_code::not_authorized);
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );
                yield ep().async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{*amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(send_auth) {
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
            std::optional<am::error_code> ec,
            std::optional<am::system_error> se,
            std::optional<am::packet_variant> pv,
            std::optional<am::packet_id_type> /*pid*/
        ) override {
            reenter(this) {
                yield as::dispatch(
                    as::bind_executor(
                        ep().get_executor(),
                        *this
                    )
                );
                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
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
                        [&](am::v5::connack_packet const& p) {
                            BOOST_TEST(!p.session_present());
                            BOOST_TEST(p.code() == am::connect_reason_code::success);
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );
                yield ep().async_send(
                    am::v5::auth_packet{
                        am::auth_reason_code::success
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{*amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
