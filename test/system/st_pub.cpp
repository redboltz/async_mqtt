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

BOOST_AUTO_TEST_SUITE(st_sub)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(v311_pub_to_broker) {
    broker_runner br;
    as::io_context ioc;
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
                ep().set_auto_pub_response(true);

                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().async_send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0,      // keep_alive none
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().async_recv(*this);
                BOOST_TEST(pv->get_if<am::v3_1_1::connack_packet>());

                // publish QoS0
                yield ep().async_send(
                    am::v3_1_1::publish_packet{
                        "topic1",
                        "payload1",
                        am::qos::at_most_once | am::pub::retain::no | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish QoS1
                pid = *ep().acquire_unique_packet_id();
                yield ep().async_send(
                    am::v3_1_1::publish_packet{
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!*se);
                // recv puback
                yield ep().async_recv(*this);
                BOOST_TEST(*pv == am::v3_1_1::puback_packet{pid});

                // publish QoS2
                pid = *ep().acquire_unique_packet_id();
                yield ep().async_send(
                    am::v3_1_1::publish_packet{
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::yes
                    },
                    *this
                );
                BOOST_TEST(!*se);
                // recv pubrec
                yield ep().async_recv(*this);
                BOOST_TEST(*pv == am::v3_1_1::pubrec_packet{pid});
                // recv pubcomp
                yield ep().async_recv(*this);
                BOOST_TEST(*pv == am::v3_1_1::pubcomp_packet{pid});
                yield ep().async_close(*this);
                yield set_finish();
            }
        }
        am::packet_id_type pid;
    };

    tc t{*amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_pub_to_broker) {
    broker_runner br;
    as::io_context ioc;
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
                ep().set_auto_pub_response(true);

                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep().async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0,      // keep_alive none
                        "cid1",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().async_recv(*this);
                BOOST_TEST(pv->get_if<am::v5::connack_packet>());

                // publish QoS0
                yield ep().async_send(
                    am::v5::publish_packet{
                        0, // packet_id
                        "topic1",
                        "payload1",
                        am::qos::at_most_once | am::pub::retain::no | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish QoS1
                pid = *ep().acquire_unique_packet_id();
                yield ep().async_send(
                    am::v5::publish_packet{
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::no
                    },
                    *this
                );
                BOOST_TEST(!*se);
                // recv puback
                yield ep().async_recv(*this);
                BOOST_TEST(*pv == (am::v5::puback_packet{pid, am::puback_reason_code::no_matching_subscribers}));

                // publish QoS2
                pid = *ep().acquire_unique_packet_id();
                yield ep().async_send(
                    am::v5::publish_packet{
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::exactly_once | am::pub::retain::no | am::pub::dup::yes
                    },
                    *this
                );
                BOOST_TEST(!*se);
                // recv pubrec
                yield ep().async_recv(*this);
                BOOST_CHECK(
                    *pv == (am::v5::pubrec_packet{pid, am::pubrec_reason_code::no_matching_subscribers}) ||
                    *pv == (am::v5::pubrec_packet{pid})
                );
                // recv pubcomp
                yield ep().async_recv(*this);
                BOOST_TEST(*pv == am::v5::pubcomp_packet{pid});
                yield ep().async_close(*this);
                yield set_finish();
            }
        }
        am::packet_id_type pid;
    };

    tc t{*amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_from_broker) {
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    broker_runner br;
    as::io_context ioc_pub;
    as::io_context ioc_sub;
    static auto guard_pub{as::make_work_guard(ioc_pub.get_executor())};
    static auto guard_sub{as::make_work_guard(ioc_sub.get_executor())};
    std::shared_ptr<ep_t> amep_pub = ep_t::create(
        am::protocol_version::v3_1_1,
        ioc_pub.get_executor()
    );
    std::shared_ptr<ep_t> amep_sub = ep_t::create(
        am::protocol_version::v3_1_1,
        ioc_sub.get_executor()
    );

    std::thread th_pub {
        [&] {
            struct tc : coro_base<ep_t> {
                using coro_base<ep_t>::coro_base;
            private:
                void proc(
                    std::optional<am::error_code> /*ec*/,
                    std::optional<am::system_error> se,
                    std::optional<am::packet_variant> pv,
                    std::optional<am::packet_id_type> /*pid*/
                ) override {
                    reenter(this) {
                        // wait for subscribe endpoint becomes ready
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        yield am::async_underlying_handshake(
                            ep().next_layer(),
                            "127.0.0.1",
                            "1883",
                            *this
                        );
                        ep().set_auto_pub_response(true);
                        yield ep().async_send(
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
                        BOOST_TEST(!*se);
                        yield ep().async_recv(*this);
                        BOOST_TEST(pv->get_if<am::v3_1_1::connack_packet>());

                        // publish QoS0
                        yield ep().async_send(
                            am::v3_1_1::publish_packet{
                                "topic1",
                                "payload1",
                                am::qos::at_most_once
                            },
                            *this
                        );
                        BOOST_TEST(!*se);

                        // publish QoS1
                        yield ep().async_send(
                            am::v3_1_1::publish_packet{
                                *ep().acquire_unique_packet_id(),
                                "topic1",
                                "payload2",
                                am::qos::at_least_once
                            },
                            *this
                        );
                        BOOST_TEST(!*se);

                        // publish QoS2
                        yield ep().async_send(
                            am::v3_1_1::publish_packet{
                                *ep().acquire_unique_packet_id(),
                                "topic1",
                                "payload3",
                                am::qos::exactly_once
                            },
                            *this
                        );
                        BOOST_TEST(!*se);

                        yield ep().async_recv(*this); // recv puback
                        yield ep().async_recv(*this); // recv pubrec

                        // wait for broker delivers publish packets.
                        // to adjust expected recv packet_id
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));

                        yield ep().async_close(*this);
                        guard_pub.reset();
                    }
                }
            };
            tc t{*amep_pub, "127.0.0.1", 1883};
            t();
            ioc_pub.run();
        }
    };
    std::thread th_sub {
        [&] {
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
                        ep().set_auto_pub_response(true);

                        // connect sub
                        yield am::async_underlying_handshake(
                            ep().next_layer(),
                            "127.0.0.1",
                            "1883",
                            *this
                        );
                        BOOST_TEST(*ec == am::error_code{});
                        yield ep().async_send(
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

                        yield ep().async_send(
                            am::v3_1_1::subscribe_packet{
                                *ep().acquire_unique_packet_id(),
                                {
                                    {"topic1", am::qos::exactly_once},
                                }
                            },
                            *this
                        );
                        BOOST_TEST(!*se);
                        yield ep().async_recv(*this);
                        BOOST_TEST(pv->get_if<am::v3_1_1::suback_packet>());

                        // wait for broker delivers publish packets.
                        // to adjust expected recv packet_id
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));

                        yield ep().async_recv(*this);
                        BOOST_TEST(
                            *pv
                            ==
                            (am::v3_1_1::publish_packet{
                                "topic1",
                                "payload1",
                                am::qos::at_most_once
                            })
                        );
                        yield ep().async_recv(*this);
                        BOOST_TEST(
                            *pv
                            ==
                            (am::v3_1_1::publish_packet{
                                1,
                                "topic1",
                                "payload2",
                                am::qos::at_least_once
                            })
                        );
                        yield ep().async_recv(*this);
                        BOOST_TEST(
                            *pv
                            ==
                            (am::v3_1_1::publish_packet{
                                2,
                                "topic1",
                                "payload3",
                                am::qos::exactly_once
                            })
                        );

                        yield ep().async_recv(*this);
                        BOOST_TEST(pv->get_if<am::v3_1_1::pubrel_packet>());
                        yield ep().async_send(am::v3_1_1::disconnect_packet{}, *this);
                        BOOST_TEST(!*se);
                        yield ep().async_recv(am::filter::match, {}, *this);
                        BOOST_TEST(!*pv); // wait and check close by broker

                        guard_sub.reset();
                    }
                }
            };
            tc t{*amep_sub, "127.0.0.1", 1883};
            t();
            ioc_sub.run();
        }
    };
    th_sub.join();
    th_pub.join();
}

