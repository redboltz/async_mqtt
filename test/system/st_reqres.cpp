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
                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("cid1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1"),
                        {
                            am::property::request_response_information{true},
                            am::property::session_expiry_interval{am::session_never_expire}
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
                pv->visit(
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
                yield ep().close(*this);

                // reconnect inherit
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
                        {
                            am::property::request_response_information{true},
                            am::property::session_expiry_interval{am::session_never_expire}
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
                pv->visit(
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
                yield ep().close(*this);

                // reconnect no inherit (clean_start)
                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("cid1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1"),
                        {
                            am::property::request_response_information{true}
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
                pv->visit(
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
                yield ep().close(*this);
                yield set_finish();
            }
        }

        am::buffer response_topic;
    };

    tc t{amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
