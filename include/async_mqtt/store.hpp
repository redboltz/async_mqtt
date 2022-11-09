// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_STORE_HPP)
#define ASYNC_MQTT_STORE_HPP

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <async_mqtt/packet/store_packet_variant.hpp>
#include <async_mqtt/packet/packet_traits.hpp>

namespace async_mqtt {

namespace mi = boost::multi_index;

template <std::size_t PacketIdBytes>
class store {
public:
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;
    using store_packet_t = basic_store_packet_variant<PacketIdBytes>;

    template <typename Packet>
    bool add(Packet const& packet) {
        if constexpr(is_publish<Packet>()) {
            if (packet.opts().qos() == qos::at_least_once ||
                packet.opts().qos() == qos::exactly_once) {
                return elems_.push_back(packet).second;
            }
        }
        else if constexpr(is_pubrel<Packet>()) {
            return elems_.push_back(packet).second;
        }
        return false;
    }

    bool erase(response_packet r, packet_id_t packet_id) {
        auto& idx = elems_.template get<tag_res_id>();
        auto it = idx.find(std::make_tuple(r, packet_id));
        if (it == idx.end()) return false;
        idx.erase(it);
        return true;
    }

    void clear() {
        elems_.clear();
    }

    template <typename Func>
    void for_each(Func const& func) {
        for (auto const& e : elems_) {
            func(e);
        }
    }

private:
    struct tag_seq{};
    struct tag_res_id{};
    using mi_elem = mi::multi_index_container<
        store_packet_t,
        mi::indexed_by<
            mi::sequenced<
                mi::tag<tag_seq>
            >,
            mi::ordered_unique<
                mi::tag<tag_res_id>,
                mi::key<
                    &store_packet_t::response_packet_type,
                    &store_packet_t::packet_id
                >
            >
        >
    >;

    mi_elem elems_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_STORE_HPP
