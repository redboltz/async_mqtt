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

BOOST_AUTO_TEST_CASE(v5_store_after_error) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};

    BOOST_TEST(c.get_connection_status() == am::connection_status::disconnected);
    {
        // CONNECT
        auto p = am::v5::connect_packet{
            true,   // clean_start
            0, // keep_alive
            "cid1",
            std::nullopt,
            std::nullopt,
            std::nullopt,
            am::properties{
                am::property::session_expiry_interval(am::session_never_expire)
            }
        };
        c.send(p);
    }
    BOOST_TEST(c.get_connection_status() == am::connection_status::connecting);
    {
        // CONNACK
        std::istringstream is{
            am::to_string(
                am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                }.const_buffer_sequence()
            )
        };
        c.recv(is);
    }
    BOOST_TEST(c.get_connection_status() == am::connection_status::connected);
    {
        // error packet
        char too_large[] {
            0x01, char(0xff), char(0xff), char(0xff), char(0xff),
        };
        std::istringstream is{std::string(too_large, sizeof(too_large))};
        c.recv(is);
    }
    BOOST_TEST(c.get_connection_status() == am::connection_status::connected);

    // PUBLISH QoS1
    auto pid_opt = c.acquire_unique_packet_id();
    auto publish = am::v5::publish_packet(
        *pid_opt,
        "topic1",
        "payload1",
        am::qos::at_least_once
    );
    auto events = c.send(publish);
    BOOST_TEST(events.size() == 1);
    std::visit(
        am::overload {
            [&](am::event::send const&) {
                BOOST_TEST(true);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[0]
    );
    c.notify_closed();
    BOOST_TEST(c.get_connection_status() == am::connection_status::disconnected);
    auto packets = c.get_stored_packets();
    BOOST_TEST(packets.size() == 1);
    auto dup_publish = publish;
    dup_publish.set_dup(true);
    packets[0].visit(
        am::overload {
            [&](am::v5::publish_packet const& p) {
                BOOST_TEST(p == dup_publish);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        }
    );
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

// When a QoS1 PUBLISH cannot be sent because the peer's Receive Maximum is
// exceeded, the tentatively stored publish (and its pid_puback_ entry) must be
// cleaned up. Otherwise the released packet_id leaves a dangling store entry.
BOOST_AUTO_TEST_CASE(v5_receive_maximum_exceeded_recover_qos1) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};

    // CONNECT with session_expiry_interval to enable the store.
    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0,      // keep_alive
        "cid1",
        std::nullopt,
        std::nullopt,
        std::nullopt,
        am::properties{
            am::property::session_expiry_interval{am::session_never_expire}
        }
    };
    c.send(connect);

    // CONNACK granting Receive Maximum of 1.
    {
        auto connack = am::v5::connack_packet{
            false,   // session_present
            am::connect_reason_code::success,
            am::properties{
                am::property::receive_maximum{1}
            }
        };
        std::istringstream is{am::to_string(connack.const_buffer_sequence())};
        c.recv(is);
    }
    BOOST_TEST(c.get_connection_status() == am::connection_status::connected);
    BOOST_CHECK(c.get_receive_maximum_vacancy_for_send() == std::size_t{1});

    // First PUBLISH consumes the single available slot.
    auto pid_a = c.acquire_unique_packet_id();
    auto pub_a = am::v5::publish_packet(
        *pid_a,
        "topic/a",
        "A",
        am::qos::at_least_once
    );
    c.send(pub_a);
    BOOST_CHECK(c.get_receive_maximum_vacancy_for_send() == std::size_t{0});

    // Second PUBLISH must fail with receive_maximum_exceeded.
    auto pid_b = c.acquire_unique_packet_id();
    auto pub_b = am::v5::publish_packet(
        *pid_b,
        "topic/b",
        "B",
        am::qos::at_least_once
    );
    auto events = c.send(pub_b);
    BOOST_TEST(events.size() == 2);
    std::visit(
        am::overload {
            [&](am::error_code const& e) {
                BOOST_TEST(e == am::disconnect_reason_code::receive_maximum_exceeded);
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
                BOOST_TEST(ev.get() == *pid_b);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[1]
    );

    // The failed publish must not have consumed a flow-control slot.
    BOOST_CHECK(c.get_receive_maximum_vacancy_for_send() == std::size_t{0});

    // The store must contain only publish A; publish B must have been erased.
    {
        auto packets = c.get_stored_packets();
        BOOST_TEST(packets.size() == 1);
        auto dup_a = pub_a;
        dup_a.set_dup(true);
        packets[0].visit(
            am::overload {
                [&](am::v5::publish_packet const& p) {
                    BOOST_TEST(p == dup_a);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            }
        );
    }

    // PUBACK for A frees the slot and clears the store.
    {
        auto puback = am::v5::puback_packet{*pid_a};
        std::istringstream is{am::to_string(puback.const_buffer_sequence())};
        c.recv(is);
    }
    BOOST_CHECK(c.get_receive_maximum_vacancy_for_send() == std::size_t{1});
    BOOST_TEST(c.get_stored_packets().size() == 0);

    // A subsequent PUBLISH (reusing a released packet_id) must be admitted and
    // stored on its own, proving the released id carried no stale store entry.
    auto pid_c = c.acquire_unique_packet_id();
    auto pub_c = am::v5::publish_packet(
        *pid_c,
        "topic/c",
        "C",
        am::qos::at_least_once
    );
    auto events_c = c.send(pub_c);
    BOOST_TEST(events_c.size() == 1);
    std::visit(
        am::overload {
            [&](am::event::send const&) {
                BOOST_TEST(true);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events_c[0]
    );
    {
        auto packets = c.get_stored_packets();
        BOOST_TEST(packets.size() == 1);
        auto dup_c = pub_c;
        dup_c.set_dup(true);
        packets[0].visit(
            am::overload {
                [&](am::v5::publish_packet const& p) {
                    BOOST_TEST(p == dup_c);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            }
        );
    }
}

// QoS2 variant: the stored publish and the pid_pubrec_ entry must be cleaned up
// when Receive Maximum is exceeded.
BOOST_AUTO_TEST_CASE(v5_receive_maximum_exceeded_recover_qos2) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0,      // keep_alive
        "cid1",
        std::nullopt,
        std::nullopt,
        std::nullopt,
        am::properties{
            am::property::session_expiry_interval{am::session_never_expire}
        }
    };
    c.send(connect);

    {
        auto connack = am::v5::connack_packet{
            false,   // session_present
            am::connect_reason_code::success,
            am::properties{
                am::property::receive_maximum{1}
            }
        };
        std::istringstream is{am::to_string(connack.const_buffer_sequence())};
        c.recv(is);
    }
    BOOST_TEST(c.get_connection_status() == am::connection_status::connected);

    auto pid_a = c.acquire_unique_packet_id();
    auto pub_a = am::v5::publish_packet(
        *pid_a,
        "topic/a",
        "A",
        am::qos::exactly_once
    );
    c.send(pub_a);

    auto pid_b = c.acquire_unique_packet_id();
    auto pub_b = am::v5::publish_packet(
        *pid_b,
        "topic/b",
        "B",
        am::qos::exactly_once
    );
    auto events = c.send(pub_b);
    BOOST_TEST(events.size() == 2);
    std::visit(
        am::overload {
            [&](am::error_code const& e) {
                BOOST_TEST(e == am::disconnect_reason_code::receive_maximum_exceeded);
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
                BOOST_TEST(ev.get() == *pid_b);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[1]
    );

    // Only publish A must remain in the store.
    {
        auto packets = c.get_stored_packets();
        BOOST_TEST(packets.size() == 1);
        auto dup_a = pub_a;
        dup_a.set_dup(true);
        packets[0].visit(
            am::overload {
                [&](am::v5::publish_packet const& p) {
                    BOOST_TEST(p == dup_a);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            }
        );
    }
}

// When a QoS1 PUBLISH carries an out-of-range Topic Alias, sending fails after
// the publish has already been added to the store. The store entry and the
// pid_puback_ entry must be erased so the released packet_id leaves no trace.
BOOST_AUTO_TEST_CASE(v5_topic_alias_range_over_recover) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0,      // keep_alive
        "cid1",
        std::nullopt,
        std::nullopt,
        std::nullopt,
        am::properties{
            am::property::session_expiry_interval{am::session_never_expire}
        }
    };
    c.send(connect);

    {
        auto connack = am::v5::connack_packet{
            false,   // session_present
            am::connect_reason_code::success,
            am::properties{
                am::property::topic_alias_maximum{2}
            }
        };
        std::istringstream is{am::to_string(connack.const_buffer_sequence())};
        c.recv(is);
    }
    BOOST_TEST(c.get_connection_status() == am::connection_status::connected);

    // Topic Alias 5 exceeds the granted maximum of 2.
    auto pid = c.acquire_unique_packet_id();
    auto publish = am::v5::publish_packet(
        *pid,
        "topic/a",
        "A",
        am::qos::at_least_once,
        am::properties{
            am::property::topic_alias{5}
        }
    );
    auto events = c.send(publish);
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
                BOOST_TEST(ev.get() == *pid);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[1]
    );

    // The store must be empty: the tentatively added publish was erased.
    BOOST_TEST(c.get_stored_packets().size() == 0);

    // The released packet_id must be reusable for a subsequent valid publish.
    auto pid_next = c.acquire_unique_packet_id();
    auto publish_next = am::v5::publish_packet(
        *pid_next,
        "topic/b",
        "B",
        am::qos::at_least_once
    );
    auto events_next = c.send(publish_next);
    BOOST_TEST(events_next.size() == 1);
    std::visit(
        am::overload {
            [&](am::event::send const&) {
                BOOST_TEST(true);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events_next[0]
    );
    {
        auto packets = c.get_stored_packets();
        BOOST_TEST(packets.size() == 1);
        auto dup_next = publish_next;
        dup_next.set_dup(true);
        packets[0].visit(
            am::overload {
                [&](am::v5::publish_packet const& p) {
                    BOOST_TEST(p == dup_next);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            }
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
