// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <limits>

#include <async_mqtt/util/packet_id_manager.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet_id)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE( initial ) {
    am::packet_id_manager<am::packet_id_type> pidm;
    BOOST_TEST(*pidm.acquire_unique_id() == 1);
}

BOOST_AUTO_TEST_CASE( increment ) {
    am::packet_id_manager<am::packet_id_type> pidm;
    BOOST_TEST(*pidm.acquire_unique_id() == 1);
    BOOST_TEST(*pidm.acquire_unique_id() == 2);
}

BOOST_AUTO_TEST_CASE( user_register ) {
    am::packet_id_manager<am::packet_id_type> pidm;
    BOOST_TEST(!pidm.register_id(0));
    BOOST_TEST(pidm.register_id(1));
    BOOST_TEST(!pidm.register_id(1));
    BOOST_TEST(pidm.register_id(2));
}

BOOST_AUTO_TEST_CASE( skip_acquire ) {
    am::packet_id_manager<am::packet_id_type> pidm;
    BOOST_TEST(pidm.register_id(3));
    BOOST_TEST(*pidm.acquire_unique_id() == 1);
    BOOST_TEST(*pidm.acquire_unique_id() == 2);
    BOOST_TEST(*pidm.acquire_unique_id() == 4);
    BOOST_TEST(*pidm.acquire_unique_id() == 5);
}

BOOST_AUTO_TEST_CASE( release_but_increment ) {
    am::packet_id_manager<am::packet_id_type> pidm;
    BOOST_TEST(*pidm.acquire_unique_id() == 1);
    BOOST_TEST(*pidm.acquire_unique_id() == 2);
    BOOST_TEST(*pidm.acquire_unique_id() == 3);
    pidm.release_id(2);
    BOOST_TEST(*pidm.acquire_unique_id() == 2);
    BOOST_TEST(*pidm.acquire_unique_id() == 4);
}

BOOST_AUTO_TEST_CASE( rotate ) {
    am::packet_id_manager<am::packet_id_type> pidm;
    std::vector<am::packet_id_type> result;
    std::vector<am::packet_id_type> expected;
    for (am::packet_id_type i = 0; i != std::numeric_limits<am::packet_id_type>::max(); ++i) {
        result.push_back(*pidm.acquire_unique_id());
        expected.push_back(am::packet_id_type(i + 1));
    }
    BOOST_TEST(result == expected);
    pidm.release_id(1);
    BOOST_TEST(*pidm.acquire_unique_id() == 1);
    pidm.release_id(5);
    BOOST_TEST(*pidm.acquire_unique_id() == 5);
    pidm.release_id(2);
    BOOST_TEST(*pidm.acquire_unique_id() == 2);
}

BOOST_AUTO_TEST_CASE( exhausted ) {
    am::packet_id_manager<am::packet_id_type> pidm;
    for (am::packet_id_type i = 0; i != std::numeric_limits<am::packet_id_type>::max(); ++i) {
        pidm.acquire_unique_id();
    }
    BOOST_TEST(!pidm.acquire_unique_id());
}

BOOST_AUTO_TEST_SUITE_END()
