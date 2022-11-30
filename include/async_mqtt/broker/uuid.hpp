// Copyright Takatoshi Kondo 2021
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_UUID_HPP)
#define ASYNC_MQTT_BROKER_UUID_HPP

#include <string>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>


namespace async_mqtt {

inline std::string create_uuid_string() {
    // See https://github.com/boostorg/uuid/issues/121
    thread_local boost::uuids::random_generator gen;
    return boost::uuids::to_string(gen());
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_UUID_HPP
