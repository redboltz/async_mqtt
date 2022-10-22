// Copyright Takatoshi Kondo 2021
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <string_view>
#include <sstream>

#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/packet/connect_return_code.hpp>
#include <async_mqtt/packet/control_packet_type.hpp>
#include <async_mqtt/packet/pubopts.hpp>
#include <async_mqtt/packet/reason_code.hpp>
#include <async_mqtt/packet/suback_return_code.hpp>
#include <async_mqtt/packet/subopts.hpp>

BOOST_AUTO_TEST_SUITE(ut_code)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE( connect_return_code ) {
    {
        auto c = am::connect_return_code::accepted;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("accepted"));
    }
    {
        auto c = am::connect_return_code::unacceptable_protocol_version;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unacceptable_protocol_version"));
    }
    {
        auto c = am::connect_return_code::identifier_rejected;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("identifier_rejected"));
    }
    {
        auto c = am::connect_return_code::server_unavailable;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("server_unavailable"));
    }
    {
        auto c = am::connect_return_code::bad_user_name_or_password;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("bad_user_name_or_password"));
    }
    {
        auto c = am::connect_return_code::not_authorized;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("not_authorized"));
    }
    {
        auto c = am::connect_return_code(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unknown_connect_return_code"));
    }
}

BOOST_AUTO_TEST_CASE( control_packet_type ) {
    {
        auto c = am::control_packet_type::connect;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("connect"));
    }
    {
        auto c = am::control_packet_type::connack;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("connack"));
    }
    {
        auto c = am::control_packet_type::publish;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("publish"));
    }
    {
        auto c = am::control_packet_type::puback;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("puback"));
    }
    {
        auto c = am::control_packet_type::pubrec;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("pubrec"));
    }
    {
        auto c = am::control_packet_type::pubrel;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("pubrel"));
    }
    {
        auto c = am::control_packet_type::pubcomp;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("pubcomp"));
    }
    {
        auto c = am::control_packet_type::subscribe;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("subscribe"));
    }
    {
        auto c = am::control_packet_type::suback;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("suback"));
    }
    {
        auto c = am::control_packet_type::unsubscribe;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unsubscribe"));
    }
    {
        auto c = am::control_packet_type::unsuback;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unsuback"));
    }
    {
        auto c = am::control_packet_type::pingreq;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("pingreq"));
    }
    {
        auto c = am::control_packet_type::pingresp;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("pingresp"));
    }
    {
        auto c = am::control_packet_type::auth;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("auth"));
    }
    {
        auto c = am::control_packet_type(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unknown_control_packet_type"));
    }
}

BOOST_AUTO_TEST_CASE( protocol_version ) {
    {
        auto c = am::protocol_version::undetermined;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("undetermined"));
    }
    {
        auto c = am::protocol_version::v3_1_1;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("v3_1_1"));
    }
    {
        auto c = am::protocol_version::v5;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("v5"));
    }
    {
        auto c = am::protocol_version(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unknown_protocol_version"));
    }
}

BOOST_AUTO_TEST_CASE( publish ) {
    // retain
    {
        auto c = am::pub::retain::yes;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("yes"));
    }
    {
        auto c = am::pub::retain::no;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("no"));
    }
    {
        auto c = am::pub::retain(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("invalid_retain"));
    }
    // dup
    {
        auto c = am::pub::dup::yes;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("yes"));
    }
    {
        auto c = am::pub::dup::no;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("no"));
    }
    {
        auto c = am::pub::dup(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("invalid_dup"));
    }
}

BOOST_AUTO_TEST_CASE( suback_return_code ) {
    {
        auto c = am::suback_return_code::success_maximum_qos_0;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("success_maximum_qos_0"));
    }
    {
        auto c = am::suback_return_code::success_maximum_qos_1;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("success_maximum_qos_1"));
    }
    {
        auto c = am::suback_return_code::success_maximum_qos_2;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("success_maximum_qos_2"));
    }
    {
        auto c = am::suback_return_code::failure;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("failure"));
    }
    {
        auto c = am::suback_return_code(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unknown_suback_return_code"));
    }
}

