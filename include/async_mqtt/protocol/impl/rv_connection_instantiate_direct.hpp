// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_IMPL_RV_CONNECTION_INSTANTIATE_DIRECT_HPP)
#define ASYNC_MQTT_PROTOCOL_IMPL_RV_CONNECTION_INSTANTIATE_DIRECT_HPP

#include <async_mqtt/protocol/rv_connection.hpp>

#include <async_mqtt/detail/instantiate_helper.hpp>

#define ASYNC_MQTT_INSTANTIATE_EACH(a_role, a_size) \
namespace async_mqtt { \
template \
class basic_rv_connection<a_role, a_size>; \
} // namespace async_mqtt

#define ASYNC_MQTT_PP_GENERATE(r, product) \
    BOOST_PP_EXPAND( \
        ASYNC_MQTT_INSTANTIATE_EACH \
        BOOST_PP_SEQ_TO_TUPLE( \
            product \
        ) \
    )

BOOST_PP_SEQ_FOR_EACH_PRODUCT(ASYNC_MQTT_PP_GENERATE, (ASYNC_MQTT_PP_ROLE)(ASYNC_MQTT_PP_SIZE))

#undef ASYNC_MQTT_PP_GENERATE
#undef ASYNC_MQTT_INSTANTIATE_EACH

#endif // ASYNC_MQTT_PROTOCOL_IMPL_RV_CONNECTION_INSTANTIATE_DIRECT_HPP
