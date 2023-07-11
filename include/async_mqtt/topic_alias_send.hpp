// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_TOPIC_ALIAS_SEND_HPP)
#define ASYNC_MQTT_TOPIC_ALIAS_SEND_HPP

#include <string>
#include <unordered_map>
#include <array>

#include <boost/assert.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key.hpp>

#include <async_mqtt/constant.hpp>
#include <async_mqtt/log.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/optional.hpp>
#include <async_mqtt/util/value_allocator.hpp>
#include <async_mqtt/util/time_point.hpp>
#include <async_mqtt/util/string_view.hpp>

namespace async_mqtt {

namespace mi = boost::multi_index;

class topic_alias_send {
public:
    topic_alias_send(topic_alias_t max)
        :max_{max}, va_{min_, max_} {}

    void insert_or_update(string_view topic, topic_alias_t alias) {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "topic_alias_send insert"
            << " topic:" << topic
            << " alias:" << alias;
        BOOST_ASSERT(!topic.empty() && alias >= min_ && alias <= max_);
        va_.use(alias);
        auto& idx = aliases_.get<tag_alias>();
        auto it = idx.lower_bound(alias);
        if (it == idx.end() || it->alias != alias) {
            idx.emplace_hint(it, std::string(topic), alias, std::chrono::steady_clock::now());
        }
        else {
            idx.modify(
                it,
                [&](entry& e) {
                    e.topic = std::string{topic};
                    e.tp = std::chrono::steady_clock::now();
                },
                [](auto&) { BOOST_ASSERT(false); }
            );

        }
    }

    std::string find(topic_alias_t alias) {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "find_topic_by_alias"
            << " alias:" << alias;

        BOOST_ASSERT(alias >= min_ && alias <= max_);
        auto& idx = aliases_.get<tag_alias>();
        auto it = idx.find(alias);
        if (it == idx.end()) return std::string();

        idx.modify(
            it,
            [&](entry& e) {
                e.tp = std::chrono::steady_clock::now();
            },
            [](auto&) { BOOST_ASSERT(false); }
        );
        return it->topic;
    }

    std::string find_without_touch(topic_alias_t alias) const {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "find_topic_by_alias"
            << " alias:" << alias;

        BOOST_ASSERT(alias >= min_ && alias <= max_);
        auto& idx = aliases_.get<tag_alias>();
        auto it = idx.find(alias);
        if (it == idx.end()) return std::string();
        return it->topic;
    }

    optional<topic_alias_t> find(string_view topic) const {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "find_alias_by_topic"
            << " topic:" << topic;

        auto& idx = aliases_.get<tag_topic_name>();
        auto it = idx.find(topic);
        if (it == idx.end()) return nullopt;
        return it->alias;
    }

    void clear() {
        ASYNC_MQTT_LOG("mqtt_impl", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "clear_topic_alias";
        aliases_.clear();
        va_.clear();
    }

    topic_alias_t get_lru_alias() const {
        BOOST_ASSERT(max_ > 0);
        if (auto alias_opt = va_.first_vacant()) {
            return *alias_opt;
        }
        auto& idx = aliases_.get<tag_tp>();
        return idx.begin()->alias;
    }

    topic_alias_t max() const { return max_; }

private:
    static constexpr topic_alias_t min_ = 1;
    topic_alias_t max_;

    struct entry {
        entry(std::string topic, topic_alias_t alias, time_point_t tp)
            : topic{force_move(topic)}, alias{alias}, tp{force_move(tp)} {}

        string_view get_topic_as_view() const {
            return topic;
        }

        std::string topic;
        topic_alias_t alias;
        time_point_t tp;
    };
    struct tag_tp {};
    struct tag_alias {};
    struct tag_topic_name {};
    using mi_topic_alias = mi::multi_index_container<
        entry,
        mi::indexed_by<
            mi::ordered_unique<
                mi::tag<tag_alias>,
                mi::key<&entry::alias>
            >,
            mi::ordered_unique<
                mi::tag<tag_topic_name>,
                mi::key<&entry::get_topic_as_view>
            >,
            mi::ordered_non_unique<
                mi::tag<tag_tp>,
                mi::key<&entry::tp>
            >
        >
    >;

    mi_topic_alias aliases_;
    value_allocator<topic_alias_t> va_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_TOPIC_ALIAS_SEND_HPP
