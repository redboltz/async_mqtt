// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_STORE_HPP)
#define ASYNC_MQTT_UTIL_STORE_HPP

#include <boost/asio/steady_timer.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <async_mqtt/util/log.hpp>
#include <async_mqtt/packet/store_packet_variant.hpp>
#include <async_mqtt/packet/packet_traits.hpp>


namespace async_mqtt {

namespace as = boost::asio;
namespace mi = boost::multi_index;

template <std::size_t PacketIdBytes>
class store {
public:
    using store_packet_type = basic_store_packet_variant<PacketIdBytes>;

    explicit store(as::any_io_executor exe):exe_{exe}{}

    template <typename Packet>
    bool add(Packet const& packet) {
        if constexpr(is_publish<Packet>()) {
            if (packet.opts().get_qos() == qos::at_least_once ||
                packet.opts().get_qos() == qos::exactly_once) {
                return elems_.emplace_back(packet).second;
            }
        }
        else if constexpr(is_pubrel<Packet>()) {
            return elems_.emplace_back(packet).second;
        }
        return false;
    }

    bool erase(response_packet r, typename basic_packet_id_type<PacketIdBytes>::type packet_id) {
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

    std::vector<store_packet_type> get_stored() const {
        ASYNC_MQTT_LOG("mqtt_impl", info)
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
    as::any_io_executor exe_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_STORE_HPP
