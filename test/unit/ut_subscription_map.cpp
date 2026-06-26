// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <set>

#include <broker/subscription_map.hpp>

BOOST_AUTO_TEST_SUITE(ut_subscription_map)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE( failed_erase ) {
    using elem_t = int;
    using value_t = std::shared_ptr<elem_t>; // shared_ptr for '<' and hash
    using sm_t = am::multiple_subscription_map<std::string, value_t>;

    sm_t m;
    auto v1 = std::make_shared<elem_t>(1);
    auto v2 = std::make_shared<elem_t>(2);

    BOOST_TEST(m.size() == 0);
    auto it_success1 = m.insert_or_assign("a/b/c", "test", v1);
    assert(it_success1.second);
    BOOST_TEST(m.size() == 1);

    auto it_success2 = m.insert_or_assign("a/b", "test", v2);
    assert(it_success2.second);
    BOOST_TEST(m.size() == 2);

    auto e1 = m.erase(it_success1.first, "test");
    BOOST_TEST(e1 == 1);
    BOOST_TEST(m.size() == 1);

    auto e2 = m.erase(it_success2.first, "test"); //  Invalid handle was specified is thrown here
    BOOST_TEST(e2 == 1);
    BOOST_TEST(m.size() == 0);

}

BOOST_AUTO_TEST_CASE( test_single_subscription ) {
    std::string text = "example/test/A";

    am::single_subscription_map< std::string > map;
    auto handle = map.insert(text, text).first;
    BOOST_TEST(handle.second == "A");
    BOOST_TEST(map.handle_to_topic_filter(handle) == text);
    BOOST_TEST(map.insert(text, text).second == false);
    map.update(handle, "new_value");
    map.erase(handle);

    BOOST_TEST(map.insert(text, text).second == true);
    BOOST_TEST(map.erase(text) == 1);

    BOOST_TEST(map.size() == 0);
    BOOST_TEST(map.internal_size() == 1);

    map.insert(text, text);
    BOOST_TEST(map.size() == 1);
    BOOST_TEST(map.internal_size() > 1u);

    map.erase(text);
    BOOST_TEST(map.size() == 0);
    BOOST_TEST(map.internal_size() == 1);

    std::vector<std::string> values = {
        "example/test/A", "example/+/A", "example/#", "#"
    };

    for (auto const& i : values) {
        map.insert(i, i);
    }

    // Attempt to remove entry which has no value
    BOOST_TEST(map.erase("example") == 0);
    BOOST_TEST(map.erase(*map.lookup("example")) == 0);
    BOOST_TEST(map.erase("example") == 0);
    BOOST_TEST(map.erase(*map.lookup("example")) == 0);

    std::vector<std::string> matches;
    map.find("example/test/A", [&matches](std::string const &a) {
        matches.push_back(a);
    });
    BOOST_TEST(matches.size() == 4);

    matches = {};
    map.find("hash_match_only", [&matches](std::string const &a) {
        matches.push_back(a);
    });
    BOOST_TEST(matches.size() == 1);

    matches = {};
    map.find("example/hash_only", [&matches](std::string const &a) {
        matches.push_back(a);
    });
    BOOST_TEST(matches.size() == 2);

    matches = {};
    map.find("example/plus/A", [&matches](std::string const &a) {
        matches.push_back(a);
    });
    BOOST_TEST(matches.size() == 3);

    BOOST_TEST(map.erase("non-existent") == 0);

    for (auto const& i : values) {
        BOOST_TEST(map.size() != 0);
        BOOST_TEST(map.erase(i) == 1);
    }

    BOOST_TEST(map.size() == 0);
    BOOST_TEST(map.internal_size() == 1);

    std::vector< am::single_subscription_map< std::string >::handle > handles;
    for (auto const& i : values) {
        handles.push_back(map.insert(i, i).first);
    }

    for (auto const& i : handles) {
        BOOST_TEST(map.size() != 0);
        BOOST_TEST(map.erase(i) == 1);
    }

    BOOST_TEST(map.size() == 0);
    BOOST_TEST(map.internal_size() == 1);

}

