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

BOOST_AUTO_TEST_SUITE(st_reqres)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(generate_reuse_renew) {
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
            am::packet_variant pv,
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
                        true,   // clean_start
                        0, // keep_alive
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1",
                        {
                            am::property::request_response_information{true},
                            am::property::session_expiry_interval{am::session_never_expire}
                        }
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep().async_recv(*this);
                pv.visit(
                    am::overload {
                        [&](am::v5::connack_packet& p) {
                            BOOST_TEST(!p.session_present());
                            BOOST_TEST(p.code() == am::connect_reason_code::success);
                            for (auto& prop : p.props()) {
                                prop.visit(
                                    am::overload{
                                        [this](am::property::response_information const& v) {
                                            response_topic = v.val();
                                        },
                                        [](auto const&){}
                                    }
                                );
                            }
                            BOOST_TEST(!response_topic.empty());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );
                yield ep().async_close(*this);

                // reconnect inherit
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
                        {
                            am::property::request_response_information{true},
                            am::property::session_expiry_interval{am::session_never_expire}
                        }
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep().async_recv(*this);
                pv.visit(
                    am::overload {
                        [&](am::v5::connack_packet& p) {
                            BOOST_TEST(p.session_present());
                            BOOST_TEST(p.code() == am::connect_reason_code::success);
                            for (auto& prop : p.props()) {
                                prop.visit(
                                    am::overload{
                                        [this](am::property::response_information const& v) {
                                            // same as the previous response topic
                                            BOOST_TEST(response_topic == v.val());
                                        },
                                        [](auto const&){}
                                    }
                                );
                            }
                            BOOST_TEST(!response_topic.empty());
                        },
                        [](auto const&) {
                            BOOST_TEST(false);
                        }
                   }
                );
                yield ep().async_close(*this);

                // reconnect no inherit (clean_start)
                yield ep().async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep().async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1",
                        {
                            am::property::request_response_information{true}
                        }
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep().async_recv(*this);
                pv.visit(
                    am::overload {
                        [&](am::v5::connack_packet& p) {
                            BOOST_TEST(!p.session_present());
                            BOOST_TEST(p.code() == am::connect_reason_code::success);
                            for (auto& prop : p.props()) {
                                prop.visit(
                                    am::overload{
                                        [this](am::property::response_information const& v) {
                                            // response topic regenereted
                                            BOOST_TEST(!v.val().empty());
                                            BOOST_TEST(response_topic != v.val());
                                        },
                                        [](auto const&){}
                                    }
                                );
                            }
                            BOOST_TEST(!response_topic.empty());
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

        std::string response_topic;
    };

    tc t{amep};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
