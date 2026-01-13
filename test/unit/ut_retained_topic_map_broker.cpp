// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <set>

#include <broker/retained_topic_map.hpp>
#include <async_mqtt/protocol/packet/property_variant.hpp>

BOOST_AUTO_TEST_SUITE(ut_retained_topic_map_broker)

namespace am = async_mqtt;

struct retain {
    retain(
        am::buffer topic,
        am::buffer contents,
        am::properties props,
        am::qos qos_value)
        :topic(am::force_move(topic)),
         contents(am::force_move(contents)),
         props(am::force_move(props)),
         qos_value(qos_value)
    { }
    am::buffer topic;
    am::buffer contents;
    am::properties props;
    am::qos qos_value;
};

using retained_messages = am::retained_topic_map<retain>;

BOOST_AUTO_TEST_CASE( multi_non_wc_crud ) {
    retained_messages m;

    // publish
    {
        retain r {
            "a/b/c",
            "contents1",
            am::properties {},
            am::qos::at_most_once
        };
        m.insert_or_assign(r.topic, r);
    }
    {
        retain r {
            "a/b",
            "contents2",
            am::properties {},
            am::qos::at_most_once
        };
        m.insert_or_assign(r.topic, r);
    }

    // subscribe match
    {
        std::set<std::string_view> msgs {
            "contents1"
        };
        m.find(
            "a/b/c",
            [&](retain const& v) {
                BOOST_TEST(msgs.erase(v.contents) == 1);
            }
        );
        BOOST_TEST(msgs.empty());
    }
    {
        std::set<std::string_view> msgs {
            "contents2"
        };
        m.find(
            "a/b",
            [&](retain const& v) {
                BOOST_TEST(msgs.erase(v.contents) == 1);
            }
        );
        BOOST_TEST(msgs.empty());
    }

    // remove (publish with empty contents
    {
        m.erase("a/b");
        m.find(
            "a/b",
            [&](retain const&) {
                BOOST_TEST(false);
            }
        );
        std::set<std::string_view> msgs {
            "contents1"
        };
        m.find(
            "a/b/c",
            [&](retain const& v) {
                BOOST_TEST(msgs.erase(v.contents) == 1);
            }
        );
        BOOST_TEST(msgs.empty());
    }
    {
        m.erase("a/b/c");
        m.find(
            "a/b",
            [&](retain const&) {
                BOOST_TEST(false);
            }
        );
        m.find(
            "a/b/c",
            [&](retain const&) {
                BOOST_TEST(false);
            }
        );
    }
}

BOOST_AUTO_TEST_CASE( multi_wc_crud ) {
    retained_messages m;

    // publish
    {
        retain r {
            "a/b/c",
            "contents1",
            am::properties {},
            am::qos::at_most_once
        };
        m.insert_or_assign(r.topic, r);
    }
    {
        retain r {
            "a/b",
            "contents2",
            am::properties {},
            am::qos::at_most_once
        };
        m.insert_or_assign(r.topic, r);
    }

    // subscribe match
    {
        std::set<std::string_view> msgs {
            "contents1",
        };
        m.find(
            "a/+/c",
            [&](retain const& v) {
                BOOST_TEST(msgs.erase(v.contents) == 1);
            }
        );
        BOOST_TEST(msgs.empty());
    }
    {
        std::set<std::string_view> msgs {
            "contents1",
            "contents2",
        };
        m.find(
            "a/#",
            [&](retain const& v) {
                BOOST_TEST(msgs.erase(v.contents) == 1);
            }
        );
        BOOST_TEST(msgs.empty());
    }
    {
        std::set<std::string_view> msgs {
            "contents1",
            "contents2",
        };
        m.find(
            "a/+/#",
            [&](retain const& v) {
                BOOST_TEST(msgs.erase(v.contents) == 1);
            }
        );
        BOOST_TEST(msgs.empty());
    }

    // remove (publish with empty contents)
    {
        m.erase("a/b");
        {
            std::set<std::string_view> msgs {
                "contents1",
            };
            m.find(
                "a/#",
                [&](retain const& v) {
                    BOOST_TEST(msgs.erase(v.contents) == 1);
                }
            );
            BOOST_TEST(msgs.empty());
        }
        {
            std::set<std::string_view> msgs {
                "contents1"
            };
            m.find(
                "a/#",
                [&](retain const& v) {
                    BOOST_TEST(msgs.erase(v.contents) == 1);
                }
            );
            BOOST_TEST(msgs.empty());
        }
    }
    {
        m.erase("a/b/c");
        m.find(
            "a/+/c",
            [&](retain const&) {
                BOOST_TEST(false);
            }
        );
        m.find(
            "#",
            [&](retain const&) {
                BOOST_TEST(false);
            }
        );
    }
}