BOOST_AUTO_TEST_CASE( test_multiple_subscription ) {
    std::string text = "example/test/A";

    am::multiple_subscription_map<std::string, int> map;

    BOOST_TEST(map.insert_or_assign("a/b/c", "123", 0).second == true);
    BOOST_TEST(map.size() == 1);
    BOOST_TEST(map.internal_size() == 4);

    map.find("a/b/c", [](std::string const &key, int value) {
        BOOST_TEST(key == "123");
        BOOST_TEST(value == 0);
    });

    BOOST_TEST(map.insert_or_assign("a/b/c", "123", 1).second == false);
    BOOST_TEST(map.size() == 1);
    BOOST_TEST(map.internal_size() == 4);

    map.find("a/b/c", [](std::string const &key, int value) {
        BOOST_TEST(key == "123");
        BOOST_TEST(value == 1);
    });

    map.insert_or_assign("a/b", "123", 0);
    BOOST_TEST(map.size() == 2);
    BOOST_TEST(map.internal_size() == 4);

    map.erase("a/b", "123");
    BOOST_TEST(map.size() == 1);
    BOOST_TEST(map.internal_size() == 4);

    map.erase("a/b", "123");
    BOOST_TEST(map.size() == 1);
    BOOST_TEST(map.internal_size() == 4);

    map.erase("a/b/c", "123");
    BOOST_TEST(map.size() == 0);
    BOOST_TEST(map.internal_size() == 1);

    std::vector<std::string> values = {
        "example/test/A", "example/+/A", "example/#", "#"
    };

    // Add some duplicates and overlapping paths
    map.insert_or_assign(values[0], values[0], 0);
    BOOST_TEST(map.insert_or_assign(values[0], values[0], 0).second == false);
    BOOST_TEST(map.insert_or_assign(values[0], "blaat", 0).second == true);

    map.erase(values[0], "blaat");
    BOOST_TEST(map.size() == 1);

    map.erase(values[0], values[0]);
    BOOST_TEST(map.size() == 0);

    // Perform test again but this time using handles
    map.insert_or_assign(values[0], values[0], 0);
    BOOST_TEST(map.insert_or_assign(*map.lookup(values[0]), values[0], 0).second == false);
    BOOST_TEST(map.insert_or_assign(*map.lookup(values[0]), "blaat", 0).second == true);

    BOOST_TEST(!map.lookup("non/exist"));

    map.erase(*map.lookup(values[0]), "blaat");
    BOOST_TEST(map.size() == 1);

    map.erase(*map.lookup(values[0]), values[0]);
    BOOST_TEST(map.size() == 0);

    for (auto const& i : values) {
        map.insert_or_assign(i, i, 0);
    }

    BOOST_TEST(map.size() == 4);

    // Attempt to remove entry which has no value
    BOOST_TEST(map.erase("example", "example") == 0);
    BOOST_TEST(map.erase(*map.lookup("example"), "example") == 0);
    BOOST_TEST(map.erase("example", "example") == 0);
    BOOST_TEST(map.erase(*map.lookup("example"), "example") == 0);

    BOOST_TEST(map.lookup(values[0])->second == "A");
    BOOST_TEST(map.handle_to_topic_filter(*map.lookup(values[0])) == values[0]);

    std::vector<std::string> matches;
    map.find("example/test/A", [&matches](std::string const &a, int /*value*/) {
        matches.push_back(a);
    });
    BOOST_TEST(matches.size() == 4);

    matches = {};
    map.find("hash_match_only", [&matches](std::string const &a, int /*value*/) {
        matches.push_back(a);
    });
    BOOST_TEST(matches.size() == 1);

    matches = {};
    map.find("example/hash_only", [&matches](std::string const &a, int /*value*/) {
        matches.push_back(a);
    });
    BOOST_TEST(matches.size() == 2);

    matches = {};
    map.find("example/plus/A", [&matches](std::string const &a, int /*value*/) {
        matches.push_back(a);
    });
    BOOST_TEST(matches.size() == 3);

    BOOST_TEST(map.erase("non-existent", "non-existent") == 0);

    for (auto const& i : values) {
        BOOST_TEST(map.size() != 0);
        BOOST_TEST(map.erase(i, i) == 1);
    }

    BOOST_TEST(map.size() == 0);
    BOOST_TEST(map.internal_size() == 1);

    // Check if $ does not match # at root
    map = am::multiple_subscription_map<std::string, int>();

    map.insert_or_assign("#", "123", 10);
    map.insert_or_assign("example/plus/A", "123", 10);

    matches = {};
    map.find("example/plus/A", [&matches](std::string const &a, int /*value*/) {
        matches.push_back(a);
    });
    BOOST_TEST(matches.size() == 2);

    matches = {};
    map.find("$SYS/plus/A", [&matches](std::string const &a, int /*value*/) {
        matches.push_back(a);
    });
    BOOST_TEST(matches.size() == 0);

 //   map.dump(std::cout);
}

