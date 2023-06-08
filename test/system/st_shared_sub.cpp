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

BOOST_AUTO_TEST_SUITE(st_shared_sub)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(v5_from_broker) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t(
        am::protocol_version::v5,
        ioc.get_executor()
    );
    auto amep_sub1 = ep_t(
        am::protocol_version::v5,
        ioc.get_executor()
    );
    auto amep_sub2 = ep_t(
        am::protocol_version::v5,
        ioc.get_executor()
    );
    auto amep_sub3 = ep_t(
        am::protocol_version::v5,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            pub,
            sub1,
            sub2,
            sub3,
        };
        void proc(
            am::optional<am::error_code> ec,
            am::optional<am::system_error> se,
            am::optional<am::packet_variant> pv,
            am::optional<packet_id_t> /*pid*/
        ) override {
            reenter(this) {
                // connect sub1
                yield ep(sub1).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub1).send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("sub1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub1).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::connack_packet>());
                yield ep(sub1).send(
                    am::v5::subscribe_packet{
                        *ep(sub1).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("$share/sn1/topic1"), am::qos::at_most_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub1).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::suback_packet>());

                // connect sub2
                yield ep(sub2).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub2).send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("sub2"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub2).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::connack_packet>());
                yield ep(sub2).send(
                    am::v5::subscribe_packet{
                        *ep(sub2).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("$share/sn1/topic1"), am::qos::at_least_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub2).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::suback_packet>());

                // connect sub3
                yield ep(sub3).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub3).send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("sub3"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub3).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::connack_packet>());
                yield ep(sub3).send(
                    am::v5::subscribe_packet{
                        *ep(sub3).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("$share/sn1/topic1"), am::qos::exactly_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub3).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::suback_packet>());

                // connect pub
                yield ep(pub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(pub).send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("pub"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(pub).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::connack_packet>());

                // publish 1
                yield ep(pub).send(
                    am::v5::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_most_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish 2
                yield ep(pub).send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload2"),
                        am::qos::at_least_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish 3
                yield ep(pub).send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload3"),
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish 4
                yield ep(pub).send(
                    am::v5::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload4"),
                        am::qos::at_most_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish 5
                yield ep(pub).send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload5"),
                        am::qos::at_least_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish 6
                yield ep(pub).send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload6"),
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                yield ep(sub1).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_most_once
                    })
                );
                yield ep(sub2).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        1,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload2"),
                        am::qos::at_least_once
                    })
                );
                yield ep(sub3).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        1,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload3"),
                        am::qos::exactly_once
                    })
                );

                yield ep(sub1).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload4"),
                        am::qos::at_most_once
                    })
                );
                yield ep(sub2).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        2,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload5"),
                        am::qos::at_least_once
                    })
                );
                yield ep(sub3).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        2,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload6"),
                        am::qos::exactly_once
                    })
                );


                yield ep(pub).close(*this);
                yield ep(sub1).close(*this);
                yield ep(sub2).close(*this);
                yield ep(sub3).close(*this);
                yield set_finish();
            }
        }
    };

    tc t{{amep_pub, amep_sub1, amep_sub2, amep_sub3}, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_unsub_from_broker) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t(
        am::protocol_version::v5,
        ioc.get_executor()
    );
    auto amep_sub1 = ep_t(
        am::protocol_version::v5,
        ioc.get_executor()
    );
    auto amep_sub2 = ep_t(
        am::protocol_version::v5,
        ioc.get_executor()
    );
    auto amep_sub3 = ep_t(
        am::protocol_version::v5,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            pub,
            sub1,
            sub2,
            sub3,
        };
        void proc(
            am::optional<am::error_code> ec,
            am::optional<am::system_error> se,
            am::optional<am::packet_variant> pv,
            am::optional<packet_id_t> pid
        ) override {
            reenter(this) {
                // connect sub1
                yield ep(sub1).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub1).send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("sub1"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub1).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::connack_packet>());
                yield ep(sub1).send(
                    am::v5::subscribe_packet{
                        *ep(sub1).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("$share/sn1/topic1"), am::qos::at_most_once},
                        },
                        {am::property::subscription_identifier{1}}
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub1).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::suback_packet>());

                // connect sub2
                yield ep(sub2).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub2).send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("sub2"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub2).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::connack_packet>());
                yield ep(sub2).send(
                    am::v5::subscribe_packet{
                        *ep(sub2).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("$share/sn1/topic1"), am::qos::at_least_once},
                        },
                        {am::property::subscription_identifier{1}}
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub2).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::suback_packet>());

                // connect sub3
                yield ep(sub3).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub3).send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("sub3"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub3).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::connack_packet>());
                yield ep(sub3).send(
                    am::v5::subscribe_packet{
                        *ep(sub3).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("$share/sn1/topic1"), am::qos::exactly_once},
                        },
                        {am::property::subscription_identifier{10}}
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub3).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::suback_packet>());

                // connect pub
                yield ep(pub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(pub).send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        am::allocate_buffer("pub"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(pub).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::connack_packet>());

                // publish 1
                yield ep(pub).send(
                    am::v5::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_most_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish 2
                yield ep(pub).send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload2"),
                        am::qos::at_least_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish 3
                yield ep(pub).send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload3"),
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!*se);


                yield ep(sub1).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_most_once,
                        {am::property::subscription_identifier{1}}
                    })
                );
                yield ep(sub2).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        1,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload2"),
                        am::qos::at_least_once,
                        {am::property::subscription_identifier{1}}
                    })
                );
                yield ep(sub3).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        1,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload3"),
                        am::qos::exactly_once,
                        {am::property::subscription_identifier{10}}
                    })
                );

                // unsub
                yield ep(sub1).acquire_unique_packet_id(*this);
                yield ep(sub1).send(
                    am::v5::unsubscribe_packet{
                        *pid,
                        {
                            am::allocate_buffer("$share/sn1/topic1")
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub1).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::unsuback_packet>());

                // publish 4
                yield ep(pub).send(
                    am::v5::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload4"),
                        am::qos::at_most_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish 5
                yield ep(pub).send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload5"),
                        am::qos::at_least_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                // publish 6
                yield ep(pub).send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload6"),
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!*se);

                yield ep(sub2).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload4"),
                        am::qos::at_most_once,
                        {am::property::subscription_identifier{1}}
                    })
                );
                yield ep(sub3).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        2,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload5"),
                        am::qos::at_least_once,
                        {am::property::subscription_identifier{10}}
                    })
                );
                yield ep(sub2).recv(*this);
                BOOST_TEST(
                    *pv
                    ==
                    (am::v5::publish_packet{
                        2,
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload6"),
                        am::qos::at_least_once,
                        {am::property::subscription_identifier{1}}
                    })
                );


                yield ep(pub).close(*this);
                yield ep(sub1).close(*this);
                yield ep(sub2).close(*this);
                yield ep(sub3).close(*this);
                yield set_finish();
            }
        }
    };

    tc t{{amep_pub, amep_sub1, amep_sub2, amep_sub3}, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
