// Copyright Takatoshi Kondo 2024
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

BOOST_AUTO_TEST_SUITE(st_order)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(v311_qos0) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t::create(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );
    auto amep_sub = ep_t::create(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            pub,
            sub
        };
        void proc(
            am::optional<am::error_code> ec,
            am::optional<am::system_error> se,
            am::optional<am::packet_variant> pv,
            am::optional<packet_id_t> /*pid*/
        ) override {
            reenter(this) {
                ep(pub).set_auto_pub_response(true);
                ep(sub).set_auto_pub_response(true);
                // connect sub
                yield ep(sub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub).send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0, // keep_alive
                        am::allocate_buffer("sub"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
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

                yield ep(sub).send(
                    am::v3_1_1::subscribe_packet{
                        *ep(sub).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("topic1"), am::qos::exactly_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
                BOOST_TEST(pv->get_if<am::v3_1_1::suback_packet>());


                // connect pub
                yield ep(pub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(pub).send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
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
                BOOST_TEST(pv->get_if<am::v3_1_1::connack_packet>());

                // publish QoS0
                count = 0;
                while (true) {
                    yield ep(pub).send(
                        am::v3_1_1::publish_packet{
                            am::allocate_buffer("topic1"),
                            am::allocate_buffer("payload" + std::to_string(++count)),
                            am::qos::at_most_once
                        },
                        *this
                    );
                    BOOST_TEST(!*se);
                    if (count == 100) break;
                }

                // recv publish QoS0
                count = 0;
                while (true) {
                    yield ep(sub).recv(*this);
                    BOOST_TEST(
                        *pv
                        ==
                        (am::v3_1_1::publish_packet{
                            am::allocate_buffer("topic1"),
                            am::allocate_buffer("payload" + std::to_string(++count)),
                            am::qos::at_most_once
                        })
                    );
                    if (count == 100) break;
                }
                yield ep(pub).close(*this);
                yield ep(sub).close(*this);
                yield set_finish();
            }
        }

        int count;
    };

    tc t{{*amep_pub, *amep_sub}, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_qos1) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t::create(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );
    auto amep_sub = ep_t::create(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            pub,
            sub
        };
        void proc(
            am::optional<am::error_code> ec,
            am::optional<am::system_error> se,
            am::optional<am::packet_variant> pv,
            am::optional<packet_id_t> /*pid*/
        ) override {
            reenter(this) {
                ep(pub).set_auto_pub_response(true);
                ep(sub).set_auto_pub_response(true);
                // connect sub
                yield ep(sub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub).send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0, // keep_alive
                        am::allocate_buffer("sub"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
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

                yield ep(sub).send(
                    am::v3_1_1::subscribe_packet{
                        *ep(sub).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("topic1"), am::qos::exactly_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
                BOOST_TEST(pv->get_if<am::v3_1_1::suback_packet>());


                // connect pub
                yield ep(pub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(pub).send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
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
                BOOST_TEST(pv->get_if<am::v3_1_1::connack_packet>());

                // publish QoS1
                count = 0;
                while (true) {
                    yield ep(pub).send(
                        am::v3_1_1::publish_packet{
                            *ep(pub).acquire_unique_packet_id(),
                            am::allocate_buffer("topic1"),
                            am::allocate_buffer("payload" + std::to_string(++count)),
                            am::qos::at_least_once
                        },
                        *this
                    );
                    BOOST_TEST(!*se);
                    yield ep(pub).recv(*this); // recv puback
                    if (count == 100) break;
                }

                // recv publish QoS1
                count = 0;
                while (true) {
                    yield ep(sub).recv(*this);
                    BOOST_ASSERT(pv->get_if<am::v3_1_1::publish_packet>());
                    BOOST_TEST(pv->get_if<am::v3_1_1::publish_packet>()->opts().get_qos() == am::qos::at_least_once);
                    BOOST_TEST(
                        am::to_string(pv->get_if<am::v3_1_1::publish_packet>()->payload())
                        ==
                        "payload" + std::to_string(++count)
                    );
                    if (count == 100) break;
                }
                yield ep(pub).close(*this);
                yield ep(sub).close(*this);
                yield set_finish();
            }
        }

        int count;
    };

    tc t{{*amep_pub, *amep_sub}, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v311_qos2) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t::create(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );
    auto amep_sub = ep_t::create(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            pub,
            sub
        };
        void proc(
            am::optional<am::error_code> ec,
            am::optional<am::system_error> se,
            am::optional<am::packet_variant> pv,
            am::optional<packet_id_t> /*pid*/
        ) override {
            reenter(this) {
                ep(pub).set_auto_pub_response(true);
                ep(sub).set_auto_pub_response(true);
                // connect sub
                yield ep(sub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub).send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0, // keep_alive
                        am::allocate_buffer("sub"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
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

                yield ep(sub).send(
                    am::v3_1_1::subscribe_packet{
                        *ep(sub).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("topic1"), am::qos::exactly_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
                BOOST_TEST(pv->get_if<am::v3_1_1::suback_packet>());


                // connect pub
                yield ep(pub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(pub).send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
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
                BOOST_TEST(pv->get_if<am::v3_1_1::connack_packet>());

                // publish QoS2
                count = 0;
                while (true) {
                    yield ep(pub).send(
                        am::v3_1_1::publish_packet{
                            *ep(pub).acquire_unique_packet_id(),
                            am::allocate_buffer("topic1"),
                            am::allocate_buffer("payload" + std::to_string(++count)),
                            am::qos::exactly_once
                        },
                        *this
                    );
                    BOOST_TEST(!*se);
                    yield ep(pub).recv(*this); // recv pubrec
                    if (count == 100) break;
                }

                // recv publish QoS2
                count = 0;
                while (true) {
                    yield ep(sub).recv(am::filter::match, {am::control_packet_type::publish}, *this);
                    BOOST_ASSERT(pv->get_if<am::v3_1_1::publish_packet>());
                    BOOST_TEST(pv->get_if<am::v3_1_1::publish_packet>()->opts().get_qos() == am::qos::exactly_once);
                    BOOST_TEST(
                        am::to_string(pv->get_if<am::v3_1_1::publish_packet>()->payload())
                        ==
                        "payload" + std::to_string(++count)
                    );
                    if (count == 100) break;
                }
                yield ep(pub).close(*this);
                yield ep(sub).close(*this);
                yield set_finish();
            }
        }

        int count;
    };

    tc t{{*amep_pub, *amep_sub}, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_qos0) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t::create(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );
    auto amep_sub = ep_t::create(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            pub,
            sub
        };
        void proc(
            am::optional<am::error_code> ec,
            am::optional<am::system_error> se,
            am::optional<am::packet_variant> pv,
            am::optional<packet_id_t> /*pid*/
        ) override {
            reenter(this) {
                ep(pub).set_auto_pub_response(true);
                ep(sub).set_auto_pub_response(true);
                // connect sub
                yield ep(sub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub).send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0, // keep_alive
                        am::allocate_buffer("sub"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
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

                yield ep(sub).send(
                    am::v3_1_1::subscribe_packet{
                        *ep(sub).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("topic1"), am::qos::exactly_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
                BOOST_TEST(pv->get_if<am::v3_1_1::suback_packet>());


                // connect pub
                yield ep(pub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(pub).send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
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
                BOOST_TEST(pv->get_if<am::v3_1_1::connack_packet>());

                // publish QoS0
                count = 0;
                while (true) {
                    yield ep(pub).send(
                        am::v3_1_1::publish_packet{
                            am::allocate_buffer("topic1"),
                            am::allocate_buffer("payload" + std::to_string(++count)),
                            am::qos::at_most_once
                        },
                        *this
                    );
                    BOOST_TEST(!*se);
                    if (count == 100) break;
                }

                // recv publish QoS0
                count = 0;
                while (true) {
                    yield ep(sub).recv(*this);
                    BOOST_TEST(
                        *pv
                        ==
                        (am::v3_1_1::publish_packet{
                            am::allocate_buffer("topic1"),
                            am::allocate_buffer("payload" + std::to_string(++count)),
                            am::qos::at_most_once
                        })
                    );
                    if (count == 100) break;
                }
                yield ep(pub).close(*this);
                yield ep(sub).close(*this);
                yield set_finish();
            }
        }

        int count;
    };

    tc t{{*amep_pub, *amep_sub}, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_qos1) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t::create(
        am::protocol_version::v5,
        ioc.get_executor()
    );
    auto amep_sub = ep_t::create(
        am::protocol_version::v5,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            pub,
            sub
        };
        void proc(
            am::optional<am::error_code> ec,
            am::optional<am::system_error> se,
            am::optional<am::packet_variant> pv,
            am::optional<packet_id_t> /*pid*/
        ) override {
            reenter(this) {
                ep(pub).set_auto_pub_response(true);
                ep(sub).set_auto_pub_response(true);
                // connect sub
                yield ep(sub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub).send(
                    am::v5::connect_packet{
                        true,   // clean_session
                        0, // keep_alive
                        am::allocate_buffer("sub"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
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

                yield ep(sub).send(
                    am::v5::subscribe_packet{
                        *ep(sub).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("topic1"), am::qos::exactly_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::suback_packet>());


                // connect pub
                yield ep(pub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(pub).send(
                    am::v5::connect_packet{
                        true,   // clean_session
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

                // publish QoS1
                count = 0;
                while (true) {
                    yield ep(pub).send(
                        am::v5::publish_packet{
                            *ep(pub).acquire_unique_packet_id(),
                            am::allocate_buffer("topic1"),
                            am::allocate_buffer("payload" + std::to_string(++count)),
                            am::qos::at_least_once
                        },
                        *this
                    );
                    BOOST_TEST(!*se);
                    yield ep(pub).recv(*this); // recv puback
                    if (count == 100) break;
                }

                // recv publish QoS1
                count = 0;
                while (true) {
                    yield ep(sub).recv(*this);
                    BOOST_ASSERT(pv->get_if<am::v5::publish_packet>());
                    BOOST_TEST(pv->get_if<am::v5::publish_packet>()->opts().get_qos() == am::qos::at_least_once);
                    BOOST_TEST(
                        am::to_string(pv->get_if<am::v5::publish_packet>()->payload())
                        ==
                        "payload" + std::to_string(++count)
                    );
                    if (count == 100) break;
                }
                yield ep(pub).close(*this);
                yield ep(sub).close(*this);
                yield set_finish();
            }
        }

        int count;
    };

    tc t{{*amep_pub, *amep_sub}, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_CASE(v5_qos2) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep_pub = ep_t::create(
        am::protocol_version::v5,
        ioc.get_executor()
    );
    auto amep_sub = ep_t::create(
        am::protocol_version::v5,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        enum : std::size_t {
            pub,
            sub
        };
        void proc(
            am::optional<am::error_code> ec,
            am::optional<am::system_error> se,
            am::optional<am::packet_variant> pv,
            am::optional<packet_id_t> /*pid*/
        ) override {
            reenter(this) {
                ep(pub).set_auto_pub_response(true);
                ep(sub).set_auto_pub_response(true);
                // connect sub
                yield ep(sub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(sub).send(
                    am::v5::connect_packet{
                        true,   // clean_session
                        0, // keep_alive
                        am::allocate_buffer("sub"),
                        am::nullopt, // will
                        am::allocate_buffer("u1"),
                        am::allocate_buffer("passforu1")
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
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

                yield ep(sub).send(
                    am::v5::subscribe_packet{
                        *ep(sub).acquire_unique_packet_id(),
                        {
                            {am::allocate_buffer("topic1"), am::qos::exactly_once},
                        }
                    },
                    *this
                );
                BOOST_TEST(!*se);
                yield ep(sub).recv(*this);
                BOOST_TEST(pv->get_if<am::v5::suback_packet>());


                // connect pub
                yield ep(pub).next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield ep(pub).send(
                    am::v5::connect_packet{
                        true,   // clean_session
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

                // publish QoS2
                count = 0;
                while (true) {
                    yield ep(pub).send(
                        am::v5::publish_packet{
                            *ep(pub).acquire_unique_packet_id(),
                            am::allocate_buffer("topic1"),
                            am::allocate_buffer("payload" + std::to_string(++count)),
                            am::qos::exactly_once
                        },
                        *this
                    );
                    BOOST_TEST(!*se);
                    yield ep(pub).recv(*this); // recv pubrec
                    if (count == 100) break;
                }

                // recv publish QoS2
                count = 0;
                while (true) {
                    yield ep(sub).recv(am::filter::match, {am::control_packet_type::publish}, *this);
                    BOOST_ASSERT(pv->get_if<am::v5::publish_packet>());
                    BOOST_TEST(pv->get_if<am::v5::publish_packet>()->opts().get_qos() == am::qos::exactly_once);
                    BOOST_TEST(
                        am::to_string(pv->get_if<am::v5::publish_packet>()->payload())
                        ==
                        "payload" + std::to_string(++count)
                    );
                    if (count == 100) break;
                }
                yield ep(pub).close(*this);
                yield ep(sub).close(*this);
                yield set_finish();
            }
        }

        int count;
    };

    tc t{{*amep_pub, *amep_sub}, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}

BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
