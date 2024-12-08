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

BOOST_AUTO_TEST_SUITE(st_keep_alive)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(v311_timeout) {
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
                yield ep().async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        1,  // 1sec
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep().async_recv({am::control_packet_type::connack}, *this);
                BOOST_TEST(pv_opt->get_if<am::v3_1_1::connack_packet>());
                ep().set_pingreq_send_interval(std::chrono::seconds{10});
                yield ep().async_recv(am::filter::except, {am::control_packet_type::pingresp}, *this);
                BOOST_TEST(ec);
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

BOOST_AUTO_TEST_CASE(v5_timeout) {
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
                yield ep().async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v5::connect_packet{
                        true,   // clean_session
                        1,  // 1sec
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1",
                        am::properties{}
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep().async_recv({am::control_packet_type::connack}, *this);
                BOOST_TEST(pv_opt->get_if<am::v5::connack_packet>());
                ep().set_pingreq_send_interval(std::chrono::seconds{10});
                yield ep().async_recv(am::filter::except, {am::control_packet_type::pingresp}, *this);
                BOOST_TEST(pv_opt->get_if<am::v5::disconnect_packet>());
                yield ep().async_recv(am::filter::except, {am::control_packet_type::pingresp}, *this);
                BOOST_TEST(ec);
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

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
