// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/protocol/rv_connection.hpp>
#include <async_mqtt/protocol/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_connection)

namespace am = async_mqtt;


BOOST_AUTO_TEST_CASE(v5_connect_connack) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};

    auto w = am::will{
        "topic1",
        "payload1",
        am::pub::retain::yes | am::qos::at_least_once,
        am::properties{
            am::property::will_delay_interval(0x0fffffff),
            am::property::content_type("json")
        }
    };

    auto props = am::properties{
        am::property::session_expiry_interval(0x0fffffff),
        am::property::user_property("mykey", "myval")
    };

    auto p = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        w,
        "user1",
        "pass1",
        props
    };

    auto events = c.send(p);
    BOOST_TEST(events.size() == 2);
    std::visit(
        am::overload{
            [&](am::event::send const& ev) {
                BOOST_TEST(!ev.get_release_packet_id_if_send_error());
                BOOST_TEST(ev.get() == p);
            },
            [](auto const&...) {
                BOOST_TEST(false);
            }
        },
        events[0]
    );
    std::visit(
        am::overload{
            [&](am::event::timer const& ev) {
                BOOST_TEST(ev.get_op() == am::timer_op::reset);
                BOOST_TEST(ev.get_kind() == am::timer_kind::pingreq_send);
                BOOST_CHECK(ev.get_ms() == std::chrono::seconds{0x1234});
            },
            [](auto const&...) {
                BOOST_TEST(false);
            }
        },
        events[1]
    );

    char recv_connack[] {
        0x20,       // fixed_header
        0x08,       // remaining_length
        0x01,       // session_present
        char(0x87), // connect_reason_code
        0x05,       // property_length
        0x11, 0x0f, char(0xff), char(0xff), char(0xff), // session_expiry_interval
    };

    std::istringstream is{std::string{recv_connack, sizeof(recv_connack)}};
    events = c.recv(is);
    BOOST_TEST(events.size() == 1);
    {
        auto expected =
            [&] {
                auto props = am::properties{
                    am::property::session_expiry_interval(0x0fffffff)
                };
                return am::v5::connack_packet{
                    true,   // session_present
                    am::connect_reason_code::not_authorized,
                    props
                };
            } ();
        std::visit(
            am::overload{
                [&](am::event::packet_received const& ev) {
                    BOOST_TEST(ev.get() == expected);
                },
                [](auto const&...) {
                    BOOST_TEST(false);
                }
            },
            events[0]
        );
    }
}