BOOST_AUTO_TEST_CASE( test_multiple_subscription_modify ) {
    struct my {
        void const_mem_fun() const {
            // std::cout << "const_mem_fun()" << std::endl;
        }
        void non_const_mem_fun() {
            // std::cout << "non_const_mem_fun()" << std::endl;
        }
    };


    using mi_t = am::multiple_subscription_map<std::string, my>;
    mi_t map;
    map.insert_or_assign("a/b/c", "123", my());
    map.insert_or_assign("a/b/c", "456", my());

    map.modify("a/b/c", [](std::string const& /*key*/, my &value) {
        value.const_mem_fun();
        value.non_const_mem_fun();
    });
}

BOOST_AUTO_TEST_CASE( test_move_only ) {

    struct my {
        my() = delete;
        my(int) {}
        my(my const&) = delete;
        my(my&&) = default;
        my& operator=(my const&) = delete;
        my& operator=(my&&) = default;
        ~my() = default;
    };

    using mi_t = am::multiple_subscription_map<std::string, my>;
    mi_t map;
    map.insert_or_assign("a/b/c", "123", my(1));
    map.insert_or_assign("a/b/c", "456", my(2));
}

// A multi-level wildcard '#' also matches the parent level itself.
// e.g. topic_filter "a/b/#" must match topic "a/b" (MQTT spec 4.7.1.2).
// https://github.com/redboltz/async_mqtt/issues (broker not forwarding "a/b" to "a/b/#")
BOOST_AUTO_TEST_CASE( hash_matches_parent_level ) {
    am::single_subscription_map<std::string> map;

    std::vector<std::string> values = {
        "a/b/#", "a/+", "a/#", "#", "x/y/z"
    };
    for (auto const& i : values) {
        map.insert(i, i);
    }

    auto collect = [&](std::string_view topic) {
        std::set<std::string> matches;
        map.find(topic, [&matches](std::string const& a) {
            matches.insert(a);
        });
        return matches;
    };

    // "a/b" must match "a/b/#" (parent level), "a/+" (single level) and "a/#", "#"
    {
        auto matches = collect("a/b");
        BOOST_TEST(matches.count("a/b/#") == 1); // the regression: parent-level match
        BOOST_TEST(matches.count("a/+") == 1);
        BOOST_TEST(matches.count("a/#") == 1);
        BOOST_TEST(matches.count("#") == 1);
        BOOST_TEST(matches.size() == 4u);
    }

    // "a/b/c" must match "a/b/#" and "a/#", "#" but NOT "a/+"
    {
        auto matches = collect("a/b/c");
        BOOST_TEST(matches.count("a/b/#") == 1);
        BOOST_TEST(matches.count("a/#") == 1);
        BOOST_TEST(matches.count("#") == 1);
        BOOST_TEST(matches.count("a/+") == 0);
        BOOST_TEST(matches.size() == 3u);
    }

    // "a" (the parent of "a/#") must match "a/#" and "#"
    {
        auto matches = collect("a");
        BOOST_TEST(matches.count("a/#") == 1); // parent-level match
        BOOST_TEST(matches.count("#") == 1);
        BOOST_TEST(matches.size() == 2u);
    }

    // "x/y/z" exact match plus "#"; "x/y" must match "x/y/z"? no, but exists no "x/y/#"
    {
        auto matches = collect("x/y/z");
        BOOST_TEST(matches.count("x/y/z") == 1);
        BOOST_TEST(matches.count("#") == 1);
        BOOST_TEST(matches.size() == 2u);
    }
}

