// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <set>

#include <async_mqtt/broker/retained_topic_map.hpp>
#include <async_mqtt/packet/property_variant.hpp>

BOOST_AUTO_TEST_SUITE(ut_retained_topic_map_broker)

namespace am = async_mqtt;
using namespace am::literals;

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
            "a/b/c"_mb,
            "contents1"_mb,
            am::properties {},
            am::qos::at_most_once
        };
        m.insert_or_assign(r.topic, r);
    }
    {
        retain r {
            "a/b"_mb,
            "contents2"_mb,
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
        m.erase("a/b"_mb);
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
        m.erase("a/b/c"_mb);
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
            "a/b/c"_mb,
            "contents1"_mb,
            am::properties {},
            am::qos::at_most_once
        };
        m.insert_or_assign(r.topic, r);
    }
    {
        retain r {
            "a/b"_mb,
            "contents2"_mb,
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
        m.erase("a/b"_mb);
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
        m.erase("a/b/c"_mb);
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

BOOST_AUTO_TEST_SUITE_END()
