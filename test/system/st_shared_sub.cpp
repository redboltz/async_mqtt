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
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t{
        am::protocol_version::v5,
        ioc.get_executor()
    };
    auto amep_sub1 = ep_t{
        am::protocol_version::v5,
        ioc.get_executor()
    };
    auto amep_sub2 = ep_t{
        am::protocol_version::v5,
        ioc.get_executor()
    };
    auto amep_sub3 = ep_t{
        am::protocol_version::v5,
        ioc.get_executor()
    };

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
            am::error_code ec,
            std::optional<am::packet_variant> pv_opt,
            am::packet_id_type /*pid*/
        ) override {
            reenter(this) {
                ep(pub).set_auto_pub_response(true);
                ep(sub1).set_auto_pub_response(true);
                ep(sub2).set_auto_pub_response(true);
                ep(sub3).set_auto_pub_response(true);

                // connect sub1
                yield as::dispatch(
                    as::bind_executor(
                        ep(sub1).get_executor(),
                        *this
                    )
                );
                yield ep(sub1).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(sub1).async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        "sub1",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub1).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::connack_packet>());
                yield as::dispatch(
                    as::bind_executor(
                        ep(sub1).get_executor(),
                        *this
                    )
                );
                yield ep(sub1).async_send(
                    am::v5::subscribe_packet{
                        *ep(sub1).acquire_unique_packet_id(),
                        {
                            {"$share/sn1/topic1", am::qos::at_most_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub1).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::suback_packet>());

                // connect sub2
                yield ep(sub2).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub2).get_executor(),
                        *this
                    )
                );
                yield ep(sub2).async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        "sub2",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub2).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::connack_packet>());
                yield as::dispatch(
                    as::bind_executor(
                        ep(sub2).get_executor(),
                        *this
                    )
                );
                yield ep(sub2).async_send(
                    am::v5::subscribe_packet{
                        *ep(sub2).acquire_unique_packet_id(),
                        {
                            {"$share/sn1/topic1", am::qos::at_least_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub2).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::suback_packet>());

                // connect sub3
                yield as::dispatch(
                    as::bind_executor(
                        ep(sub3).get_executor(),
                        *this
                    )
                );
                yield ep(sub3).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(sub3).async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        "sub3",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub3).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::connack_packet>());

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub3).get_executor(),
                        *this
                    )
                );
                yield ep(sub3).async_send(
                    am::v5::subscribe_packet{
                        *ep(sub3).acquire_unique_packet_id(),
                        {
                            {"$share/sn1/topic1", am::qos::exactly_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub3).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::suback_packet>());

                // connect pub
                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );
                yield ep(pub).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(pub).async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        "pub",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::connack_packet>());

                // publish 1
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        "topic1",
                        "payload1",
                        am::qos::at_most_once
                    },
                    *this
                );
                BOOST_TEST(!ec);

                // publish 2
                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload2",
                        am::qos::at_least_once
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(*this); // recv puback

                // publish 3
                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload3",
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(*this); // recv pubrec


                // publish 4
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        "topic1",
                        "payload4",
                        am::qos::at_most_once
                    },
                    *this
                );
                BOOST_TEST(!ec);

                // publish 5
                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload5",
                        am::qos::at_least_once
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(*this); // recv puback

                // publish 6
                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload6",
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(*this); // recv pubrec

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub1).get_executor(),
                        *this
                    )
                );
                yield ep(sub1).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        "topic1",
                        "payload1",
                        am::qos::at_most_once
                    })
                );

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub2).get_executor(),
                        *this
                    )
                );
                yield ep(sub2).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        1,
                        "topic1",
                        "payload2",
                        am::qos::at_least_once
                    })
                );

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub3).get_executor(),
                        *this
                    )
                );
                yield ep(sub3).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        1,
                        "topic1",
                        "payload3",
                        am::qos::exactly_once
                    })
                );

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub1).get_executor(),
                        *this
                    )
                );
                yield ep(sub1).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        "topic1",
                        "payload4",
                        am::qos::at_most_once
                    })
                );

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub2).get_executor(),
                        *this
                    )
                );
                yield ep(sub2).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        2,
                        "topic1",
                        "payload5",
                        am::qos::at_least_once
                    })
                );

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub3).get_executor(),
                        *this
                    )
                );
                yield ep(sub3).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        2,
                        "topic1",
                        "payload6",
                        am::qos::exactly_once
                    })
                );


                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );
                yield ep(pub).async_close(*this);

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub1).get_executor(),
                        *this
                    )
                );
                yield ep(sub1).async_close(*this);

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub2).get_executor(),
                        *this
                    )
                );
                yield ep(sub2).async_close(*this);

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub3).get_executor(),
                        *this
                    )
                );
                yield ep(sub3).async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{{amep_pub, amep_sub1, amep_sub2, amep_sub3}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_unsub_from_broker) {
    broker_runner br;
    as::io_context ioc;
    static auto guard{as::make_work_guard(ioc.get_executor())};
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t{
        am::protocol_version::v5,
        ioc.get_executor()
    };
    auto amep_sub1 = ep_t{
        am::protocol_version::v5,
        ioc.get_executor()
    };
    auto amep_sub2 = ep_t{
        am::protocol_version::v5,
        ioc.get_executor()
    };
    auto amep_sub3 = ep_t{
        am::protocol_version::v5,
        ioc.get_executor()
    };

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
            am::error_code ec,
            std::optional<am::packet_variant> pv_opt,
            am::packet_id_type pid
        ) override {
            reenter(this) {
                ep(pub).set_auto_pub_response(true);
                ep(sub1).set_auto_pub_response(true);
                ep(sub2).set_auto_pub_response(true);
                ep(sub3).set_auto_pub_response(true);

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub1).get_executor(),
                        *this
                    )
                );

                // connect sub1
                yield ep(sub1).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(sub1).async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        "sub1",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub1).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::connack_packet>());
                yield ep(sub1).async_send(
                    am::v5::subscribe_packet{
                        *ep(sub1).acquire_unique_packet_id(),
                        {
                            {"$share/sn1/topic1", am::qos::at_most_once},
                        },
                        {am::property::subscription_identifier{1}}
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub1).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::suback_packet>());

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub2).get_executor(),
                        *this
                    )
                );

                // connect sub2
                yield ep(sub2).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(sub2).async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        "sub2",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub2).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::connack_packet>());
                yield ep(sub2).async_send(
                    am::v5::subscribe_packet{
                        *ep(sub2).acquire_unique_packet_id(),
                        {
                            {"$share/sn1/topic1", am::qos::at_least_once},
                        },
                        {am::property::subscription_identifier{1}}
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub2).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::suback_packet>());

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub3).get_executor(),
                        *this
                    )
                );

                // connect sub3
                yield ep(sub3).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(sub3).async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        "sub3",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub3).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::connack_packet>());
                yield ep(sub3).async_send(
                    am::v5::subscribe_packet{
                        *ep(sub3).acquire_unique_packet_id(),
                        {
                            {"$share/sn1/topic1", am::qos::exactly_once},
                        },
                        {am::property::subscription_identifier{10}}
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub3).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::suback_packet>());

                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );

                // connect pub
                yield ep(pub).async_underlying_handshake(
                    "127.0.0.1",
                    "1883",
                    *this
                );
                BOOST_TEST(ec == am::error_code{});
                yield ep(pub).async_send(
                    am::v5::connect_packet{
                        true,   // clean_start
                        0, // keep_alive
                        "pub",
                        std::nullopt, // will
                        "u1",
                        "passforu1"
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::connack_packet>());

                // publish 1
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        "topic1",
                        "payload1",
                        am::qos::at_most_once
                    },
                    *this
                );
                BOOST_TEST(!ec);

                // publish 2
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload2",
                        am::qos::at_least_once
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(*this); // recv puback

                // publish 3
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload3",
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(*this); // recv pubrec


                yield as::dispatch(
                    as::bind_executor(
                        ep(sub1).get_executor(),
                        *this
                    )
                );

                yield ep(sub1).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        "topic1",
                        "payload1",
                        am::qos::at_most_once,
                        {am::property::subscription_identifier{1}}
                    })
                );

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub2).get_executor(),
                        *this
                    )
                );

                yield ep(sub2).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        1,
                        "topic1",
                        "payload2",
                        am::qos::at_least_once,
                        {am::property::subscription_identifier{1}}
                    })
                );

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub3).get_executor(),
                        *this
                    )
                );
                yield ep(sub3).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        1,
                        "topic1",
                        "payload3",
                        am::qos::exactly_once,
                        {am::property::subscription_identifier{10}}
                    })
                );

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub1).get_executor(),
                        *this
                    )
                );

                // unsub
                yield ep(sub1).async_acquire_unique_packet_id(*this);
                BOOST_TEST(!ec);
                yield ep(sub1).async_send(
                    am::v5::unsubscribe_packet{
                        pid,
                        {
                            "$share/sn1/topic1"
                        }
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(sub1).async_recv(*this);
                BOOST_TEST(pv_opt->get_if<am::v5::unsuback_packet>());

                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );

                // publish 4
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        "topic1",
                        "payload4",
                        am::qos::at_most_once
                    },
                    *this
                );
                BOOST_TEST(!ec);

                // publish 5
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload5",
                        am::qos::at_least_once
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(*this); // recv puback

                // publish 6
                yield ep(pub).async_send(
                    am::v5::publish_packet{
                        *ep(pub).acquire_unique_packet_id(),
                        "topic1",
                        "payload6",
                        am::qos::exactly_once
                    },
                    *this
                );
                BOOST_TEST(!ec);
                yield ep(pub).async_recv(*this); // recv pubrec

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub2).get_executor(),
                        *this
                    )
                );

                yield ep(sub2).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        "topic1",
                        "payload4",
                        am::qos::at_most_once,
                        {am::property::subscription_identifier{1}}
                    })
                );

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub3).get_executor(),
                        *this
                    )
                );

                yield ep(sub3).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        2,
                        "topic1",
                        "payload5",
                        am::qos::at_least_once,
                        {am::property::subscription_identifier{10}}
                    })
                );

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub2).get_executor(),
                        *this
                    )
                );
                yield ep(sub2).async_recv(am::filter::match, {am::control_packet_type::publish}, *this);
                BOOST_TEST(
                    *pv_opt
                    ==
                    (am::v5::publish_packet{
                        1,
                        "topic1",
                        "payload6",
                        am::qos::at_least_once,
                        {am::property::subscription_identifier{1}}
                    })
                );


                yield as::dispatch(
                    as::bind_executor(
                        ep(pub).get_executor(),
                        *this
                    )
                );
                yield ep(pub).async_close(*this);

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub1).get_executor(),
                        *this
                    )
                );
                yield ep(sub1).async_close(*this);

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub2).get_executor(),
                        *this
                    )
                );
                yield ep(sub2).async_close(*this);

                yield as::dispatch(
                    as::bind_executor(
                        ep(sub3).get_executor(),
                        *this
                    )
                );
                yield ep(sub3).async_close(*this);
                set_finish();
                guard.reset();
            }
        }
    };

    tc t{{amep_pub, amep_sub1, amep_sub2, amep_sub3}};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