// '#' must not match topics starting with '$' when subscribed at the root level,
// but "$SYS/#" must match "$SYS" (parent level) as well.
BOOST_AUTO_TEST_CASE( hash_parent_level_dollar ) {
    am::single_subscription_map<std::string> map;

    std::vector<std::string> values = {
        "#", "$SYS/#"
    };
    for (auto const& i : values) {
        map.insert(i, i);
    }

    auto collect = [&](std::string_view topic) {
        std::set<std::string> matches;
        map.find(topic, [&matches](std::string const& a) {
            matches.insert(a);
        });
        return matches;
    };

    // root level "#" must not match "$SYS/..." topics
    {
        auto matches = collect("$SYS/broker/uptime");
        BOOST_TEST(matches.count("$SYS/#") == 1);
        BOOST_TEST(matches.count("#") == 0);
        BOOST_TEST(matches.size() == 1u);
    }

    // "$SYS/#" must match its parent level "$SYS"; root "#" must still be excluded
    {
        auto matches = collect("$SYS");
        BOOST_TEST(matches.count("$SYS/#") == 1); // parent-level match
        BOOST_TEST(matches.count("#") == 0);
        BOOST_TEST(matches.size() == 1u);
    }
}

// find()/modify() accept match_parent_hash to opt out of the parent-level '#'
// match. The broker ACL relies on this: a rule on "a/b/#" governs the "a/b/"
// subtree but must not govern the bare parent topic "a/b". Disabling the option
// must not affect descendant matching (e.g. "a/b/c" still matches "a/b/#").
BOOST_AUTO_TEST_CASE( hash_parent_level_opt_out ) {
    am::multiple_subscription_map<std::string, int> map;

    map.insert_or_assign("a/b/#", "abhash", 0);
    map.insert_or_assign("a/#", "ahash", 0);
    map.insert_or_assign("#", "roothash", 0);

    auto collect = [&](std::string_view topic, bool match_parent_hash) {
        std::set<std::string> matches;
        map.find(
            topic,
            [&matches](std::string const& key, int /*value*/) {
                matches.insert(key);
            },
            match_parent_hash
        );
        return matches;
    };

    // Default (match_parent_hash == true): "a/b" matches the parent level "a/b/#".
    {
        auto matches = collect("a/b", true);
        BOOST_TEST(matches.count("abhash") == 1); // parent-level
        BOOST_TEST(matches.count("ahash") == 1);
        BOOST_TEST(matches.count("roothash") == 1);
        BOOST_TEST(matches.size() == 3u);
    }

    // match_parent_hash == false: "a/b" no longer matches "a/b/#" (parent level),
    // but still matches the shallower "a/#" and "#".
    {
        auto matches = collect("a/b", false);
        BOOST_TEST(matches.count("abhash") == 0); // parent-level suppressed
        BOOST_TEST(matches.count("ahash") == 1);
        BOOST_TEST(matches.count("roothash") == 1);
        BOOST_TEST(matches.size() == 2u);
    }

    // Descendant matching is unaffected by the option: "a/b/c" still matches
    // "a/b/#" even with match_parent_hash == false.
    {
        auto matches = collect("a/b/c", false);
        BOOST_TEST(matches.count("abhash") == 1); // descendant, via in-loop '#'
        BOOST_TEST(matches.count("ahash") == 1);
        BOOST_TEST(matches.count("roothash") == 1);
        BOOST_TEST(matches.size() == 3u);
    }
}

BOOST_AUTO_TEST_SUITE_END()
