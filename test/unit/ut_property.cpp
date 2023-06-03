// Copyright Takatoshi Kondo 2019
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <iterator>

#include <boost/lexical_cast.hpp> // for operator<<() test

#include <async_mqtt/util/optional.hpp>
#include <async_mqtt/packet/property.hpp>
#include <async_mqtt/packet//property_variant.hpp>

BOOST_AUTO_TEST_SUITE(ut_property)

namespace am = async_mqtt;
using namespace am::literals;

BOOST_AUTO_TEST_CASE( payload_format_indicator ) {
    am::property::payload_format_indicator v1 { am::payload_format::binary };
    am::property::payload_format_indicator v2 { am::payload_format::string };

    BOOST_TEST(boost::lexical_cast<std::string>(v1) == "{id:payload_format_indicator,val:binary}");
    BOOST_TEST(boost::lexical_cast<std::string>(v2) == "{id:payload_format_indicator,val:string}");
}

BOOST_AUTO_TEST_CASE( message_expiry_interval ) {
    am::property::message_expiry_interval v { 1234 };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:message_expiry_interval,val:1234}");
}

BOOST_AUTO_TEST_CASE( subscription_identifier ) {
    am::property::subscription_identifier v { 1234 };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:subscription_identifier,val:1234}");
}

BOOST_AUTO_TEST_CASE(session_expiry_interval  ) {
    am::property::session_expiry_interval v { 1234 };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:session_expiry_interval,val:1234}");
}

BOOST_AUTO_TEST_CASE( server_keep_alive ) {
    am::property::server_keep_alive v { 1234 };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:server_keep_alive,val:1234}");
}

BOOST_AUTO_TEST_CASE( request_problem_information ) {
    am::property::request_problem_information v { true };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:request_problem_information,val:1}");
}

BOOST_AUTO_TEST_CASE( will_delay_interval ) {
    am::property::will_delay_interval v { 1234 };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:will_delay_interval,val:1234}");
}

BOOST_AUTO_TEST_CASE( request_response_information ) {
    am::property::request_response_information v { false };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:request_response_information,val:0}");
}

BOOST_AUTO_TEST_CASE( receive_maximum ) {
    am::property::receive_maximum v { 1234 };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:receive_maximum,val:1234}");
}

BOOST_AUTO_TEST_CASE( topic_alias_maximum ) {
    am::property::topic_alias_maximum v { 1234 };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:topic_alias_maximum,val:1234}");
}

BOOST_AUTO_TEST_CASE( topic_alias ) {
    am::property::topic_alias v { 1234 };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:topic_alias,val:1234}");
}

BOOST_AUTO_TEST_CASE( maximum_qos ) {
    am::property::maximum_qos v { am::qos::at_most_once };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:maximum_qos,val:0}");
}

BOOST_AUTO_TEST_CASE( retain_available ) {
    am::property::retain_available v { true };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:retain_available,val:1}");
}

BOOST_AUTO_TEST_CASE( maximum_packet_size ) {
    am::property::maximum_packet_size v { 1234 };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:maximum_packet_size,val:1234}");
}

BOOST_AUTO_TEST_CASE( wildcard_subscription_available ) {
    am::property::wildcard_subscription_available v { true };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:wildcard_subscription_available,val:1}");
}

BOOST_AUTO_TEST_CASE( subscription_identifier_available ) {
    am::property::subscription_identifier_available v { false };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:subscription_identifier_available,val:0}");
}

BOOST_AUTO_TEST_CASE( shared_subscription_available ) {
    am::property::shared_subscription_available v { true };

    BOOST_TEST(boost::lexical_cast<std::string>(v) == "{id:shared_subscription_available,val:1}");
}

// property has _ref

BOOST_AUTO_TEST_CASE( content_type ) {
    am::property::content_type v1 { "abc"_mb };
    BOOST_TEST(boost::lexical_cast<std::string>(v1) == "{id:content_type,val:abc}");
}

BOOST_AUTO_TEST_CASE( response_topic ) {
    am::property::response_topic v1 { "abc"_mb };

    BOOST_TEST(boost::lexical_cast<std::string>(v1) == "{id:response_topic,val:abc}");
}

BOOST_AUTO_TEST_CASE( correlation_data ) {
    using namespace std::literals::string_view_literals;
    am::property::correlation_data v1 { "a\0bc"_mb };
    BOOST_TEST(boost::lexical_cast<std::string>(v1) == "{id:correlation_data,val:a\\u0000bc}"sv);
    BOOST_TEST(v1.val() == "a\0bc"sv);
}

BOOST_AUTO_TEST_CASE( assigned_client_identifier ) {
    am::property::assigned_client_identifier v1 { "abc"_mb };
    BOOST_TEST(boost::lexical_cast<std::string>(v1) == "{id:assigned_client_identifier,val:abc}");
}

