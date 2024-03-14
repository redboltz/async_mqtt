// Copyright Takatoshi Kondo 2021
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <string_view>
#include <sstream>

#include <async_mqtt/util/scope_guard.hpp>

BOOST_AUTO_TEST_SUITE(ut_unique_scope_guard)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE( simple ) {
    int i = 0;
    {
        auto g1 = am::unique_scope_guard(
            [&] {
                ++i;
            }
        );
    }
    BOOST_TEST(i == 1);
}

BOOST_AUTO_TEST_CASE( move_c ) {
    int i = 0;
    {
        auto g1 = am::unique_scope_guard(
            [&] {
                ++i;
            }
        );
        auto g2{std::move(g1)};
    }
    BOOST_TEST(i == 1);
}

BOOST_AUTO_TEST_SUITE_END()
