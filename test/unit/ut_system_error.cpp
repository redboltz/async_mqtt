// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/exception.hpp>

BOOST_AUTO_TEST_SUITE(ut_system_error)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(create_compare) {
    auto se1 = am::make_error(am::errc::bad_message, "aaa");
    auto se2 = am::make_error(am::errc::bad_message, "bbb");
    auto se3 = am::make_error(am::errc::protocol_error, "aaa");
    auto se1_dup = am::make_error(am::errc::bad_message, "aaa");
    BOOST_TEST(se1 == se1);
    BOOST_TEST(se1 != se2);
    BOOST_TEST(se1 != se3);
    BOOST_TEST(se1 == se1_dup);
    BOOST_TEST(!(se1 < se1));
    BOOST_TEST(!(se1 > se1));
    BOOST_TEST((se1 < se2 || se1 > se2));
    BOOST_TEST((se1 > se3 || se1 < se3));
    BOOST_TEST(!(se1 < se1_dup));
    BOOST_TEST(!(se1 > se1_dup));
}

BOOST_AUTO_TEST_SUITE_END()
