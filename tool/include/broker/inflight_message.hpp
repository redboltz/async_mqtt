// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_INFLIGHT_MESSAGE_HPP)
#define ASYNC_MQTT_BROKER_INFLIGHT_MESSAGE_HPP


#include <chrono>
#include <optional>
#include <variant>

#include <boost/asio/steady_timer.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/key.hpp>

#include <async_mqtt/util/overload.hpp>
#include <async_mqtt/packet/store_packet_variant.hpp>
#include <async_mqtt/util/log.hpp>

#include <broker/tags.hpp>

namespace async_mqtt {

namespace mi = boost::multi_index;

class inflight_messages;

class inflight_message {
public:
    inflight_message(
        store_packet_variant packet,
        std::shared_ptr<as::steady_timer> tim_message_expiry)
        :packet_ { force_move(packet) },
         tim_message_expiry_ { force_move(tim_message_expiry) }
    {}

    packet_id_type packet_id() const {
        return packet_.packet_id();
    }

    template <typename Epsp>
    void send(Epsp& epsp) const {
        std::optional<store_packet_variant> packet_opt;
        epsp.register_packet_id(packet_id());
        epsp.async_send(
            packet_opt ? *packet_opt : packet_,
            [epsp](error_code const& ec) {
                if (ec) {
                    ASYNC_MQTT_LOG("mqtt_broker", trace)
                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                        << ec.message();
                }
            }
        );
    }

    store_packet_variant const& packet() const {
        return packet_;
    }

private:
    friend class inflight_messages;

    store_packet_variant packet_;
    std::shared_ptr<as::steady_timer> tim_message_expiry_;
};

class inflight_messages {
public:
    void insert(
        store_packet_variant packet,
        std::shared_ptr<as::steady_timer> tim_message_expiry
    ) {
        messages_.emplace_back(
            force_move(packet),
            force_move(tim_message_expiry)
        );
    }

    template <typename Epsp>
    void send_all_messages(Epsp& epsp) {
        for (auto const& ifm : messages_) {
            ifm.send(epsp);
        }
    }

    void clear() {
        messages_.clear();
    }

    template <typename Tag>
    decltype(auto) get() {
        return messages_.get<Tag>();
    }

    template <typename Tag>
    decltype(auto) get() const {
        return messages_.get<Tag>();
    }

private:
    using mi_inflight_message = mi::multi_index_container<
        inflight_message,
        mi::indexed_by<
            mi::sequenced<
                mi::tag<tag_seq>
            >,
            mi::ordered_unique<
                mi::tag<tag_pid>,
                BOOST_MULTI_INDEX_CONST_MEM_FUN(inflight_message, packet_id_type, packet_id)
            >,
            mi::ordered_non_unique<
                mi::tag<tag_tim>,
                BOOST_MULTI_INDEX_MEMBER(inflight_message, std::shared_ptr<as::steady_timer>, tim_message_expiry_)
            >
        >
    >;

    mi_inflight_message messages_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_INFLIGHT_MESSAGE_HPP
