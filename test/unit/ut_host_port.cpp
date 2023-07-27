// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <vector>
#include <async_mqtt/host_port.hpp>

BOOST_AUTO_TEST_SUITE(ut_host_port)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(basic) {
    {
        auto hp = am::host_port("hostname", 1234);
        BOOST_TEST(hp.host == "hostname");
        BOOST_TEST(hp.port == 1234);
        BOOST_TEST(am::to_string(hp) == "hostname:1234");
    }
    {
        auto hp = am::host_port("123.45.67.89", 1234);
        BOOST_TEST(hp.host == "123.45.67.89");
        BOOST_TEST(hp.port == 1234);
        BOOST_TEST(am::to_string(hp) == "123.45.67.89:1234");
    }
    {
        auto hp = am::host_port("1fff:0:a88:85a3::ac1f", 1234);
        BOOST_TEST(hp.host == "1fff:0:a88:85a3::ac1f");
        BOOST_TEST(hp.port == 1234);
        // intentionally not [1fff:0:a88:85a3::ac1f]:1234
        // because it is not a URL
        BOOST_TEST(am::to_string(hp) == "1fff:0:a88:85a3::ac1f:1234");
    }
}

BOOST_AUTO_TEST_CASE(from_string) {
    {
        auto hp = am::host_port_from_string("hostname:1234");
        BOOST_TEST(!!hp);
        BOOST_TEST(*hp == am::host_port("hostname", 1234));
    }
    {
        auto hp = am::host_port_from_string("123.45.67.89:1234");
        BOOST_TEST(!!hp);
        BOOST_TEST(*hp == am::host_port("123.45.67.89", 1234));
    }
    {
        auto hp = am::host_port_from_string("1fff:0:a88:85a3::ac1f:1234");
        BOOST_TEST(!!hp);
        BOOST_TEST(*hp == am::host_port("1fff:0:a88:85a3::ac1f", 1234));
    }
    {
        auto hp = am::host_port_from_string("[1fff:0:a88:85a3::ac1f]:1234");
        BOOST_TEST(!!hp);
        BOOST_TEST(*hp == am::host_port("1fff:0:a88:85a3::ac1f", 1234));
    }
}

BOOST_AUTO_TEST_CASE(from_string_invalid) {
    {
        auto hp = am::host_port_from_string("");
        BOOST_TEST(!hp);
    }
    {
        auto hp = am::host_port_from_string("1");
        BOOST_TEST(!hp);
    }
    {
        auto hp = am::host_port_from_string(":1");
        BOOST_TEST(!hp);
    }
    {
        auto hp = am::host_port_from_string("a:a");
        BOOST_TEST(!hp);
    }
}

BOOST_AUTO_TEST_SUITE_END()