BOOST_AUTO_TEST_CASE( connect_reason_code ) {
    {
        auto c = am::connect_reason_code::success;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("success"));
    }
    {
        auto c = am::connect_reason_code::unspecified_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unspecified_error"));
    }
    {
        auto c = am::connect_reason_code::malformed_packet;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("malformed_packet"));
    }
    {
        auto c = am::connect_reason_code::protocol_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("protocol_error"));
    }
    {
        auto c = am::connect_reason_code::implementation_specific_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("implementation_specific_error"));
    }
    {
        auto c = am::connect_reason_code::unsupported_protocol_version;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unsupported_protocol_version"));
    }
    {
        auto c = am::connect_reason_code::client_identifier_not_valid;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("client_identifier_not_valid"));
    }
    {
        auto c = am::connect_reason_code::bad_user_name_or_password;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("bad_user_name_or_password"));
    }
    {
        auto c = am::connect_reason_code::not_authorized;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("not_authorized"));
    }
    {
        auto c = am::connect_reason_code::server_unavailable;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("server_unavailable"));
    }
    {
        auto c = am::connect_reason_code::server_busy;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("server_busy"));
    }
    {
        auto c = am::connect_reason_code::banned;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("banned"));
    }
    {
        auto c = am::connect_reason_code::server_shutting_down;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("server_shutting_down"));
    }
    {
        auto c = am::connect_reason_code::bad_authentication_method;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("bad_authentication_method"));
    }
    {
        auto c = am::connect_reason_code::topic_name_invalid;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("topic_name_invalid"));
    }
    {
        auto c = am::connect_reason_code::packet_too_large;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("packet_too_large"));
    }
    {
        auto c = am::connect_reason_code::quota_exceeded;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("quota_exceeded"));
    }
    {
        auto c = am::connect_reason_code::payload_format_invalid;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("payload_format_invalid"));
    }
    {
        auto c = am::connect_reason_code::retain_not_supported;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("retain_not_supported"));
    }
    {
        auto c = am::connect_reason_code::qos_not_supported;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("qos_not_supported"));
    }
    {
        auto c = am::connect_reason_code::use_another_server;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("use_another_server"));
    }
    {
        auto c = am::connect_reason_code::server_moved;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("server_moved"));
    }
    {
        auto c = am::connect_reason_code::connection_rate_exceeded;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("connection_rate_exceeded"));
    }
    {
        auto c = am::connect_reason_code(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unknown_connect_reason_code"));
    }
}

BOOST_AUTO_TEST_CASE( disconnect_reason_code ) {
    {
        auto c = am::disconnect_reason_code::normal_disconnection;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("normal_disconnection"));
    }
    {
        auto c = am::disconnect_reason_code::disconnect_with_will_message;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("disconnect_with_will_message"));
    }
    {
        auto c = am::disconnect_reason_code::unspecified_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unspecified_error"));
    }
    {
        auto c = am::disconnect_reason_code::malformed_packet;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("malformed_packet"));
    }
    {
        auto c = am::disconnect_reason_code::protocol_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("protocol_error"));
    }
    {
        auto c = am::disconnect_reason_code::implementation_specific_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("implementation_specific_error"));
    }
    {
        auto c = am::disconnect_reason_code::not_authorized;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("not_authorized"));
    }
    {
        auto c = am::disconnect_reason_code::server_busy;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("server_busy"));
    }
    {
        auto c = am::disconnect_reason_code::server_shutting_down;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("server_shutting_down"));
    }
    {
        auto c = am::disconnect_reason_code::keep_alive_timeout;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("keep_alive_timeout"));
    }
    {
        auto c = am::disconnect_reason_code::session_taken_over;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("session_taken_over"));
    }
    {
        auto c = am::disconnect_reason_code::topic_filter_invalid;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("topic_filter_invalid"));
    }
    {
        auto c = am::disconnect_reason_code::topic_name_invalid;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("topic_name_invalid"));
    }
    {
        auto c = am::disconnect_reason_code::packet_too_large;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("packet_too_large"));
    }
    {
        auto c = am::disconnect_reason_code::message_rate_too_high;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("message_rate_too_high"));
    }
    {
        auto c = am::disconnect_reason_code::quota_exceeded;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("quota_exceeded"));
    }
    {
        auto c = am::disconnect_reason_code::administrative_action;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("administrative_action"));
    }
    {
        auto c = am::disconnect_reason_code::payload_format_invalid;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("payload_format_invalid"));
    }
    {
        auto c = am::disconnect_reason_code::retain_not_supported;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("retain_not_supported"));
    }
    {
        auto c = am::disconnect_reason_code::qos_not_supported;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("qos_not_supported"));
    }
    {
        auto c = am::disconnect_reason_code::use_another_server;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("use_another_server"));
    }
    {
        auto c = am::disconnect_reason_code::server_moved;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("server_moved"));
    }
    {
        auto c = am::disconnect_reason_code::shared_subscriptions_not_supported;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("shared_subscriptions_not_supported"));
    }
    {
        auto c = am::disconnect_reason_code::connection_rate_exceeded;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("connection_rate_exceeded"));
    }
    {
        auto c = am::disconnect_reason_code::maximum_connect_time;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("maximum_connect_time"));
    }
    {
        auto c = am::disconnect_reason_code::subscription_identifiers_not_supported;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("subscription_identifiers_not_supported"));
    }
    {
        auto c = am::disconnect_reason_code::wildcard_subscriptions_not_supported;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("wildcard_subscriptions_not_supported"));
    }
    {
        auto c = am::disconnect_reason_code(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unknown_disconnect_reason_code"));
    }
}

