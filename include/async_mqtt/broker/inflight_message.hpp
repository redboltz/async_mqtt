// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_INFLIGHT_MESSAGE_HPP)
#define ASYNC_MQTT_BROKER_INFLIGHT_MESSAGE_HPP


#include <chrono>

#include <boost/asio/steady_timer.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/key.hpp>

#include <async_mqtt/packet/store_packet_variant.hpp>
#include <async_mqtt/util/any.hpp>
#include <async_mqtt/log.hpp>

#include <async_mqtt/broker/common_type.hpp>
#include <async_mqtt/broker/tags.hpp>

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

    packet_id_t packet_id() const {
        return packet_.packet_id();
    }

    template <typename Epsp>
    void send(Epsp& epsp) const {
        optional<store_packet_variant> packet_opt;
        if (tim_message_expiry_) {
            packet_.visit(
                overload {
                    [&](v5::basic_publish_packet<sizeof(packet_id_t)> const& m) {
                        auto updated_packet = m;
                        auto d =
                            std::chrono::duration_cast<std::chrono::seconds>(
                                tim_message_expiry_->expiry() - std::chrono::steady_clock::now()
                            ).count();
                        if (d < 0) d = 0;
                        updated_packet.update_message_expiry_interval(static_cast<std::uint32_t>(d));
                        packet_opt.emplace(force_move(updated_packet));
                    },
                    [](auto const&) {
                    }
                }
            );
        }
        epsp.register_packet_id(packet_id());
        epsp.send(
            packet_opt ? *packet_opt : packet_,
            [epsp](system_error const& ec) {
                if (ec) {
                    ASYNC_MQTT_LOG("mqtt_broker", trace)
                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                        << ec.what();
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
                BOOST_MULTI_INDEX_CONST_MEM_FUN(inflight_message, packet_id_t, packet_id)
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
