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

BOOST_AUTO_TEST_SUITE(st_cancel)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(ep) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep = ep_t{
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    };

    as::cancellation_signal sig1;
    std::size_t canceled = 0;
    amep.async_underlying_handshake(
        "127.0.0.1",
        "1883",
        [&](am::error_code const& ec) {
            BOOST_TEST(ec == am::error_code{});
            amep.async_send(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1",
                    std::nullopt, // will
                    "u1",
                    "passforu1"
                },
                [&](am::error_code const& ec) {
                    BOOST_TEST(!ec);
                    amep.async_recv(
                        [&](am::error_code const& ec, std::optional<am::packet_variant> pv_opt) {
                            BOOST_TEST(!ec);
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

                            // test case
                            amep.async_recv(
                                as::bind_cancellation_slot(
                                    sig1.slot(),
                                    [&](am::error_code const& ec, std::optional<am::packet_variant> pv_opt) {
                                        BOOST_TEST(ec == as::error::operation_aborted);
                                        BOOST_TEST(!pv_opt);
                                        ++canceled;
                                        amep.async_recv(
                                            as::bind_cancellation_slot(
                                                sig1.slot(),
                                                [&](am::error_code const& ec, std::optional<am::packet_variant> pv_opt) {
                                                    BOOST_TEST(ec == as::error::operation_aborted);
                                                    BOOST_TEST(!pv_opt);
                                                    ++canceled;
                                                    amep.async_close(as::detached);
                                                }
                                            )
                                        );
                                        auto tim = std::make_shared<as::steady_timer>(
                                            ioc.get_executor(),
                                            std::chrono::milliseconds(100)
                                        );
                                        tim->async_wait(
                                            [&, tim](am::error_code const& ec) {
                                                BOOST_TEST(ec == am::errc::success);
                                                sig1.emit(as::cancellation_type::terminal);
                                            }
                                        );
                                    }
                                )
                            );
                            auto tim = std::make_shared<as::steady_timer>(
                                ioc.get_executor(),
                                std::chrono::milliseconds(100)
                            );
                            tim->async_wait(
                                [&, tim](am::error_code const& ec) {
                                    BOOST_TEST(ec == am::errc::success);
                                    sig1.emit(as::cancellation_type::terminal);
                                }
                            );
                        }
                    );
                }
            );
        }
    );

    ioc.run();
    BOOST_TEST(canceled == 2);
}

BOOST_AUTO_TEST_CASE(cl) {
    broker_runner br;
    as::io_context ioc;
    using cl_t = am::client<am::protocol_version::v3_1_1, am::protocol::mqtt>;
    auto amcl = cl_t{
        ioc.get_executor()
    };

    as::cancellation_signal sig1;
    std::size_t canceled = 0;
    amcl.async_underlying_handshake(
        "127.0.0.1",
        "1883",
        [&](am::error_code const& ec) {
            BOOST_TEST(ec == am::error_code{});
            amcl.async_start(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1",
                    std::nullopt, // will
                    "u1",
                    "passforu1"
                },
                [&](am::error_code const& ec, std::optional<cl_t::connack_packet> connack_opt) {
                    BOOST_TEST(!ec);
                    BOOST_CHECK(connack_opt);
                    // test case
                    amcl.async_recv(
                        as::bind_cancellation_slot(
                            sig1.slot(),
                            [&](am::error_code const& ec, std::optional<am::packet_variant> pv_opt) {
                                BOOST_TEST(ec == as::error::operation_aborted);
                                BOOST_TEST(!pv_opt);
                                ++canceled;
                                amcl.async_recv(
                                    as::bind_cancellation_slot(
                                        sig1.slot(),
                                        [&](am::error_code const& ec, std::optional<am::packet_variant> pv_opt) {
                                            BOOST_TEST(ec == as::error::operation_aborted);
                                            BOOST_TEST(!pv_opt);
                                            ++canceled;
                                            amcl.async_close(as::detached);
                                        }
                                    )
                                );
                                auto tim = std::make_shared<as::steady_timer>(
                                    ioc.get_executor(),
                                    std::chrono::milliseconds(100)
                                );
                                tim->async_wait(
                                    [&, tim](am::error_code const& ec) {
                                        BOOST_TEST(ec == am::errc::success);
                                        sig1.emit(as::cancellation_type::terminal);
                                    }
                                );
                            }
                        )
                    );
                    auto tim = std::make_shared<as::steady_timer>(
                        ioc.get_executor(),
                        std::chrono::milliseconds(100)
                    );
                    tim->async_wait(
                        [&, tim](am::error_code const& ec) {
                            BOOST_TEST(ec == am::errc::success);
                            sig1.emit(as::cancellation_type::terminal);
                        }
                    );
                }
            );
        }
    );

    ioc.run();
    BOOST_TEST(canceled == 2);
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
