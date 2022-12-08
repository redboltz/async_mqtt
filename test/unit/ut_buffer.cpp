// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <vector>
#include <async_mqtt/buffer.hpp>

BOOST_AUTO_TEST_SUITE(ut_buffer)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(seq) {
    BOOST_TEST(am::is_buffer_sequence<am::buffer>::value);
    BOOST_TEST(am::is_buffer_sequence<std::vector<am::buffer>>::value);
    BOOST_TEST(am::is_buffer_sequence<std::decay_t<std::vector<am::buffer> const&>>::value);
}

BOOST_AUTO_TEST_SUITE_END()
