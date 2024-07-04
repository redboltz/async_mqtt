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

    am::error_code connect_return_code1 = am::make_error_code(am::connect_return_code::accepted);
    BOOST_TEST(connect_return_code1 == am::connect_return_code::accepted);
    BOOST_CHECK(!connect_return_code1);
    am::error_code connect_return_code2 = am::make_error_code(am::connect_return_code::unacceptable_protocol_version);
    BOOST_TEST(connect_return_code2 == am::connect_return_code::unacceptable_protocol_version);
    BOOST_CHECK(connect_return_code2);

    am::error_code suback_return_code1 = am::make_error_code(am::suback_return_code::success_maximum_qos_0);
    BOOST_TEST(suback_return_code1 == am::suback_return_code::success_maximum_qos_0);
    BOOST_TEST(suback_return_code1 != am::suback_return_code::success_maximum_qos_1);
    BOOST_TEST(suback_return_code1 != am::suback_return_code::success_maximum_qos_2);
    BOOST_CHECK(!suback_return_code1);
    am::error_code suback_return_code2 = am::make_error_code(am::suback_return_code::success_maximum_qos_1);
    BOOST_TEST(suback_return_code2 == am::suback_return_code::success_maximum_qos_1);
    BOOST_CHECK(!suback_return_code2);
    am::error_code suback_return_code3 = am::make_error_code(am::suback_return_code::success_maximum_qos_2);
    BOOST_TEST(suback_return_code3 == am::suback_return_code::success_maximum_qos_2);
    BOOST_CHECK(!suback_return_code3);
    am::error_code suback_return_code4 = am::make_error_code(am::suback_return_code::failure);
    BOOST_TEST(suback_return_code4 == am::suback_return_code::failure);
    BOOST_CHECK(suback_return_code4);

    am::error_code connect_reason_code1 = am::make_error_code(am::connect_reason_code::success);
    BOOST_TEST(connect_reason_code1 == am::connect_reason_code::success);
    BOOST_CHECK(!connect_reason_code1);
    am::error_code connect_reason_code2 = am::make_error_code(am::connect_reason_code::malformed_packet);
    BOOST_TEST(connect_reason_code2 == am::connect_reason_code::malformed_packet);
    BOOST_CHECK(connect_reason_code2);

    am::error_code disconnect_reason_code1 = am::make_error_code(am::disconnect_reason_code::normal_disconnection);
    BOOST_TEST(disconnect_reason_code1 == am::disconnect_reason_code::normal_disconnection);
    BOOST_CHECK(!disconnect_reason_code1);
    am::error_code disconnect_reason_code2 = am::make_error_code(am::disconnect_reason_code::disconnect_with_will_message);
    BOOST_TEST(disconnect_reason_code2 == am::disconnect_reason_code::disconnect_with_will_message);
    BOOST_CHECK(!disconnect_reason_code2);
    am::error_code disconnect_reason_code3 = am::make_error_code(am::disconnect_reason_code::malformed_packet);
    BOOST_TEST(disconnect_reason_code3 == am::disconnect_reason_code::malformed_packet);
    BOOST_CHECK(disconnect_reason_code3);

    am::error_code suback_reason_code0 = am::make_error_code(am::suback_reason_code::granted_qos_0);
    BOOST_TEST(suback_reason_code0 == am::suback_reason_code::granted_qos_0);
    BOOST_TEST(suback_reason_code0 != am::suback_reason_code::granted_qos_1);
    BOOST_TEST(suback_reason_code0 != am::suback_reason_code::granted_qos_2);
    BOOST_CHECK(!suback_reason_code0);
    am::error_code suback_reason_code1 = am::make_error_code(am::suback_reason_code::granted_qos_1);
    BOOST_TEST(suback_reason_code1 == am::suback_reason_code::granted_qos_1);
    BOOST_CHECK(!suback_reason_code1);
    am::error_code suback_reason_code2 = am::make_error_code(am::suback_reason_code::granted_qos_2);
    BOOST_TEST(suback_reason_code2 == am::suback_reason_code::granted_qos_2);
    BOOST_CHECK(!suback_reason_code2);
    am::error_code suback_reason_code3 = am::make_error_code(am::suback_reason_code::packet_identifier_in_use);
    BOOST_TEST(suback_reason_code3 == am::suback_reason_code::packet_identifier_in_use);
    BOOST_CHECK(suback_reason_code3);

    am::error_code unsuback_reason_code1 = am::make_error_code(am::unsuback_reason_code::success);
    BOOST_TEST(unsuback_reason_code1 == am::unsuback_reason_code::success);
    BOOST_CHECK(!unsuback_reason_code1);
    am::error_code unsuback_reason_code2 = am::make_error_code(am::unsuback_reason_code::no_subscription_existed);
    BOOST_TEST(unsuback_reason_code2 == am::unsuback_reason_code::no_subscription_existed);
    BOOST_CHECK(!unsuback_reason_code2);
    am::error_code unsuback_reason_code3 = am::make_error_code(am::unsuback_reason_code::not_authorized);
    BOOST_TEST(unsuback_reason_code3 == am::unsuback_reason_code::not_authorized);
    BOOST_CHECK(unsuback_reason_code3);

    am::error_code puback_reason_code1 = am::make_error_code(am::puback_reason_code::success);
    BOOST_TEST(puback_reason_code1 == am::puback_reason_code::success);
    BOOST_CHECK(!puback_reason_code1);
    am::error_code puback_reason_code2 = am::make_error_code(am::puback_reason_code::no_matching_subscribers);
    BOOST_TEST(puback_reason_code2 == am::puback_reason_code::no_matching_subscribers);
    BOOST_CHECK(!puback_reason_code2);
    am::error_code puback_reason_code3 = am::make_error_code(am::puback_reason_code::topic_name_invalid);
    BOOST_TEST(puback_reason_code3 == am::puback_reason_code::topic_name_invalid);
    BOOST_CHECK(puback_reason_code3);

    am::error_code pubrec_reason_code1 = am::make_error_code(am::pubrec_reason_code::success);
    BOOST_TEST(pubrec_reason_code1 == am::pubrec_reason_code::success);
    BOOST_CHECK(!pubrec_reason_code1);
    am::error_code pubrec_reason_code2 = am::make_error_code(am::pubrec_reason_code::no_matching_subscribers);
    BOOST_TEST(pubrec_reason_code2 == am::pubrec_reason_code::no_matching_subscribers);
    BOOST_CHECK(!pubrec_reason_code2);
    am::error_code pubrec_reason_code3 = am::make_error_code(am::pubrec_reason_code::topic_name_invalid);
    BOOST_TEST(pubrec_reason_code3 == am::pubrec_reason_code::topic_name_invalid);
    BOOST_CHECK(pubrec_reason_code3);

    am::error_code pubrel_reason_code1 = am::make_error_code(am::pubrel_reason_code::success);
    BOOST_TEST(pubrel_reason_code1 == am::pubrel_reason_code::success);
    BOOST_CHECK(!pubrel_reason_code1);
    am::error_code pubrel_reason_code2 = am::make_error_code(am::pubrel_reason_code::packet_identifier_not_found);
    BOOST_TEST(pubrel_reason_code2 == am::pubrel_reason_code::packet_identifier_not_found);
    BOOST_CHECK(pubrel_reason_code2);

    am::error_code pubcomp_reason_code1 = am::make_error_code(am::pubcomp_reason_code::success);
    BOOST_TEST(pubcomp_reason_code1 == am::pubcomp_reason_code::success);
    BOOST_CHECK(!pubcomp_reason_code1);
    am::error_code pubcomp_reason_code2 = am::make_error_code(am::pubcomp_reason_code::packet_identifier_not_found);
    BOOST_TEST(pubcomp_reason_code2 == am::pubcomp_reason_code::packet_identifier_not_found);
    BOOST_CHECK(pubcomp_reason_code2);

    am::error_code auth_reason_code1 = am::make_error_code(am::auth_reason_code::success);
    BOOST_TEST(auth_reason_code1 == am::auth_reason_code::success);
    BOOST_CHECK(!auth_reason_code1);
    am::error_code auth_reason_code2 = am::make_error_code(am::auth_reason_code::continue_authentication);
    BOOST_TEST(auth_reason_code2 == am::auth_reason_code::continue_authentication);
    BOOST_CHECK(!auth_reason_code2);
    am::error_code auth_reason_code3 = am::make_error_code(am::auth_reason_code::re_authenticate);
    BOOST_TEST(auth_reason_code3 == am::auth_reason_code::re_authenticate);
    BOOST_CHECK(!auth_reason_code3);

    // should not equal even if the value is both 0, because of the different category.
    BOOST_TEST(success != connect_return_code1);
    BOOST_TEST(success != suback_return_code1);
    BOOST_TEST(success != suback_return_code2);
    BOOST_TEST(success != suback_return_code3);
    BOOST_TEST(success != connect_reason_code1);
    BOOST_TEST(success != disconnect_reason_code1);
    BOOST_TEST(success != suback_reason_code1);
    BOOST_TEST(success != unsuback_reason_code1);
    BOOST_TEST(success != puback_reason_code1);
    BOOST_TEST(success != pubrec_reason_code1);
    BOOST_TEST(success != pubrel_reason_code1);
    BOOST_TEST(success != pubcomp_reason_code1);
    BOOST_TEST(success != auth_reason_code1);
}

