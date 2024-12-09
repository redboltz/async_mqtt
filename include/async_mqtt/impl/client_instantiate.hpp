// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_INSTANTIATE_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_INSTANTIATE_HPP

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#include <async_mqtt/detail/instantiate_helper.hpp>

#define ASYNC_MQTT_INSTANTIATE_EACH(a_version, a_protocol) \
namespace async_mqtt { \
namespace detail { \
template \
class client_impl<a_version, a_protocol>; \
} \
template \
class client<a_version, a_protocol>; \
} // namespace async_mqtt

#define ASYNC_MQTT_PP_GENERATE(r, product) \
    BOOST_PP_EXPAND( \
        ASYNC_MQTT_INSTANTIATE_EACH \
        BOOST_PP_SEQ_TO_TUPLE( \
            product \
        ) \
    )

BOOST_PP_SEQ_FOR_EACH_PRODUCT(ASYNC_MQTT_PP_GENERATE, (ASYNC_MQTT_PP_VERSION)(ASYNC_MQTT_PP_PROTOCOL))

#undef ASYNC_MQTT_PP_GENERATE
#undef ASYNC_MQTT_INSTANTIATE_EACH

#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_CLIENT_INSTANTIATE_HPP
