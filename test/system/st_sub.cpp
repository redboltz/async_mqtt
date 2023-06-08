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

BOOST_AUTO_TEST_CASE(v311_sub) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep = ep_t(
        am::protocol_version::v3_1_1,
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
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0,      // keep_alive none
                        am::allocate_buffer("cid1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
                BOOST_TEST(pv->get_if<am::v3_1_1::connack_packet>());

                // subscribe
                pid = *ep().acquire_unique_packet_id();
                yield ep().send(
                    am::v3_1_1::subscribe_packet{
                        pid,
                        {
                            {am::allocate_buffer("topic1"), am::qos::at_most_once},
                            {am::allocate_buffer("topic2"), am::qos::at_least_once},
                            {am::allocate_buffer("topic3"), am::qos::exactly_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
                BOOST_TEST(
                    *pv == (am::v3_1_1::suback_packet{
                        pid,
                        {
                            am::suback_return_code::success_maximum_qos_0,
                            am::suback_return_code::success_maximum_qos_1,
                            am::suback_return_code::success_maximum_qos_2,
                        }
                    })
                );

                // subscribe overwrite
                pid = *ep().acquire_unique_packet_id();
                yield ep().send(
                    am::v3_1_1::subscribe_packet{
                        pid,
                        {
                            {am::allocate_buffer("topic1"), am::qos::at_least_once},
                            {am::allocate_buffer("topic2"), am::qos::exactly_once},
                            {am::allocate_buffer("topic3"), am::qos::at_most_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
                BOOST_TEST(
                    *pv == (am::v3_1_1::suback_packet{
                        pid,
                        {
                            am::suback_return_code::success_maximum_qos_1,
                            am::suback_return_code::success_maximum_qos_2,
                            am::suback_return_code::success_maximum_qos_0,
                        }
                    })
                );

                // unsubscribe
                pid = *ep().acquire_unique_packet_id();
                yield ep().send(
                    am::v3_1_1::unsubscribe_packet{
                        pid,
                        {
                            am::allocate_buffer("topic1"),
                            am::allocate_buffer("topic2"),
                            am::allocate_buffer("topic3"),
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
                BOOST_TEST(
                    *pv == am::v3_1_1::unsuback_packet{
                        pid
                    }
                );
                yield ep().close(*this);
                yield set_finish();
            }
        }
        ep_t::packet_id_t pid;
    };

    tc t{amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_sub) {
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
                        true,   // clean_session
                        0,      // keep_alive none
                        am::allocate_buffer("cid1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1"),
                        am::properties{}
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
                BOOST_TEST(pv->get_if<am::v5::connack_packet>());

                // subscribe
                pid = *ep().acquire_unique_packet_id();
                yield ep().send(
                    am::v5::subscribe_packet{
                        pid,
                        {
                            {
                                am::allocate_buffer("topic1"),
                                am::qos::at_most_once |
                                am::sub::retain_handling::send |
                                am::sub::rap::dont |
                                am::sub::nl::no
                            },
                            {
                                am::allocate_buffer("topic2"),
                                am::qos::at_least_once |
                                am::sub::retain_handling::send_only_new_subscription |
                                am::sub::rap::retain |
                                am::sub::nl::yes
                            },
                            {
                                am::allocate_buffer("topic3"),
                                am::qos::exactly_once |
                                am::sub::retain_handling::not_send
                            },
                        },
                        am::properties{}
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
                BOOST_TEST(
                    *pv == (am::v5::suback_packet{
                        pid,
                        {
                            am::suback_reason_code::granted_qos_0,
                            am::suback_reason_code::granted_qos_1,
                            am::suback_reason_code::granted_qos_2,
                        },
                        am::properties{}
                    })
                );

                // subscribe overwrite
                pid = *ep().acquire_unique_packet_id();
                yield ep().send(
                    am::v5::subscribe_packet{
                        pid,
                        {
                            {am::allocate_buffer("topic1"), am::qos::at_least_once},
                            {am::allocate_buffer("topic2"), am::qos::exactly_once},
                            {am::allocate_buffer("topic3"), am::qos::at_most_once},
                        },
                        am::properties{}
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
                BOOST_TEST(
                    *pv == (am::v5::suback_packet{
                        pid,
                        {
                            am::suback_reason_code::granted_qos_1,
                            am::suback_reason_code::granted_qos_2,
                            am::suback_reason_code::granted_qos_0,
                        },
                        am::properties{}
                    })
                );

                // unsubscribe
                pid = *ep().acquire_unique_packet_id();
                yield ep().send(
                    am::v5::unsubscribe_packet{
                        pid,
                        {
                            am::allocate_buffer("topic1"),
                            am::allocate_buffer("topic2"),
                            am::allocate_buffer("topic3"),
                        },
                        am::properties{}
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep().recv(*this);
                BOOST_TEST(
                    *pv == (am::v5::unsuback_packet{
                        pid,
                        {
                            am::unsuback_reason_code::success,
                            am::unsuback_reason_code::success,
                            am::unsuback_reason_code::success,
                        },
                        am::properties{}
                    })
                );
                yield ep().close(*this);
                yield set_finish();
            }
        }
        ep_t::packet_id_t pid;
    };

    tc t{amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