BOOST_AUTO_TEST_CASE( suback_reason_code ) {
    {
        auto c = am::suback_reason_code::granted_qos_0;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("granted_qos_0"));
    }
    {
        auto c = am::suback_reason_code::granted_qos_1;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("granted_qos_1"));
    }
    {
        auto c = am::suback_reason_code::granted_qos_2;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("granted_qos_2"));
    }
    {
        auto c = am::suback_reason_code::unspecified_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unspecified_error"));
    }
    {
        auto c = am::suback_reason_code::implementation_specific_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("implementation_specific_error"));
    }
    {
        auto c = am::suback_reason_code::not_authorized;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("not_authorized"));
    }
    {
        auto c = am::suback_reason_code::topic_filter_invalid;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("topic_filter_invalid"));
    }
    {
        auto c = am::suback_reason_code::packet_identifier_in_use;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("packet_identifier_in_use"));
    }
    {
        auto c = am::suback_reason_code::quota_exceeded;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("quota_exceeded"));
    }
    {
        auto c = am::suback_reason_code::shared_subscriptions_not_supported;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("shared_subscriptions_not_supported"));
    }
    {
        auto c = am::suback_reason_code::subscription_identifiers_not_supported;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("subscription_identifiers_not_supported"));
    }
    {
        auto c = am::suback_reason_code::wildcard_subscriptions_not_supported;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("wildcard_subscriptions_not_supported"));
    }
    {
        auto c = am::suback_reason_code(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unknown_suback_reason_code"));
    }
}

BOOST_AUTO_TEST_CASE( unsuback_reason_code ) {
    {
        auto c = am::unsuback_reason_code::success;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("success"));
    }
    {
        auto c = am::unsuback_reason_code::no_subscription_existed;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("no_subscription_existed"));
    }
    {
        auto c = am::unsuback_reason_code::unspecified_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unspecified_error"));
    }
    {
        auto c = am::unsuback_reason_code::implementation_specific_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("implementation_specific_error"));
    }
    {
        auto c = am::unsuback_reason_code::not_authorized;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("not_authorized"));
    }
    {
        auto c = am::unsuback_reason_code::topic_filter_invalid;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("topic_filter_invalid"));
    }
    {
        auto c = am::unsuback_reason_code::packet_identifier_in_use;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("packet_identifier_in_use"));
    }
    {
        auto c = am::unsuback_reason_code(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unknown_unsuback_reason_code"));
    }
}

BOOST_AUTO_TEST_CASE( puback_reason_code ) {
    {
        auto c = am::puback_reason_code::success;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("success"));
    }
    {
        auto c = am::puback_reason_code::no_matching_subscribers;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("no_matching_subscribers"));
    }
    {
        auto c = am::puback_reason_code::unspecified_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unspecified_error"));
    }
    {
        auto c = am::puback_reason_code::implementation_specific_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("implementation_specific_error"));
    }
    {
        auto c = am::puback_reason_code::not_authorized;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("not_authorized"));
    }
    {
        auto c = am::puback_reason_code::topic_name_invalid;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("topic_name_invalid"));
    }
    {
        auto c = am::puback_reason_code::packet_identifier_in_use;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("packet_identifier_in_use"));
    }
    {
        auto c = am::puback_reason_code::quota_exceeded;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("quota_exceeded"));
    }
    {
        auto c = am::puback_reason_code::payload_format_invalid;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("payload_format_invalid"));
    }
    {
        auto c = am::puback_reason_code(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unknown_puback_reason_code"));
    }
}

