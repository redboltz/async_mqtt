// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_RECV_IPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_RECV_IPP

#include <async_mqtt/impl/endpoint_recv.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
recv_op::
send_publish_from_queue() {
    if (ep->status_ != connection_status::connected) return;
    while (!ep->publish_queue_.empty() &&
           ep->publish_send_count_ != ep->publish_send_max_) {
        async_send(
            ep,
            force_move(ep->publish_queue_.front()),
            true, // from queue
            as::detached
        );
        ep->publish_queue_.pop_front();
    }
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
recv_op::
process_qos2_publish(
    protocol_version ver,
    typename basic_packet_id_type<PacketIdBytes>::type packet_id
) {
    bool already_handled = false;
    if (ep->qos2_publish_handled_.find(packet_id) == ep->qos2_publish_handled_.end()) {
        ep->qos2_publish_handled_.emplace(packet_id);
    }
    else {
        already_handled = true;
    }
    if (ep->status_ == connection_status::connected &&
        (ep->auto_pub_response_ ||
         already_handled) // already_handled is true only if the pubrec packet
    ) {                   // corresponding to the publish packet has already
        // been sent as success
        switch (ver) {
        case protocol_version::v3_1_1:
            async_send(
                ep,
                v3_1_1::basic_pubrec_packet<PacketIdBytes>(packet_id),
                as::detached
            );
            break;
        case protocol_version::v5:
            ep->async_send(
                ep,
                v5::basic_pubrec_packet<PacketIdBytes>(packet_id),
                as::detached
            );
            break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }
    if (already_handled) {
        return false;
    }
    return true;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
recv_op::
handle_v3_1_1_connect(v3_1_1::connect_packet& p) {
    ep->initialize();
    ep->protocol_version_ = protocol_version::v3_1_1;
    ep->status_ = connection_status::connecting;
    auto keep_alive = p.keep_alive();
    if (keep_alive != 0) {
        ep->pingreq_recv_timeout_ms_.emplace(
            std::chrono::milliseconds{
                keep_alive * 1000 * 3 / 2
            }
        );
    }
    if (p.clean_session()) {
        ep->need_store_ = false;
    }
    else {
        ep->need_store_ = true;
    }
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
recv_op::
handle_v5_connect(v5::connect_packet& p) {
    ep->initialize();
    ep->protocol_version_ = protocol_version::v5;
    ep->status_ = connection_status::connecting;
    auto keep_alive = p.keep_alive();
    if (keep_alive != 0) {
        ep->pingreq_recv_timeout_ms_.emplace(
            std::chrono::milliseconds{
                keep_alive * 1000 * 3 / 2
            }
        );
    }
    for (auto const& prop : p.props()) {
        prop.visit(
            overload {
                [&](property::topic_alias_maximum const& p) {
                    if (p.val() > 0) {
                        ep->topic_alias_send_.emplace(p.val());
                    }
                },
                [&](property::receive_maximum const& p) {
                    BOOST_ASSERT(p.val() != 0);
                    ep->publish_send_max_ = p.val();
                },
                [&](property::maximum_packet_size const& p) {
                    BOOST_ASSERT(p.val() != 0);
                    ep->maximum_packet_size_send_ = p.val();
                },
                [&](property::session_expiry_interval const& p) {
                    if (p.val() != 0) {
                        ep->need_store_ = true;
                    }
                },
                [](auto const&) {
                }
            }
        );
    }
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
recv_op::
handle_v3_1_1_connack(v3_1_1::connack_packet& p) {
    if (p.code() == connect_return_code::accepted) {
        ep->status_ = connection_status::connected;
        if (p.session_present()) {
            send_stored(ep);
        }
        else {
            ep->clear_pid_man();
            ep->store_.clear();
        }
    }
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
recv_op::
handle_v5_connack(v5::connack_packet& p) {
    if (p.code() == connect_reason_code::success) {
        ep->status_ = connection_status::connected;
         for (auto const& prop : p.props()) {
            prop.visit(
                overload {
                    [&](property::topic_alias_maximum const& p) {
                        if (p.val() > 0) {
                            ep->topic_alias_send_.emplace(p.val());
                        }
                    },
                    [&](property::receive_maximum const& p) {
                        BOOST_ASSERT(p.val() != 0);
                        ep->publish_send_max_ = p.val();
                    },
                    [&](property::maximum_packet_size const& p) {
                        BOOST_ASSERT(p.val() != 0);
                        ep->maximum_packet_size_send_ = p.val();
                    },
                    [&](property::server_keep_alive const& p) {
                        if constexpr (can_send_as_client(Role)) {
                            set_pingreq_send_interval(
                                ep,
                                std::chrono::seconds{
                                    p.val()
                                }
                            );
                        }
                    },
                    [](auto const&) {
                    }
                }
            );
        }

        if (p.session_present()) {
            send_stored(ep);
        }
        else {
            ep->clear_pid_man();
            ep->store_.clear();
        }
    }
}

} // namespace async_mqtt::detail

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#include <async_mqtt/detail/instantiate_helper.hpp>

#define ASYNC_MQTT_INSTANTIATE_EACH(a_role, a_size, a_protocol) \
namespace async_mqtt::detail { \
\
template \
void \
basic_endpoint_impl<a_role, a_size, a_protocol>::recv_op:: \
send_publish_from_queue(); \
\
template \
bool \
basic_endpoint_impl<a_role, a_size, a_protocol>::recv_op:: \
process_qos2_publish( \
    protocol_version, \
    typename basic_packet_id_type<a_size>::type \
); \
\
template \
void \
basic_endpoint_impl<a_role, a_size, a_protocol>::recv_op:: \
handle_v3_1_1_connect(v3_1_1::connect_packet&); \
\
template \
void \
basic_endpoint_impl<a_role, a_size, a_protocol>::recv_op:: \
handle_v3_1_1_connack(v3_1_1::connack_packet&); \
\
template \
void \
basic_endpoint_impl<a_role, a_size, a_protocol>::recv_op:: \
handle_v5_connect(v5::connect_packet&); \
\
template \
void \
basic_endpoint_impl<a_role, a_size, a_protocol>::recv_op:: \
handle_v5_connack(v5::connack_packet&); \
\
} // namespace async_mqtt::detail

#define ASYNC_MQTT_PP_GENERATE(r, product) \
    BOOST_PP_EXPAND( \
        ASYNC_MQTT_INSTANTIATE_EACH \
        BOOST_PP_SEQ_TO_TUPLE( \
            product \
        ) \
    )

BOOST_PP_SEQ_FOR_EACH_PRODUCT(ASYNC_MQTT_PP_GENERATE, (ASYNC_MQTT_PP_ROLE)(ASYNC_MQTT_PP_SIZE)(ASYNC_MQTT_PP_PROTOCOL))

#undef ASYNC_MQTT_PP_GENERATE
#undef ASYNC_MQTT_INSTANTIATE_EACH

#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RECV_IPP