BOOST_AUTO_TEST_CASE( authentication_method ) {
    am::property::authentication_method v1 { "abc"_mb };
    BOOST_TEST(boost::lexical_cast<std::string>(v1) == "{id:authentication_method,val:abc}");
}

BOOST_AUTO_TEST_CASE( authentication_data ) {
    am::property::authentication_data v1 { "abc"_mb };
    BOOST_TEST(boost::lexical_cast<std::string>(v1) == "{id:authentication_data,val:abc}");
}

BOOST_AUTO_TEST_CASE( response_information ) {
    am::property::response_information v1 { "abc"_mb };
    BOOST_TEST(boost::lexical_cast<std::string>(v1) == "{id:response_information,val:abc}");
}

BOOST_AUTO_TEST_CASE( server_reference ) {
    am::property::server_reference v1 { "abc"_mb };
    BOOST_TEST(boost::lexical_cast<std::string>(v1) == "{id:server_reference,val:abc}");
}

BOOST_AUTO_TEST_CASE( reason_string ) {
    am::property::reason_string v1 { "abc"_mb };
    BOOST_TEST(boost::lexical_cast<std::string>(v1) == "{id:reason_string,val:abc}");
}

BOOST_AUTO_TEST_CASE( user_property ) {
    am::property::user_property v1 { "abc"_mb, "def"_mb };
    BOOST_TEST(boost::lexical_cast<std::string>(v1) == "{id:user_property,key:abc,val:def}");
}

BOOST_AUTO_TEST_CASE(comparison) {
    {
        am::property::payload_format_indicator v1 { am::payload_format::binary };
        am::property::payload_format_indicator v2 { am::payload_format::string };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::message_expiry_interval v1 { 1234 };
        am::property::message_expiry_interval v2 { 5678 };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::subscription_identifier v1 { 1234 };
        am::property::subscription_identifier v2 { 5678 };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::session_expiry_interval v1 { 1234 };
        am::property::session_expiry_interval v2 { 5678 };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::server_keep_alive v1 { 1234 };
        am::property::server_keep_alive v2 { 5678 };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::request_problem_information v1 { false };
        am::property::request_problem_information v2 { true };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::will_delay_interval v1 { 1234 };
        am::property::will_delay_interval v2 { 5678 };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::request_response_information v1 { false };
        am::property::request_response_information v2 { true };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::receive_maximum v1 { 1234 };
        am::property::receive_maximum v2 { 5678 };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::topic_alias_maximum v1 { 1234 };
        am::property::topic_alias_maximum v2 { 5678 };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::topic_alias v1 { 1234 };
        am::property::topic_alias v2 { 5678 };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::maximum_qos v1 { am::qos::at_most_once };
        am::property::maximum_qos v2 { am::qos::at_least_once };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::retain_available v1 { false };
        am::property::retain_available v2 { true };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::maximum_packet_size v1 { 1234 };
        am::property::maximum_packet_size v2 { 5678 };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::wildcard_subscription_available v1 { false };
        am::property::wildcard_subscription_available v2 { true };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::subscription_identifier_available v1 { false };
        am::property::subscription_identifier_available v2 { true };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::shared_subscription_available v1 { false };
        am::property::shared_subscription_available v2 { true };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::content_type v1 { "abc"_mb };
        am::property::content_type v2 { "def"_mb };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::response_topic v1 { "abc"_mb };
        am::property::response_topic v2 { "def"_mb };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::correlation_data v1 { "ab\0c"_mb };
        am::property::correlation_data v2 { "ab\0f"_mb };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::assigned_client_identifier v1 { "abc"_mb };
        am::property::assigned_client_identifier v2 { "def"_mb };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::authentication_method v1 { "abc"_mb };
        am::property::authentication_method v2 { "def"_mb };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::authentication_data v1 { "abc"_mb };
        am::property::authentication_data v2 { "def"_mb };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::response_information v1 { "abc"_mb };
        am::property::response_information v2 { "def"_mb };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::server_reference v1 { "abc"_mb };
        am::property::server_reference v2 { "def"_mb };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::reason_string v1 { "abc"_mb };
        am::property::reason_string v2 { "def"_mb };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }
    {
        am::property::user_property v1 { "abc"_mb, "def"_mb };
        am::property::user_property v2 { "abc"_mb, "ghi"_mb };
        BOOST_TEST(v1 == v1);
        BOOST_TEST(v1 != v2);
        BOOST_TEST(v1 < v2);
    }

    {
        am::property::user_property v1 { "abc"_mb, "def"_mb };
        am::property::user_property v2 { "abc"_mb, "ghi"_mb };

        am::properties ps1 { v1, v2 };
        am::properties ps2 { v2, v1 };
        BOOST_TEST(ps1 == ps1);
        BOOST_TEST(ps1 != ps2);
        BOOST_TEST(ps1 < ps2);
    }
}

BOOST_AUTO_TEST_SUITE_END()
