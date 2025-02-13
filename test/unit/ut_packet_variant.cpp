// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"


#include <boost/lexical_cast.hpp>

#include <async_mqtt/protocol/packet/packet_variant.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet_variant)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE( stringize ) {
    am::packet_variant v{am::v3_1_1::pingreq_packet{}};
    BOOST_TEST(boost::lexical_cast<std::string>(v) == "v3_1_1::pingreq{}");
}

BOOST_AUTO_TEST_SUITE_END()
