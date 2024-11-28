// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_IMPL_STORE_HPP)
#define ASYNC_MQTT_PROTOCOL_IMPL_STORE_HPP

#include <boost/asio/steady_timer.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <async_mqtt/util/log.hpp>
#include <async_mqtt/protocol/packet/store_packet_variant.hpp>
#include <async_mqtt/protocol/packet/packet_traits.hpp>


namespace async_mqtt {

namespace as = boost::asio;
namespace mi = boost::multi_index;

template <std::size_t PacketIdBytes>
class store {
public:
    using store_packet_type = basic_store_packet_variant<PacketIdBytes>;

    explicit store() = default;

    template <typename Packet>
    bool add(Packet const& packet) {
        if constexpr(is_publish<Packet>()) {
            if (packet.opts().get_qos() == qos::at_least_once ||
                packet.opts().get_qos() == qos::exactly_once) {
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << "[store] add pid:" << packet.packet_id();
                return elems_.emplace_back(packet).second;
            }
        }
        else if constexpr(is_pubrel<Packet>()) {
            ASYNC_MQTT_LOG("mqtt_impl", trace)
                << "[store] add pid:" << packet.packet_id();
            return elems_.emplace_back(packet).second;
        }
        return false;
    }

    bool erase(response_packet r, typename basic_packet_id_type<PacketIdBytes>::type packet_id) {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << "[store] erase pid:" << packet_id;
        auto& idx = elems_.template get<tag_res_id>();
        auto it = idx.find(std::make_tuple(r, packet_id));
        if (it == idx.end()) return false;
        idx.erase(it);
        return true;
    }

    bool erase_publish(typename basic_packet_id_type<PacketIdBytes>::type packet_id) {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << "[store] erase_publish pid:" << packet_id;
        auto& idx = elems_.template get<tag_id>();
        auto [b, e] = idx.equal_range(packet_id);
        for (; b != e; ++b) {
            if (b->packet_id() == packet_id &&
                (
                    b->response_packet() == response_packet::v3_1_1_puback ||
                    b->response_packet() == response_packet::v3_1_1_pubrec ||
                    b->response_packet() == response_packet::v5_puback ||
                    b->response_packet() == response_packet::v5_pubrec
                )
            ) {
                b = idx.erase(b);
                return true;
            }
        }
        return false;
    }

    void clear() {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << "[store] clear";
        elems_.clear();
    }

    template <typename Func>
    void for_each(Func func) {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
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

    std::vector<store_packet_type> get_stored() const {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << "[store] get_stored";
        std::vector<store_packet_type> ret;
        ret.reserve(elems_.size());
        for (auto elem : elems_) {
            ret.push_back(force_move(elem.packet));
        }
        return ret;
    }

private:
    struct elem_t {
        elem_t(
            store_packet_type packet
        ): packet{force_move(packet)} {}

        typename basic_packet_id_type<PacketIdBytes>::type packet_id() const {
            return packet.packet_id();
        }

        response_packet response_packet_type() const {
            return packet.response_packet_type();
        }

        store_packet_type packet;
    };
    struct tag_seq{};
    struct tag_id{};
    struct tag_res_id{};
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
            >
        >
    >;

    mi_elem elems_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_IMPL_STORE_HPP
