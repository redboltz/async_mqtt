// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_DISABLE_ASSERTS

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <boost/lexical_cast.hpp>

#include <async_mqtt/util/buffer.hpp>


#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_prop_variant_no_assert)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(monostate) {
    am::property_variant pv;
    BOOST_TEST(pv.id() == static_cast<am::property::id>(0));
    BOOST_TEST(pv.num_of_const_buffer_sequence() == 0);
    BOOST_TEST(pv.const_buffer_sequence().size() == 0);
    BOOST_TEST(pv.size() == 0);
}

BOOST_AUTO_TEST_SUITE_END()
