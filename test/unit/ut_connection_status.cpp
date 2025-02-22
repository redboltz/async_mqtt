// Copyright Takatoshi Kondo 2025
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/protocol/connection_status.hpp>

BOOST_AUTO_TEST_SUITE(ut_connection_status)

namespace am = async_mqtt;


BOOST_AUTO_TEST_CASE(status) {
    {
        std::stringstream ss;
        ss << am::connection_status::connected;
        BOOST_TEST(ss.str() == "connected");
    }
    {
        std::stringstream ss;
        ss << am::connection_status::connecting;
        BOOST_TEST(ss.str() == "connecting");
    }
    {
        std::stringstream ss;
        ss << am::connection_status::disconnected;
        BOOST_TEST(ss.str() == "disconnected");
    }
    {
        std::stringstream ss;
        ss << static_cast<am::connection_status>(3);
        BOOST_TEST(ss.str() == "unknown_connection_status");
    }
}

BOOST_AUTO_TEST_SUITE_END()
