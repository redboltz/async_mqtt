// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_CLIENT_IMPL_HPP)
#define ASYNC_MQTT_CLIENT_IMPL_HPP

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key.hpp>

#include <async_mqtt/client.hpp>
#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {
namespace mi = boost::multi_index;

// classes

template <protocol_version Version, typename NextLayer>
struct client<Version, NextLayer>::pid_tim_pv_res_col {
    struct elem_type {
        elem_type(
            packet_id_t pid,
            std::shared_ptr<as::steady_timer> tim
        ): pid{pid},
           tim{force_move(tim)}
        {
        }
        elem_type(
            std::shared_ptr<as::steady_timer> tim
        ): tim{force_move(tim)}
        {
        }

        packet_id_t pid = 0;
        std::shared_ptr<as::steady_timer> tim;
        std::optional<packet_variant> pv;
        pubres_t res;
    };

    struct tag_pid {};
    struct tag_tim {};

    auto& get_pid_idx() {
        return elems.template get<tag_pid>();
    }
    auto& get_tim_idx() {
        return elems.template get<tag_tim>();
    }
    using mi_elem_type = mi::multi_index_container<
        elem_type,
        mi::indexed_by<
            mi::ordered_unique<
                mi::tag<tag_pid>,
                mi::key<&elem_type::pid>
            >,
            mi::ordered_unique<
                mi::tag<tag_tim>,
                mi::key<&elem_type::tim>
            >
        >
    >;
    mi_elem_type elems;
};

template <protocol_version Version, typename NextLayer>
struct client<Version, NextLayer>::recv_type {
    recv_type(publish_packet packet)
        :publish_opt{force_move(packet)}
    {
    }
    recv_type(disconnect_packet packet)
        :disconnect_opt{force_move(packet)}
    {
    }
    recv_type(error_code ec)
        :ec{ec}
    {
    }
    error_code ec = error_code{};
    std::optional<publish_packet> publish_opt;
    std::optional<disconnect_packet> disconnect_opt;
};


// member functions

template <protocol_version Version, typename NextLayer>
template <typename... Args>
client<Version, NextLayer>::client(
    Args&&... args
): ep_{ep_type::create(Version, std::forward<Args>(args)...)},
   tim_notify_publish_recv_{ep_->get_executor()}
{
    ep_->set_auto_pub_response(true);
    ep_->set_auto_ping_response(true);
}

template <protocol_version Version, typename NextLayer>
inline
as::any_io_executor
client<Version, NextLayer>::get_executor() const {
    return ep_->get_executor();
}

template <protocol_version Version, typename NextLayer>
inline
typename client<Version, NextLayer>::next_layer_type const&
client<Version, NextLayer>::next_layer() const {
    return ep_->next_layer();
}

template <protocol_version Version, typename NextLayer>
inline
typename client<Version, NextLayer>::next_layer_type&
client<Version, NextLayer>::next_layer() {
    return ep_->next_layer();
}

template <protocol_version Version, typename NextLayer>
inline
typename client<Version, NextLayer>::lowest_layer_type const&
client<Version, NextLayer>::lowest_layer() const {
    return ep_->lowest_layer();
}

template <protocol_version Version, typename NextLayer>
inline
typename client<Version, NextLayer>::lowest_layer_type&
client<Version, NextLayer>::lowest_layer() {
    return ep_->lowest_layer();
}

template <protocol_version Version, typename NextLayer>
inline
void
client<Version, NextLayer>::set_auto_map_topic_alias_send(bool val) {
    ep_->set_auto_map_topic_alias_send(val);
}

template <protocol_version Version, typename NextLayer>
inline
void
client<Version, NextLayer>::set_auto_replace_topic_alias_send(bool val) {
    ep_->set_auto_replace_topic_alias_send(val);
}

template <protocol_version Version, typename NextLayer>
inline
void
client<Version, NextLayer>::set_pingresp_recv_timeout_ms(std::size_t ms) {
    ep_->set_pingresp_recv_timeout_ms(ms);
}

template <protocol_version Version, typename NextLayer>
inline
void
client<Version, NextLayer>::set_bulk_write(bool val) {
    ep_->set_bulk_write(val);
}

template <protocol_version Version, typename NextLayer>
inline
void
client<Version, NextLayer>::recv_loop() {
    ep_->recv(
        [this]
        (packet_variant pv) mutable {
            pv.visit(
                overload {
                    [&](connack_packet& p) {
                        auto& idx = pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(0);
                        if (it != idx.end()) {
                            const_cast<std::optional<packet_variant>&>(it->pv).emplace(p);
                            it->tim->cancel();
                            recv_loop();
                        }
                    },
                    [&](suback_packet& p) {
                        auto& idx = pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<packet_variant>&>(it->pv).emplace(p);
                            it->tim->cancel();
                        }
                        recv_loop();
                    },
                    [&](unsuback_packet& p) {
                        auto& idx = pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<packet_variant>&>(it->pv).emplace(p);
                            it->tim->cancel();
                        }
                        recv_loop();
                    },
                    [&](publish_packet& p) {
                        recv_queue_.emplace_back(force_move(p));
                        recv_queue_inserted_  = true;
                        tim_notify_publish_recv_.cancel();
                        recv_loop();
                    },
                    [&](puback_packet& p) {
                        auto& idx = pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<puback_packet>&>(it->res.puback_opt).emplace(p);
                            it->tim->cancel();
                        }
                        recv_loop();
                    },
                    [&](pubrec_packet& p) {
                        auto& idx = pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<pubrec_packet>&>(it->res.pubrec_opt).emplace(p);
                            if constexpr (Version == protocol_version::v5) {
                                if (is_error(p.code())) {
                                    it->tim->cancel();
                                }
                            }
                        }
                        recv_loop();
                    },
                    [&](pubcomp_packet& p) {
                        auto& idx = pid_tim_pv_res_col_.get_pid_idx();
                        auto it = idx.find(p.packet_id());
                        if (it != idx.end()) {
                            const_cast<std::optional<pubcomp_packet>&>(it->res.pubcomp_opt).emplace(p);
                            it->tim->cancel();
                        }
                        recv_loop();
                    },
                    [&](disconnect_packet& p) {
                        recv_queue_.emplace_back(force_move(p));
                        recv_queue_inserted_  = true;
                        tim_notify_publish_recv_.cancel();
                        recv_loop();
                    },
                    [&](system_error const& se) {
                        recv_queue_.emplace_back(se.code());
                        recv_queue_inserted_  = true;
                        tim_notify_publish_recv_.cancel();
                    },
                    [&](auto const&) {
                        recv_loop();
                    }
                }
            );
        }
    );
}

} // namespace async_mqtt


#endif // ASYNC_MQTT_CLIENT_IMPL_HPP
