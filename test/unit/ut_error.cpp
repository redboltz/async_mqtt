// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <boost/asio.hpp>

#include <async_mqtt/error.hpp>


BOOST_AUTO_TEST_SUITE(ut_error)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(category) {
    am::error_code success;
    am::error_code actual_error = am::make_error_code(am::mqtt_error::packet_identifier_fully_used);
    BOOST_TEST(actual_error == am::mqtt_error::packet_identifier_fully_used);
    BOOST_CHECK(actual_error);

    am::error_code connect_return_code = am::make_error_code(am::connect_return_code::accepted);
    BOOST_TEST(connect_return_code == am::connect_return_code::accepted);
    BOOST_CHECK(!connect_return_code);

    am::error_code suback_return_code0 = am::make_error_code(am::suback_return_code::success_maximum_qos_0);
    BOOST_TEST(suback_return_code0 == am::suback_return_code::success_maximum_qos_0);
    BOOST_TEST(suback_return_code0 != am::suback_return_code::success_maximum_qos_1);
    BOOST_TEST(suback_return_code0 != am::suback_return_code::success_maximum_qos_2);
    BOOST_CHECK(!suback_return_code0);

    am::error_code suback_return_code1 = am::make_error_code(am::suback_return_code::success_maximum_qos_1);
    BOOST_TEST(suback_return_code1 == am::suback_return_code::success_maximum_qos_1);
    BOOST_CHECK(suback_return_code1); // boost::system::error_code evaluate to true only the value 0

    am::error_code suback_return_code2 = am::make_error_code(am::suback_return_code::success_maximum_qos_2);
    BOOST_TEST(suback_return_code2 == am::suback_return_code::success_maximum_qos_2);
    BOOST_CHECK(suback_return_code2); // boost::system::error_code evaluate to true only the value 0

    am::error_code disconnect_reason_code = am::make_error_code(am::disconnect_reason_code::normal_disconnection);
    BOOST_TEST(disconnect_reason_code == am::disconnect_reason_code::normal_disconnection);
    BOOST_CHECK(!disconnect_reason_code);

    am::error_code connect_reason_code = am::make_error_code(am::connect_reason_code::success);
    BOOST_TEST(connect_reason_code == am::connect_reason_code::success);
    BOOST_CHECK(!connect_reason_code);

    am::error_code suback_reason_code = am::make_error_code(am::suback_reason_code::granted_qos_0);
    BOOST_TEST(suback_reason_code == am::suback_reason_code::granted_qos_0);
    BOOST_CHECK(!suback_reason_code);

    am::error_code unsuback_reason_code = am::make_error_code(am::unsuback_reason_code::success);
    BOOST_TEST(unsuback_reason_code == am::unsuback_reason_code::success);
    BOOST_CHECK(!unsuback_reason_code);

    am::error_code puback_reason_code = am::make_error_code(am::puback_reason_code::success);
    BOOST_TEST(puback_reason_code == am::puback_reason_code::success);
    BOOST_CHECK(!puback_reason_code);

    am::error_code pubrec_reason_code = am::make_error_code(am::pubrec_reason_code::success);
    BOOST_TEST(pubrec_reason_code == am::pubrec_reason_code::success);
    BOOST_CHECK(!pubrec_reason_code);

    am::error_code pubrel_reason_code = am::make_error_code(am::pubrel_reason_code::success);
    BOOST_TEST(pubrel_reason_code == am::pubrel_reason_code::success);
    BOOST_CHECK(!pubrel_reason_code);

    am::error_code pubcomp_reason_code = am::make_error_code(am::pubcomp_reason_code::success);
    BOOST_TEST(pubcomp_reason_code == am::pubcomp_reason_code::success);
    BOOST_CHECK(!pubcomp_reason_code);

    am::error_code auth_reason_code = am::make_error_code(am::auth_reason_code::success);
    BOOST_TEST(auth_reason_code == am::auth_reason_code::success);
    BOOST_CHECK(!auth_reason_code);

    // should not equal even if the value is both 0, because of the different category.
    BOOST_TEST(success != connect_return_code);
    BOOST_TEST(success != suback_return_code0);
    BOOST_TEST(success != suback_return_code1);
    BOOST_TEST(success != suback_return_code2);
    BOOST_TEST(success != connect_reason_code);
    BOOST_TEST(success != disconnect_reason_code);
    BOOST_TEST(success != suback_reason_code);
    BOOST_TEST(success != unsuback_reason_code);
    BOOST_TEST(success != puback_reason_code);
    BOOST_TEST(success != pubrec_reason_code);
    BOOST_TEST(success != pubrel_reason_code);
    BOOST_TEST(success != pubcomp_reason_code);
    BOOST_TEST(success != auth_reason_code);
}

BOOST_AUTO_TEST_SUITE_END()
