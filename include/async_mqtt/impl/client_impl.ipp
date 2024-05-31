// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_CLIENT_IMPL_IPP)
#define ASYNC_MQTT_CLIENT_IMPL_IPP

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key.hpp>

#include <async_mqtt/client.hpp>
#include <async_mqtt/endpoint.hpp>

#include <async_mqtt/util/inline.hpp>


namespace async_mqtt {
namespace mi = boost::multi_index;

// member functions

template <protocol_version Version, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
client<Version, NextLayer>::recv_loop() {
    ep_->async_recv(
        [this]
        (error_code const& ec, packet_variant pv) mutable {
            if (ec) {
                recv_queue_.emplace_back(ec);
                recv_queue_inserted_  = true;
                tim_notify_publish_recv_.cancel();
                return;
            }
            pv.visit(
                overload {
                    [&](connack_packet& p) {
                        auto& idx = pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(0);
                        if (it != idx.end()) {
                            const_cast<std::optional<packet_variant>&>(it->pv).emplace(p);
                            it->tim->cancel();
                        }
                    },
                    [&](suback_packet& p) {
                        auto& idx = pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<packet_variant>&>(it->pv).emplace(p);
                            it->tim->cancel();
                        }
                    },
                    [&](unsuback_packet& p) {
                        auto& idx = pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<packet_variant>&>(it->pv).emplace(p);
                            it->tim->cancel();
                        }
                    },
                    [&](publish_packet& p) {
                        recv_queue_.emplace_back(force_move(p));
                        recv_queue_inserted_  = true;
                        tim_notify_publish_recv_.cancel();
                    },
                    [&](puback_packet& p) {
                        auto& idx = pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<puback_packet>&>(it->res.puback_opt).emplace(p);
                            it->tim->cancel();
                        }
                    },
                    [&](pubrec_packet& p) {
                        auto& idx = pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<pubrec_packet>&>(it->res.pubrec_opt).emplace(p);
                            if constexpr (Version == protocol_version::v5) {
                                if (make_error_code(p.code())) {
                                    it->tim->cancel();
                                }
                            }
                        }
                    },
                    [&](pubcomp_packet& p) {
                        auto& idx = pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<pubcomp_packet>&>(it->res.pubcomp_opt).emplace(p);
                            it->tim->cancel();
                        }
                    },
                    [&](disconnect_packet& p) {
                        recv_queue_.emplace_back(force_move(p));
                        recv_queue_inserted_  = true;
                        tim_notify_publish_recv_.cancel();
                    },
                    [&](v5::auth_packet& p) {
                        recv_queue_.emplace_back(force_move(p));
                        recv_queue_inserted_  = true;
                        tim_notify_publish_recv_.cancel();
                    },
                    [&](auto const&) {
                    }
                }
            );
            recv_loop();
        }
    );
}

} // namespace async_mqtt

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#include <async_mqtt/detail/instantiate_helper.hpp>


#define ASYNC_MQTT_INSTANTIATE_EACH(a_version, a_protocol) \
namespace async_mqtt { \
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


#endif // ASYNC_MQTT_CLIENT_IMPL_IPP
