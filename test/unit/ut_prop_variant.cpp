// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <boost/lexical_cast.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_prop_variant)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(payload_format_indicator) {
    am::property_variant pv1{
        am::property::payload_format_indicator{
            am::payload_format::binary
        }
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:payload_format_indicator,val:binary}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::publish, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(message_expiry_interval) {
    am::property_variant pv1{
        am::property::message_expiry_interval{0xff'ff'ff'ff}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:message_expiry_interval,val:4294967295}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::publish, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(content_type) {
    am::property_variant pv1{
        am::property::content_type{"html"}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:content_type,val:html}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::publish, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(response_topic) {
    am::property_variant pv1{
        am::property::response_topic{"restopic1"}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:response_topic,val:restopic1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::publish, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(correlation_data) {
    am::property_variant pv1{
        am::property::correlation_data{"reqid1"}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:correlation_data,val:reqid1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::publish, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(subscription_identifier) {
    am::property_variant pv1{
        am::property::subscription_identifier{0x0f'ff'ff'ff}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:subscription_identifier,val:268435455}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::publish, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(session_expiry_interval) {
    am::property_variant pv1{
        am::property::session_expiry_interval{0xff'ff'ff'ff}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:session_expiry_interval,val:4294967295}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connect, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(assigned_client_identifier) {
    am::property_variant pv1{
        am::property::assigned_client_identifier{"cid1"}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:assigned_client_identifier,val:cid1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connack, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(server_keep_alive) {
    am::property_variant pv1{
        am::property::server_keep_alive{0xffff}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:server_keep_alive,val:65535}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connack, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(authentication_method) {
    am::property_variant pv1{
        am::property::authentication_method{"basic"}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:authentication_method,val:basic}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connect, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(authentication_data) {
    am::property_variant pv1{
        am::property::authentication_data{"data1"}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:authentication_data,val:data1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connect, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(request_problem_information) {
    am::property_variant pv1{
        am::property::request_problem_information{true}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:request_problem_information,val:1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connect, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(will_delay_interval) {
    am::property_variant pv1{
        am::property::will_delay_interval{0xff'ff'ff'ff}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:will_delay_interval,val:4294967295}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::will, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(request_response_information) {
    am::property_variant pv1{
        am::property::request_response_information{true}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:request_response_information,val:1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connect, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(response_information) {
    am::property_variant pv1{
        am::property::response_information{"restopic1"}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:response_information,val:restopic1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connack, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(server_reference) {
    am::property_variant pv1{
        am::property::server_reference{"server1"}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:server_reference,val:server1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connack, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(reason_string) {
    am::property_variant pv1{
        am::property::reason_string{"reason1"}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:reason_string,val:reason1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connack, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(receive_maximum) {
    am::property_variant pv1{
        am::property::receive_maximum{0xffff}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:receive_maximum,val:65535}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connect, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(topic_alias_maximum) {
    am::property_variant pv1{
        am::property::topic_alias_maximum{0xffff}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:topic_alias_maximum,val:65535}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connect, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(topic_alias) {
    am::property_variant pv1{
        am::property::topic_alias{0xffff}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:topic_alias,val:65535}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::publish, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(maximum_qos) {
    am::property_variant pv1{
        am::property::maximum_qos{am::qos::at_least_once}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:maximum_qos,val:1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connack, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(retain_available) {
    am::property_variant pv1{
        am::property::retain_available{true}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:retain_available,val:1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connack, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(user_property) {
    am::property_variant pv1{
        am::property::user_property{"key1", "val1"}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:user_property,key:key1,val:val1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connack, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
}

BOOST_AUTO_TEST_CASE(maximum_packet_size) {
    am::property_variant pv1{
        am::property::maximum_packet_size{0xff'ff'ff'ff}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:maximum_packet_size,val:4294967295}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connect, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(!ec);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(wildcard_subscription_available) {
    am::property_variant pv1{
        am::property::wildcard_subscription_available{true}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:wildcard_subscription_available,val:1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connack, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(subscription_identifier_available) {
    am::property_variant pv1{
        am::property::subscription_identifier_available{true}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:subscription_identifier_available,val:1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connack, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(shared_subscription_available) {
    am::property_variant pv1{
        am::property::shared_subscription_available{true}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(pv1) ==
        "{id:shared_subscription_available,val:1}"
    );
    am::buffer buf{am::to_string(pv1.const_buffer_sequence())};
    auto wbuf{buf};
    am::error_code ec;
    auto pv2 = am::make_property_variant(wbuf, am::property_location::connack, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(pv1 == pv2);

    // success
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !!am::make_property_variant(wbuf, am::property_location::connack, ec)
        );
        BOOST_TEST(!ec);
    }

    // fail
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::connect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::publish, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::will, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::puback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrec, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubrel, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::pubcomp, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::subscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::suback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsubscribe, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::unsuback, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::disconnect, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        auto wbuf{buf};
        am::error_code ec;
        BOOST_TEST(
            !am::make_property_variant(wbuf, am::property_location::auth, ec)
        );
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(props) {
    am::properties ps1{
        am::property::payload_format_indicator{
            am::payload_format::binary
        },
        am::property::message_expiry_interval{0xff'ff'ff'ff}
    };
    BOOST_TEST(
        boost::lexical_cast<std::string>(ps1) ==
        "[{id:payload_format_indicator,val:binary},{id:message_expiry_interval,val:4294967295}]"
    );
    am::buffer buf{am::to_string(am::const_buffer_sequence(ps1))};
    am::error_code ec;
    auto ps2 = am::make_properties(buf, am::property_location::publish, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(ps1 == ps2);
}

BOOST_AUTO_TEST_SUITE_END()
