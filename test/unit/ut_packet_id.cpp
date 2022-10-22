// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <limits>

#include <async_mqtt/packet_id_manager.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet_id)

namespace am = async_mqtt;
using packet_id_t = typename am::packet_id_type<2>::type;

BOOST_AUTO_TEST_CASE( initial ) {
    am::packet_id_manager<packet_id_t> pidm;
    BOOST_TEST(*pidm.acquire_unique_id() == 1);
}

BOOST_AUTO_TEST_CASE( increment ) {
    am::packet_id_manager<packet_id_t> pidm;
    BOOST_TEST(*pidm.acquire_unique_id() == 1);
    BOOST_TEST(*pidm.acquire_unique_id() == 2);
}

BOOST_AUTO_TEST_CASE( user_register ) {
    am::packet_id_manager<packet_id_t> pidm;
    BOOST_TEST(!pidm.register_id(0));
    BOOST_TEST(pidm.register_id(1));
    BOOST_TEST(!pidm.register_id(1));
    BOOST_TEST(pidm.register_id(2));
}

BOOST_AUTO_TEST_CASE( skip_acquire ) {
    am::packet_id_manager<packet_id_t> pidm;
    BOOST_TEST(pidm.register_id(3));
    BOOST_TEST(*pidm.acquire_unique_id() == 1);
    BOOST_TEST(*pidm.acquire_unique_id() == 2);
    BOOST_TEST(*pidm.acquire_unique_id() == 4);
    BOOST_TEST(*pidm.acquire_unique_id() == 5);
}

BOOST_AUTO_TEST_CASE( release_but_increment ) {
    am::packet_id_manager<packet_id_t> pidm;
    BOOST_TEST(*pidm.acquire_unique_id() == 1);
    BOOST_TEST(*pidm.acquire_unique_id() == 2);
    BOOST_TEST(*pidm.acquire_unique_id() == 3);
    pidm.release_id(2);
    BOOST_TEST(*pidm.acquire_unique_id() == 2);
    BOOST_TEST(*pidm.acquire_unique_id() == 4);
}

BOOST_AUTO_TEST_CASE( rotate ) {
    am::packet_id_manager<packet_id_t> pidm;
    std::vector<packet_id_t> result;
    std::vector<packet_id_t> expected;
    for (packet_id_t i = 0; i != std::numeric_limits<packet_id_t>::max(); ++i) {
        result.push_back(*pidm.acquire_unique_id());
        expected.push_back(packet_id_t(i + 1));
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
    am::packet_id_manager<packet_id_t> pidm;
    for (packet_id_t i = 0; i != std::numeric_limits<packet_id_t>::max(); ++i) {
        pidm.acquire_unique_id();
    }
    BOOST_TEST(!pidm.acquire_unique_id());
}

BOOST_AUTO_TEST_SUITE_END()
