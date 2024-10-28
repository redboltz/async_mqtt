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

namespace detail {

template <protocol_version Version, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
client_impl<Version, NextLayer>::recv_loop(this_type_sp impl) {
    BOOST_ASSERT(impl);
    auto& a_impl{*impl};
    a_impl.ep_.async_recv(
        [impl = force_move(impl)]
        (error_code const& ec, std::optional<packet_variant> pv_opt) mutable {
            if (ec) {
                impl->recv_queue_.emplace_back(ec);
                impl->recv_queue_inserted_  = true;
                impl->tim_notify_publish_recv_.cancel();
                impl->pid_tim_pv_res_col_.clear();
                return;
            }
            BOOST_ASSERT(pv_opt);
            pv_opt->visit(
                overload {
                    [&](typename client_type::connack_packet& p) {
                        auto& idx = impl->pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(0);
                        if (it != idx.end()) {
                            const_cast<std::optional<packet_variant>&>(it->pv).emplace(p);
                            it->tim->cancel();
                        }
                    },
                    [&](typename client_type::suback_packet& p) {
                        auto& idx = impl->pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<packet_variant>&>(it->pv).emplace(p);
                            it->tim->cancel();
                        }
                    },
                    [&](typename client_type::unsuback_packet& p) {
                        auto& idx = impl->pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<packet_variant>&>(it->pv).emplace(p);
                            it->tim->cancel();
                        }
                    },
                    [&](typename client_type::publish_packet& p) {
                        impl->recv_queue_.emplace_back(force_move(p));
                        impl->recv_queue_inserted_  = true;
                        impl->tim_notify_publish_recv_.cancel();
                    },
                    [&](typename client_type::puback_packet& p) {
                        auto& idx = impl->pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<typename client_type::puback_packet>&>(
                                it->res.puback_opt
                            ).emplace(p);
                            it->tim->cancel();
                        }
                    },
                    [&](typename client_type::pubrec_packet& p) {
                        auto& idx = impl->pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<typename client_type::pubrec_packet>&>(
                                it->res.pubrec_opt
                            ).emplace(p);
                            if constexpr (Version == protocol_version::v5) {
                                if (make_error_code(p.code())) {
                                    it->tim->cancel();
                                }
                            }
                        }
                    },
                    [&](typename client_type::pubcomp_packet& p) {
                        auto& idx = impl->pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<typename client_type::pubcomp_packet>&>(
                                it->res.pubcomp_opt
                            ).emplace(p);
                            it->tim->cancel();
                        }
                    },
                    [&](typename client_type::disconnect_packet& p) {
                        impl->recv_queue_.emplace_back(force_move(p));
                        impl->recv_queue_inserted_  = true;
                        impl->tim_notify_publish_recv_.cancel();
                    },
                    [&](v5::auth_packet& p) {
                        impl->recv_queue_.emplace_back(force_move(p));
                        impl->recv_queue_inserted_  = true;
                        impl->tim_notify_publish_recv_.cancel();
                    },
                    [&](auto const&) {
                    }
                }
            );
            recv_loop(force_move(impl));
        }
    );
}

} // namespace detail

// member functions

template <protocol_version Version, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
client<Version, NextLayer>::recv_loop() {
    BOOST_ASSERT(impl_);
    impl_type::recv_loop(impl_);
}

} // namespace async_mqtt

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


#endif // ASYNC_MQTT_CLIENT_IMPL_IPP
