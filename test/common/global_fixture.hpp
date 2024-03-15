// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_GLOBAL_FIXTURE_HPP)
#define ASYNC_MQTT_GLOBAL_FIXTURE_HPP

#include <string_view>

#include <boost/test/unit_test.hpp>

#include <async_mqtt/setup_log.hpp>

namespace am = async_mqtt;

struct global_fixture {
    void setup() {
        auto sev =
            [&] {
                auto argc = boost::unit_test::framework::master_test_suite().argc;
                if (argc >= 2) {
                    using namespace std::string_view_literals;
                    auto argv = boost::unit_test::framework::master_test_suite().argv;
                    auto sevstr = std::string_view(argv[1]);
                    auto sl_opt = am::severity_level_from_string(sevstr);
                    if (sl_opt) return *sl_opt;
                }
                return async_mqtt::severity_level::warning;
            } ();
        async_mqtt::setup_log(sev);
    }
    void teardown() {
    }
};

BOOST_TEST_GLOBAL_FIXTURE(global_fixture);

#endif // ASYNC_MQTT_GLOBAL_FIXTURE_HPP