BOOST_AUTO_TEST_CASE(v5_non_allocate_pid_fail) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};
    auto connect = am::v5::connect_packet{
        false,   // clean_start
        0, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
        am::properties{
            am::property::session_expiry_interval{am::session_never_expire}
        }
    };

    {
        auto connack = am::v5::connack_packet{
            false,   // session_present
            am::connect_reason_code::success
        };
        c.send(connect);

        auto connack_str{am::to_string(connack.const_buffer_sequence())};
        std::istringstream is{connack_str};
        c.recv(is);
    }

    auto publish = am::v5::publish_packet(
        1,
        "topic1",
        "payload1",
        am::qos::at_least_once
    );
    auto events = c.send(publish);
    BOOST_TEST(events.size() == 1);
    std::visit(
        am::overload {
            [&](am::error_code const& e) {
                BOOST_TEST(e == am::mqtt_error::packet_identifier_invalid);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[0]
    );
}

BOOST_AUTO_TEST_CASE(v5_offline_send_fail) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};
    c.set_offline_publish(false); // default
    auto pid_opt = c.acquire_unique_packet_id();
    auto publish_reg_t1 = am::v5::publish_packet(
        *pid_opt,
        "topic1",
        "payload1",
        am::qos::at_least_once
    );
    auto events = c.send(publish_reg_t1);
    BOOST_TEST(events.size() == 2);
    std::visit(
        am::overload {
            [&](am::error_code const& e) {
                BOOST_TEST(e == am::mqtt_error::packet_not_allowed_to_send);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[0]
    );
    std::visit(
        am::overload {
            [&](am::event::packet_id_released const& ev) {
                BOOST_TEST(ev.get() == *pid_opt);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[1]
    );
}

BOOST_AUTO_TEST_CASE(v5_topic_alias_offline_fail) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};
    c.set_offline_publish(true);
    auto pid_opt = c.acquire_unique_packet_id();
    auto publish_reg_t1 = am::v5::publish_packet(
        *pid_opt,
        "topic1",
        "payload1",
        am::qos::at_least_once,
        am::properties{
            am::property::topic_alias{1}
        }
    );
    auto events = c.send(publish_reg_t1);
    BOOST_TEST(events.size() == 2);
    std::visit(
        am::overload {
            [&](am::error_code const& e) {
                BOOST_TEST(e == am::mqtt_error::packet_not_allowed_to_send);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[0]
    );
    std::visit(
        am::overload {
            [&](am::event::packet_id_released const& ev) {
                BOOST_TEST(ev.get() == *pid_opt);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[1]
    );
}

BOOST_AUTO_TEST_CASE(v5_topic_alias_size_over_resend) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};


    auto connect = am::v5::connect_packet{
        false,   // clean_start
        0, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
        am::properties{
            am::property::session_expiry_interval{am::session_never_expire}
        }
    };

    {
        auto connack = am::v5::connack_packet{
            false,   // session_present
            am::connect_reason_code::success,
            am::properties{
                am::property::maximum_packet_size{33},
                am::property::topic_alias_maximum{0xffff}
            }
        };
        c.send(connect);

        auto connack_str{am::to_string(connack.const_buffer_sequence())};
        std::istringstream is{connack_str};
        c.recv(is);
    }

    auto publish_reg_t1 = am::v5::publish_packet(
        "topic0123456789",
        "payload1",
        am::qos::at_most_once,
        am::properties{
            am::property::topic_alias{1}
        }
    );
    auto pid_opt = c.acquire_unique_packet_id();
    auto publish_use_t1 = am::v5::publish_packet(
        *pid_opt,
        "",
        "payload1",
        am::qos::at_least_once,
        am::properties{
            am::property::topic_alias{1}
        }
    );
    c.send(publish_reg_t1);
    c.send(publish_use_t1);

    c.notify_closed();

    {
        auto connack = am::v5::connack_packet{
            true,   // session_present
            am::connect_reason_code::success,
            am::properties{
                am::property::maximum_packet_size{29},
                am::property::topic_alias_maximum{0xffff}
            }
        };
        c.send(connect);

        auto connack_str{am::to_string(connack.const_buffer_sequence())};
        std::istringstream is{connack_str};
        auto events = c.recv(is);

        BOOST_TEST(events.size() == 2);
        std::visit(
            am::overload {
                [&](am::event::packet_id_released const& ev) {
                    BOOST_TEST(ev.get() == *pid_opt);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            },
            events[0]
        );
        std::visit(
            am::overload {
                [&](am::event::packet_received const& ev) {
                    BOOST_TEST(ev.get() == connack);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            },
            events[1]
        );
    }
}

BOOST_AUTO_TEST_CASE(v5_invalid_suback) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};
    auto connect = am::v5::connect_packet{
        false,   // clean_start
        0, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
    };

    {
        auto connack = am::v5::connack_packet{
            false,   // session_present
            am::connect_reason_code::success
        };
        c.send(connect);

        auto connack_str{am::to_string(connack.const_buffer_sequence())};
        std::istringstream is{connack_str};
        c.recv(is);
    }
    {
        auto suback = am::v5::suback_packet{
            0x1234,         // packet_id
            {
                am::suback_reason_code::granted_qos_0
            }
        };
        auto suback_str{am::to_string(suback.const_buffer_sequence())};
        std::istringstream is{suback_str};

        auto disconnect = am::v5::disconnect_packet{
            am::disconnect_reason_code::protocol_error
        };
        auto events = c.recv(is);
        BOOST_TEST(events.size() == 3);
        std::visit(
            am::overload {
                [&](am::event::send const& ev) {
                    BOOST_TEST(ev.get() == disconnect);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            },
            events[0]
        );
        std::visit(
            am::overload {
                [&](am::event::close const&) {
                    BOOST_TEST(true);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            },
            events[1]
        );
        std::visit(
            am::overload {
                [&](am::error_code const& e) {
                    BOOST_TEST(e == am::disconnect_reason_code::protocol_error);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            },
            events[2]
        );
    }
}

BOOST_AUTO_TEST_CASE(v5_invalid_unsuback) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};
    auto connect = am::v5::connect_packet{
        false,   // clean_start
        0, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
    };

    {
        auto connack = am::v5::connack_packet{
            false,   // session_present
            am::connect_reason_code::success
        };
        c.send(connect);

        auto connack_str{am::to_string(connack.const_buffer_sequence())};
        std::istringstream is{connack_str};
        c.recv(is);
    }
    {
        auto unsuback = am::v5::unsuback_packet(
            0x1234,         // packet_id
            std::vector<am::unsuback_reason_code> {
                am::unsuback_reason_code::no_subscription_existed
            }
        );
        auto unsuback_str{am::to_string(unsuback.const_buffer_sequence())};
        std::istringstream is{unsuback_str};

        auto disconnect = am::v5::disconnect_packet{
            am::disconnect_reason_code::protocol_error
        };
        auto events = c.recv(is);
        BOOST_TEST(events.size() == 3);
        std::visit(
            am::overload {
                [&](am::event::send const& ev) {
                    BOOST_TEST(ev.get() == disconnect);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            },
            events[0]
        );
        std::visit(
            am::overload {
                [&](am::event::close const&) {
                    BOOST_TEST(true);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            },
            events[1]
        );
        std::visit(
            am::overload {
                [&](am::error_code const& e) {
                    BOOST_TEST(e == am::disconnect_reason_code::protocol_error);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            },
            events[2]
        );
    }

}

BOOST_AUTO_TEST_CASE(v5_invalid_connect_recv) {
    am::rv_connection<am::role::server> c{am::protocol_version::v5};
    auto connect = am::v5::connect_packet{
        false,   // clean_start
        0, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
    };
    auto connect_str{am::to_string(connect.const_buffer_sequence())};

    {
        std::istringstream is{connect_str};
        c.recv(is);
    }
    {
        std::istringstream is{connect_str};
        auto events = c.recv(is);
        // recv connect while connecting
        BOOST_TEST(events.size() == 1);
        std::visit(
            am::overload {
                [&](am::error_code const& e) {
                    BOOST_TEST(e == am::disconnect_reason_code::protocol_error);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            },
            events[0]
        );
    }
}

BOOST_AUTO_TEST_CASE(v5_invalid_connack_recv) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};
    auto connack = am::v5::connack_packet{
        false,   // session_present
        am::connect_reason_code::success
    };
    auto connack_str{am::to_string(connack.const_buffer_sequence())};

    std::istringstream is{connack_str};
    auto events = c.recv(is);
    // recv connack while disconnected
    BOOST_TEST(events.size() == 1);
    std::visit(
        am::overload {
            [&](am::error_code const& e) {
                BOOST_TEST(e == am::disconnect_reason_code::protocol_error);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[0]
    );
}

BOOST_AUTO_TEST_CASE(v5_invalid_auth_recv) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};
    auto auth = am::v5::auth_packet{};
    auto auth_str{am::to_string(auth.const_buffer_sequence())};

    std::istringstream is{auth_str};
    auto events = c.recv(is);
    // recv auth while disconnected
    BOOST_TEST(events.size() == 1);
    std::visit(
        am::overload {
            [&](am::error_code const& e) {
                BOOST_TEST(e == am::disconnect_reason_code::protocol_error);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[0]
    );
}

BOOST_AUTO_TEST_CASE(v5_invalid_publish_recv) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};
    auto publish = am::v5::publish_packet(
        "topic1",
        "payload1",
        am::qos::at_most_once
    );
    auto publish_str{am::to_string(publish.const_buffer_sequence())};

    std::istringstream is{publish_str};
    auto events = c.recv(is);
    // recv publish while disconnected
    BOOST_TEST(events.size() == 1);
    std::visit(
        am::overload {
            [&](am::error_code const& e) {
                BOOST_TEST(e == am::disconnect_reason_code::protocol_error);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[0]
    );
}

BOOST_AUTO_TEST_SUITE_END()
