// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_STORE_HPP)
#define ASYNC_MQTT_STORE_HPP

#include <boost/asio/steady_timer.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <async_mqtt/log.hpp>
#include <async_mqtt/packet/store_packet_variant.hpp>
#include <async_mqtt/packet/packet_traits.hpp>


namespace async_mqtt {

namespace as = boost::asio;
namespace mi = boost::multi_index;

template <std::size_t PacketIdBytes, typename Executor>
class store {
public:
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;
    using store_packet_t = basic_store_packet_variant<PacketIdBytes>;

    store(Executor exe):exe_{exe}{}

    template <typename Packet>
    bool add(Packet const& packet) {
        if constexpr(is_publish<Packet>()) {
            if (packet.opts().get_qos() == qos::at_least_once ||
                packet.opts().get_qos() == qos::exactly_once) {
                std::uint32_t sec = 0;
                if constexpr(is_v5<Packet>()) {
                    bool finish = false;
                    for (auto const& prop : packet.props()) {
                        prop.visit(
                            overload {
                                [&](property::message_expiry_interval const& p) {
                                    sec = p.val();
                                    finish = true;
                                },
                                [](auto const&) {
                                }
                            }
                        );
                        if (finish) break;
                    }
                }
                if (sec == 0) {
                    return elems_.emplace_back(packet).second;
                }
                else {
                    auto tim = std::make_shared<as::steady_timer>(exe_);
                    tim->expires_after(std::chrono::seconds(sec));
                    tim->async_wait(
                        [this, wp = std::weak_ptr<as::steady_timer>(tim)]
                        (error_code const& ec) {
                            if (auto tim = wp.lock()) {
                                if (!ec) {
                                    auto& idx = elems_.template get<tag_tim>();
                                    auto it = idx.find(tim.get());
                                    if (it == idx.end()) return;
                                    ASYNC_MQTT_LOG("mqtt_impl", info)
                                        << "[store] message expired:" << it->packet;
                                    idx.erase(it);
                                }
                            }
                        }
                    );
                    return elems_.emplace_back(packet, tim).second;
                }
            }
        }
        else if constexpr(is_pubrel<Packet>()) {
            return elems_.emplace_back(packet).second;
        }
        return false;
    }

    bool erase(response_packet r, packet_id_t packet_id) {
        ASYNC_MQTT_LOG("mqtt_impl", info)
            << "[store] erase pid:" << packet_id;
        auto& idx = elems_.template get<tag_res_id>();
        auto it = idx.find(std::make_tuple(r, packet_id));
        if (it == idx.end()) return false;
        idx.erase(it);
        return true;
    }

    void clear() {
        ASYNC_MQTT_LOG("mqtt_impl", info)
            << "[store] clear";
        elems_.clear();
    }

    template <typename Func>
    void for_each(Func const& func) {
        ASYNC_MQTT_LOG("mqtt_impl", info)
            << "[store] for_each";
        for (auto it = elems_.begin(); it != elems_.end();) {
            if (func(it->packet)) {
                ++it;
            }
            else {
                it = elems_.erase(it);
            }
        }
    }

    std::vector<store_packet_t> get_stored() const {
        ASYNC_MQTT_LOG("mqtt_impl", info)
            << "[store] get_stored";
        std::vector<store_packet_t> ret;
        ret.reserve(elems_.size());
        for (auto elem : elems_) {
            if (elem.tim) {
                auto d =
                    std::chrono::duration_cast<std::chrono::seconds>(
                        elem.tim->expiry() - std::chrono::steady_clock::now()
                    ).count();
                if (d < 0) d = 0;
                elem.packet.update_message_expiry_interval(static_cast<std::uint32_t>(d));
            }
            ret.push_back(force_move(elem.packet));
        }
        return ret;
    }

private:
    struct elem_t {
        elem_t(
            store_packet_t packet,
            std::shared_ptr<as::steady_timer> tim = nullptr
        ): packet{force_move(packet)}, tim{force_move(tim)} {}

        packet_id_t packet_id() const {
            return packet.packet_id();
        }

        response_packet response_packet_type() const {
            return packet.response_packet_type();
        }

        void const* tim_address() const {
            return tim.get();
        }

        store_packet_t packet;
        std::shared_ptr<as::steady_timer> tim = nullptr;
    };
    struct tag_seq{};
    struct tag_res_id{};
    struct tag_tim{};
    using mi_elem = mi::multi_index_container<
        elem_t,
        mi::indexed_by<
            mi::sequenced<
                mi::tag<tag_seq>
            >,
            mi::ordered_unique<
                mi::tag<tag_res_id>,
                mi::key<
                    &elem_t::response_packet_type,
                    &elem_t::packet_id
                >
            >,
            mi::hashed_non_unique<
                mi::tag<tag_tim>,
                mi::key<
                    &elem_t::tim_address
                >
            >
        >
    >;

    mi_elem elems_;
    Executor exe_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_STORE_HPP
