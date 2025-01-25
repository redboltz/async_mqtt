// Copyright Takatoshi Kondo 2025
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/protocol/timer.hpp>

BOOST_AUTO_TEST_SUITE(ut_timer)

namespace am = async_mqtt;


BOOST_AUTO_TEST_CASE(kind) {
    {
        std::stringstream ss;
        ss << am::timer_kind::pingreq_send;
        BOOST_TEST(ss.str() == "pingreq_send");
    }
    {
        std::stringstream ss;
        ss << am::timer_kind::pingreq_recv;
        BOOST_TEST(ss.str() == "pingreq_recv");
    }
    {
        std::stringstream ss;
        ss << am::timer_kind::pingresp_recv;
        BOOST_TEST(ss.str() == "pingresp_recv");
    }
    {
        std::stringstream ss;
        ss << static_cast<am::timer_kind>(3);
        BOOST_TEST(ss.str() == "unknown_timer_kind");
    }
}

BOOST_AUTO_TEST_CASE(op) {
    {
        std::stringstream ss;
        ss << am::timer_op::reset;
        BOOST_TEST(ss.str() == "reset");
    }
    {
        std::stringstream ss;
        ss << am::timer_op::cancel;
        BOOST_TEST(ss.str() == "cancel");
    }
    {
        std::stringstream ss;
        ss << static_cast<am::timer_op>(3);
        BOOST_TEST(ss.str() == "unknown_timer_op");
    }
}

BOOST_AUTO_TEST_SUITE_END()
