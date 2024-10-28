// Copyright Takatoshi Kondo 2025
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_SEPARATE_SRC_CLIENT_HPP)
#define ASYNC_MQTT_SEPARATE_SRC_CLIENT_HPP

#include <async_mqtt/separate/src_endpoint.hpp>

#include <async_mqtt/asio_bind/impl/client_acquire_unique_packet_id.ipp>
#include <async_mqtt/asio_bind/impl/client_acquire_unique_packet_id.ipp>
#include <async_mqtt/asio_bind/impl/client_acquire_unique_packet_id_wait_until.ipp>
#include <async_mqtt/asio_bind/impl/client_auth.ipp>
#include <async_mqtt/asio_bind/impl/client_close.ipp>
#include <async_mqtt/asio_bind/impl/client_disconnect.ipp>
#include <async_mqtt/asio_bind/impl/client_misc.ipp>
#include <async_mqtt/asio_bind/impl/client_publish.ipp>
#include <async_mqtt/asio_bind/impl/client_recv.ipp>
#include <async_mqtt/asio_bind/impl/client_register_packet_id.ipp>
#include <async_mqtt/asio_bind/impl/client_release_packet_id.ipp>
#include <async_mqtt/asio_bind/impl/client_start.ipp>
#include <async_mqtt/asio_bind/impl/client_subscribe.ipp>
#include <async_mqtt/asio_bind/impl/client_unsubscribe.ipp>

#include <async_mqtt/asio_bind/impl/client_instantiate_direct.hpp>

#endif // ASYNC_MQTT_SEPARATE_SRC_CLIENT_HPP