BOOST_AUTO_TEST_CASE(category_detail) {
    {
        am::error_code e = am::make_error_code(am::mqtt_error::partial_error_detected);
        BOOST_TEST(e.message() == "partial_error_detected");
    }
    {
        am::error_code e = am::make_error_code(am::mqtt_error::packet_identifier_conflict);
        BOOST_TEST(e.message() == "packet_identifier_conflict");
    }
    {
        am::error_code e = am::make_error_code(am::mqtt_error::packet_not_allowed_to_send);
        BOOST_TEST(e.message() == "packet_not_allowed_to_send");
    }
    {
        am::error_code e = am::make_error_code(am::mqtt_error::packet_not_allowed_to_store);
        BOOST_TEST(e.message() == "packet_not_allowed_to_store");
    }
    {
        am::error_code e = am::make_error_code(am::mqtt_error::packet_not_regulated);
        BOOST_TEST(e.message() == "packet_not_regulated");
    }
    {
        std::stringstream ss;
        auto e = am::mqtt_error::partial_error_detected;
        ss << e;
        BOOST_TEST(ss.str() == "partial_error_detected");
    }
    {
        auto se = am::system_error{
            am::make_error_code(
                am::pubrel_reason_code::success
            )
        };
        BOOST_TEST(se.code() == am::pubrel_reason_code::success);
    }
    {
        auto se = am::system_error{
            am::make_error_code(
                am::auth_reason_code::success
            )
        };
        BOOST_TEST(se.code() == am::auth_reason_code::success);
    }
}

BOOST_AUTO_TEST_SUITE_END()
