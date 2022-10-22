// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/packet/topic_sharename.hpp>

BOOST_AUTO_TEST_SUITE(ut_topic_sharename)

namespace am = async_mqtt;
using namespace am::literals;

// success
BOOST_AUTO_TEST_CASE( parse_success1 ) {
    am::topic_sharename ts{"$share/share_name/topic_filter"_mb};
    BOOST_CHECK(ts);
    BOOST_TEST(ts.sharename() == "share_name");
    BOOST_TEST(ts.topic() == "topic_filter");
}

BOOST_AUTO_TEST_CASE( parse_success2 ) {
    am::topic_sharename ts{"topic_filter"_mb};
    BOOST_CHECK(ts);
    BOOST_TEST(ts.sharename() == "");
    BOOST_TEST(ts.topic() == "topic_filter");
}


BOOST_AUTO_TEST_CASE( parse_success3 ) {
    am::topic_sharename ts{"$share/share_name//"_mb};
    BOOST_CHECK(ts);
    BOOST_TEST(ts.sharename() == "share_name");
    BOOST_TEST(ts.topic() == "/");
}

// error

BOOST_AUTO_TEST_CASE( parse_error1 ) {
    am::topic_sharename ts{"$share//topic_filter"_mb};
    BOOST_CHECK(!ts);
}


BOOST_AUTO_TEST_CASE( parse_error2 ) {
    am::topic_sharename ts{"$share/share_name"_mb};
    BOOST_CHECK(!ts);
}

BOOST_AUTO_TEST_CASE( parse_error3 ) {
    am::topic_sharename ts{"$share/share_name/"_mb};
    BOOST_CHECK(!ts);
}

BOOST_AUTO_TEST_SUITE_END()
