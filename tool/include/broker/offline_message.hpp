// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_OFFLINE_MESSAGE_HPP)
#define ASYNC_MQTT_BROKER_OFFLINE_MESSAGE_HPP

#include <optional>

#include <boost/asio/steady_timer.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/key.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/log.hpp>
#include <async_mqtt/protocol/protocol_version.hpp>
#include <async_mqtt/protocol/packet/property_variant.hpp>
#include <async_mqtt/protocol/packet/v3_1_1_publish.hpp>
#include <async_mqtt/protocol/packet/v3_1_1_pubrel.hpp>
#include <async_mqtt/protocol/packet/v5_publish.hpp>
#include <async_mqtt/protocol/packet/v5_pubrel.hpp>
#include <async_mqtt/protocol/packet/pubopts.hpp>

#include <broker/tags.hpp>

namespace async_mqtt {

namespace mi = boost::multi_index;

class offline_messages;

// The offline_message structure holds messages that have been published on a
// topic that a not-currently-connected client is subscribed to.
// When a new connection is made with the client id for this saved data,
// these messages will be published to that client, and only that client.
class offline_message {
public:
    offline_message(
        std::string topic,
        std::vector<buffer> payload,
        pub::opts pubopts,
        properties props,
        std::shared_ptr<as::steady_timer> tim_message_expiry)
        : topic_{force_move(topic)},
          payload_(force_move(payload)),
          pubopts_{pubopts},
          props_(force_move(props)),
          tim_message_expiry_{force_move(tim_message_expiry)}
    {
    }

    template <typename Epsp>
    bool send(Epsp epsp, protocol_version ver) {
        auto publish =
            [&] (packet_id_type pid) {
                switch (ver) {
                case protocol_version::v3_1_1:
                    epsp.async_send(
                        v3_1_1::publish_packet{
                            pid,
                            topic_,
                            payload_,
                            pubopts_
                        },
                        [epsp](error_code const& ec) {
                            if (ec) {
                                ASYNC_MQTT_LOG("mqtt_broker", warning)
                                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                    << ec.message();
                            }
                        }
                    );
                    break;
                case protocol_version::v5: {
                    auto packet =
                        v5::publish_packet{
                            pid,
                            topic_,
                            payload_,
                            pubopts_,
                            props_
                        };
                    if (tim_message_expiry_) {
                        auto d =
                            std::chrono::duration_cast<std::chrono::seconds>(
                                tim_message_expiry_->expiry() - std::chrono::steady_clock::now()
                            ).count();
                        if (d < 0) d = 0;
                        packet.update_message_expiry_interval(static_cast<uint32_t>(d));
                    }
                    epsp.async_send(
                        force_move(packet),
                        [epsp](error_code const& ec) {
                            if (ec) {
                                ASYNC_MQTT_LOG("mqtt_broker", warning)
                                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                    << ec.message();
                            }
                        }
                    );
                } break;
                default:
                    BOOST_ASSERT(false);
                    break;
                }
            };

        auto qos_value = pubopts_.get_qos();
        if (qos_value == qos::at_least_once ||
            qos_value == qos::exactly_once) {
            if (auto pid_opt = epsp.acquire_unique_packet_id()) {
                publish(*pid_opt);
                return true;
            }
            else {
                return false;
            }
        }
        else {
            publish(0);
            return true;
        }
    }

private:
    friend class offline_messages;

    std::string topic_;
    std::vector<buffer> payload_;
    pub::opts pubopts_;
    properties props_;
    std::shared_ptr<as::steady_timer> tim_message_expiry_;
};

class offline_messages {
public:
    template <typename Epsp>
    void send_until_fail(Epsp& epsp, protocol_version ver) {
        epsp.dispatch(
            [this, epsp, ver] {
                auto& idx = messages_.get<tag_seq>();
                while (!idx.empty()) {
                    auto it = idx.begin();

                    // const_cast is appropriate here
                    // See https://github.com/boostorg/multi_index/issues/50
                    auto& m = const_cast<offline_message&>(*it);
                    if (m.send(epsp, ver)) {
                        idx.pop_front();
                    }
                    else {
                        break;
                    }
                }
            }
        );
    }

    void clear() {
        messages_.clear();
    }

    bool empty() const {
        return messages_.empty();
    }

    void push_back(
        as::any_io_executor exe,
        std::string pub_topic,
        std::vector<buffer> payload,
        pub::opts pubopts,
        properties props) {
        std::optional<std::chrono::steady_clock::duration> message_expiry_interval;

        for (auto const& prop : props) {
            prop.visit(
                overload {
                    [&](property::message_expiry_interval const& p) {
                        message_expiry_interval.emplace(std::chrono::seconds(p.val()));
                    },
                    [](auto const&){}
                }
            );
        }

        std::shared_ptr<as::steady_timer> tim_message_expiry;
        if (message_expiry_interval) {
            tim_message_expiry = std::make_shared<as::steady_timer>(exe, *message_expiry_interval);
            tim_message_expiry->async_wait(
                [this, wp = std::weak_ptr<as::steady_timer>(tim_message_expiry)](error_code ec) mutable {
                    if (auto sp = wp.lock()) {
                        if (!ec) {
                            messages_.get<tag_tim>().erase(sp);
                        }
                    }
                }
            );
        }

        auto& seq_idx = messages_.get<tag_seq>();
        seq_idx.emplace_back(
            force_move(pub_topic),
            force_move(payload),
            pubopts,
            force_move(props),
            force_move(tim_message_expiry)
        );
    }

private:
    using mi_offline_message = mi::multi_index_container<
        offline_message,
        mi::indexed_by<
            mi::sequenced<
                mi::tag<tag_seq>
            >,
            mi::ordered_non_unique<
                mi::tag<tag_tim>,
                mi::key<&offline_message::tim_message_expiry_>
            >
        >
    >;

    mi_offline_message messages_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_OFFLINE_MESSAGE_HPP