BOOST_AUTO_TEST_CASE( pubrec_reason_code ) {
    {
        auto c = am::pubrec_reason_code::success;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("success"));
    }
    {
        auto c = am::pubrec_reason_code::no_matching_subscribers;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("no_matching_subscribers"));
    }
    {
        auto c = am::pubrec_reason_code::unspecified_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unspecified_error"));
    }
    {
        auto c = am::pubrec_reason_code::implementation_specific_error;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("implementation_specific_error"));
    }
    {
        auto c = am::pubrec_reason_code::not_authorized;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("not_authorized"));
    }
    {
        auto c = am::pubrec_reason_code::topic_name_invalid;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("topic_name_invalid"));
    }
    {
        auto c = am::pubrec_reason_code::packet_identifier_in_use;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("packet_identifier_in_use"));
    }
    {
        auto c = am::pubrec_reason_code::quota_exceeded;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("quota_exceeded"));
    }
    {
        auto c = am::pubrec_reason_code::payload_format_invalid;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("payload_format_invalid"));
    }
    {
        auto c = am::pubrec_reason_code(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unknown_pubrec_reason_code"));
    }
}

BOOST_AUTO_TEST_CASE( pubrel_reason_code ) {
    {
        auto c = am::pubrel_reason_code::success;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("success"));
    }
    {
        auto c = am::pubrel_reason_code::packet_identifier_not_found;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("packet_identifier_not_found"));
    }
    {
        auto c = am::pubrel_reason_code(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unknown_pubrel_reason_code"));
    }
}

BOOST_AUTO_TEST_CASE( pubcomp_reason_code ) {
    {
        auto c = am::pubcomp_reason_code::success;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("success"));
    }
    {
        auto c = am::pubcomp_reason_code::packet_identifier_not_found;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("packet_identifier_not_found"));
    }
    {
        auto c = am::pubcomp_reason_code(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unknown_pubcomp_reason_code"));
    }
}

BOOST_AUTO_TEST_CASE( auth_reason_code ) {
    {
        auto c = am::auth_reason_code::success;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("success"));
    }
    {
        auto c = am::auth_reason_code::continue_authentication;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("continue_authentication"));
    }
    {
        auto c = am::auth_reason_code::re_authenticate;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("re_authenticate"));
    }
    {
        auto c = am::auth_reason_code(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("unknown_auth_reason_code"));
    }
}

BOOST_AUTO_TEST_CASE( subscribe_options ) {
    // retain_handling
    {
        auto c = am::sub::retain_handling::send;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("send"));
    }
    {
        auto c = am::sub::retain_handling::send_only_new_subscription;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("send_only_new_subscription"));
    }
    {
        auto c = am::sub::retain_handling::not_send;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("not_send"));
    }
    {
        auto c = am::sub::retain_handling(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("invalid_retain_handling"));
    }
    // rap
    {
        auto c = am::sub::rap::dont;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("dont"));
    }
    {
        auto c = am::sub::rap::retain;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("retain"));
    }
    {
        auto c = am::sub::rap(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("invalid_rap"));
    }
    // nl
    {
        auto c = am::sub::nl::no;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("no"));
    }
    {
        auto c = am::sub::nl::yes;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("yes"));
    }
    {
        auto c = am::sub::nl(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("invalid_nl"));
    }
    // qps
    {
        auto c = am::qos::at_most_once;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("at_most_once"));
    }
    {
        auto c = am::qos::at_least_once;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("at_least_once"));
    }
    {
        auto c = am::qos::exactly_once;
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("exactly_once"));
    }
    {
        auto c = am::qos(99);
        std::stringstream ss;
        ss << c;
        BOOST_TEST(ss.str() == std::string_view("invalid_qos"));
    }
}

BOOST_AUTO_TEST_SUITE_END()
