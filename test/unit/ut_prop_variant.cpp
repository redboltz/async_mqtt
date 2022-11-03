// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/packet/property_variant.hpp>

BOOST_AUTO_TEST_SUITE(ut_prop)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(variant) {
    am::property_variant p1 = am::property::authentication_data(am::allocate_buffer("ABC"));
    auto p2 = p1;
    BOOST_TEST(p1 == p2);
}

BOOST_AUTO_TEST_SUITE_END()