BOOST_AUTO_TEST_CASE(v5_from_broker) {
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    broker_runner br;
    as::io_context ioc_pub;
    as::io_context ioc_sub;
    static auto guard_pub{as::make_work_guard(ioc_pub.get_executor())};
    static auto guard_sub{as::make_work_guard(ioc_sub.get_executor())};
    std::shared_ptr<ep_t> amep_pub = ep_t::create(
        am::protocol_version::v5,
        ioc_pub.get_executor()
    );
    std::shared_ptr<ep_t> amep_sub = ep_t::create(
        am::protocol_version::v5,
        ioc_sub.get_executor()
    );

    std::thread th_pub {
        [&] {
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
                        ep().set_auto_pub_response(true);
                        // wait for subscribe endpoint becomes ready
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        yield am::async_underlying_handshake(
                            ep().next_layer(),
                            "127.0.0.1",
                            "1883",
                            *this
                        );
                        BOOST_TEST(*ec == am::error_code{});
                        yield ep().async_send(
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
                        BOOST_TEST(!*se);
                        yield ep().async_recv(*this);
                        BOOST_TEST(pv->get_if<am::v5::connack_packet>());

                        // publish QoS0
                        yield ep().async_send(
                            am::v5::publish_packet{
                                "topic1",
                                "payload1",
                                am::qos::at_most_once
                            },
                            *this
                        );
                        BOOST_TEST(!*se);

                        // publish QoS1
                        yield ep().async_send(
                            am::v5::publish_packet{
                                *ep().acquire_unique_packet_id(),
                                "topic1",
                                "payload2",
                                am::qos::at_least_once
                            },
                            *this
                        );
                        BOOST_TEST(!*se);

                        // publish QoS2
                        yield ep().async_send(
                            am::v5::publish_packet{
                                *ep().acquire_unique_packet_id(),
                                "topic1",
                                "payload3",
                                am::qos::exactly_once
                            },
                            *this
                        );
                        BOOST_TEST(!*se);

                        yield ep().async_recv(*this); // recv puback
                        yield ep().async_recv(*this); // recv pubrec

                        // wait for broker delivers publish packets.
                        // to adjust expected recv packet_id
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));

                        yield ep().async_close(*this);
                        guard_pub.reset();
                    }
                }
            };
            tc t{*amep_pub, "127.0.0.1", 1883};
            t();
            ioc_pub.run();
        }
    };
    std::thread th_sub {
        [&] {
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
                        ep().set_auto_pub_response(true);

                        // connect sub
                        yield am::async_underlying_handshake(
                            ep().next_layer(),
                            "127.0.0.1",
                            "1883",
                            *this
                        );
                        BOOST_TEST(*ec == am::error_code{});
                        yield ep().async_send(
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
                        BOOST_TEST(!*se);
                        yield ep().async_recv(*this);
                        pv->visit(
                            am::overload {
                                [&](am::v5::connack_packet const& p) {
                                    BOOST_TEST(!p.session_present());
                                },
                                [](auto const&) {
                                    BOOST_TEST(false);
                                }
                           }
                        );

                        yield ep().async_send(
                            am::v5::subscribe_packet{
                                *ep().acquire_unique_packet_id(),
                                {
                                    {"topic1", am::qos::exactly_once},
                                }
                            },
                            *this
                        );
                        BOOST_TEST(!*se);
                        yield ep().async_recv(*this);
                        BOOST_TEST(pv->get_if<am::v5::suback_packet>());

                        // wait for broker delivers publish packets.
                        // to adjust expected recv packet_id
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));

                        yield ep().async_recv(*this);
                        BOOST_TEST(
                            *pv
                            ==
                            (am::v5::publish_packet{
                                "topic1",
                                "payload1",
                                am::qos::at_most_once
                            })
                        );
                        yield ep().async_recv(*this);
                        BOOST_TEST(
                            *pv
                            ==
                            (am::v5::publish_packet{
                                1,
                                "topic1",
                                "payload2",
                                am::qos::at_least_once
                            })
                        );
                        yield ep().async_recv(*this);
                        BOOST_TEST(
                            *pv
                            ==
                            (am::v5::publish_packet{
                                2,
                                "topic1",
                                "payload3",
                                am::qos::exactly_once
                            })
                        );

                        yield ep().async_recv(*this);
                        BOOST_TEST(pv->get_if<am::v5::pubrel_packet>());
                        yield ep().async_send(am::v5::disconnect_packet{}, *this);
                        BOOST_TEST(!*se);
                        yield ep().async_recv(am::filter::match, {}, *this);
                        BOOST_TEST(!*pv); // wait and check close by broker

                        guard_sub.reset();
                    }
                }
            };
            tc t{*amep_sub, "127.0.0.1", 1883};
            t();
            ioc_sub.run();
        }
    };
    th_sub.join();
    th_pub.join();
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