BOOST_AUTO_TEST_CASE( wildcard_multiple_matches ) {
    // Test for issue #442: Multiple retained messages should be delivered
    // when subscribing with wildcard that matches multiple topics
    retained_messages m;

    // Publish retained message 1: a/x/b/c
    {
        retain r {
            "a/x/b/c",
            "message1",
            am::properties {},
            am::qos::at_most_once
        };
        m.insert_or_assign(r.topic, r);
    }

    // Publish retained message 2: a/y/b/c
    {
        retain r {
            "a/y/b/c",
            "message2",
            am::properties {},
            am::qos::at_most_once
        };
        m.insert_or_assign(r.topic, r);
    }

    BOOST_TEST(m.size() == 2);

    // Subscribe with wildcard a/+/b/# should match both
    {
        std::set<std::string_view> msgs {
            "message1",
            "message2",
        };
        m.find(
            "a/+/b/#",
            [&](retain const& v) {
                BOOST_TEST(msgs.erase(v.contents) == 1);
            }
        );
        BOOST_TEST(msgs.empty());
    }

    // Subscribe with wildcard a/+/b/c should also match both
    {
        std::set<std::string_view> msgs {
            "message1",
            "message2",
        };
        m.find(
            "a/+/b/c",
            [&](retain const& v) {
                BOOST_TEST(msgs.erase(v.contents) == 1);
            }
        );
        BOOST_TEST(msgs.empty());
    }

    // Subscribe with exact topic should match only one
    {
        std::set<std::string_view> msgs {
            "message1",
        };
        m.find(
            "a/x/b/c",
            [&](retain const& v) {
                BOOST_TEST(msgs.erase(v.contents) == 1);
            }
        );
        BOOST_TEST(msgs.empty());
    }

    // Add more topics to test comprehensive wildcard matching
    {
        retain r {
            "a/x/b/d",
            "message3",
            am::properties {},
            am::qos::at_most_once
        };
        m.insert_or_assign(r.topic, r);
    }
    {
        retain r {
            "a/y/b/d",
            "message4",
            am::properties {},
            am::qos::at_most_once
        };
        m.insert_or_assign(r.topic, r);
    }
    {
        retain r {
            "a/z/b/e",
            "message5",
            am::properties {},
            am::qos::at_most_once
        };
        m.insert_or_assign(r.topic, r);
    }

    BOOST_TEST(m.size() == 5);

    // Subscribe with a/+/b/# should match all 5 messages
    {
        std::set<std::string_view> msgs {
            "message1",
            "message2",
            "message3",
            "message4",
            "message5",
        };
        m.find(
            "a/+/b/#",
            [&](retain const& v) {
                BOOST_TEST(msgs.erase(v.contents) == 1);
            }
        );
        BOOST_TEST(msgs.empty());
    }

    // Subscribe with a/+/b/c should match only message1 and message2
    {
        std::set<std::string_view> msgs {
            "message1",
            "message2",
        };
        m.find(
            "a/+/b/c",
            [&](retain const& v) {
                BOOST_TEST(msgs.erase(v.contents) == 1);
            }
        );
        BOOST_TEST(msgs.empty());
    }

    // Subscribe with a/x/b/# should match message1 and message3
    {
        std::set<std::string_view> msgs {
            "message1",
            "message3",
        };
        m.find(
            "a/x/b/#",
            [&](retain const& v) {
                BOOST_TEST(msgs.erase(v.contents) == 1);
            }
        );
        BOOST_TEST(msgs.empty());
    }
}

BOOST_AUTO_TEST_SUITE_END()
