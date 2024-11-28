// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_TOPIC_SHARENAME_HPP)
#define ASYNC_MQTT_PACKET_TOPIC_SHARENAME_HPP

#include <async_mqtt/util/buffer.hpp>

namespace async_mqtt {

/**
 * @ingroup packet
 * @brief topic and sharename
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/topic_sharename.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class topic_sharename {
public:

    /**
     * @brief constructor
     * @param all_topic TopicFilter. It could contain sharename on MQTT v5.0.
     */
    template <
        typename AllTopic,
        typename std::enable_if_t<
            std::is_convertible_v<std::decay_t<AllTopic>, std::string_view>,
            std::nullptr_t
        > = nullptr
    >
    topic_sharename(
        AllTopic&& all_topic
    ): all_topic_{std::forward<AllTopic>(all_topic)} {
        auto const shared_prefix = std::string_view("$share/");
        if (all_topic_.substr(0, shared_prefix.size()) == shared_prefix) {
            // must have sharename
            sharename_ = all_topic_.substr(shared_prefix.size());

            auto const idx = sharename_.find_first_of('/');
            if (idx == 0 || idx == std::string_view::npos) return;

            topic_ = sharename_.substr(idx + 1);
            sharename_ = sharename_.substr(0, sharename_.size() - topic_.size() - 1);
        }
        else {
            topic_ = all_topic_;
        }
    }

    /**
     * @brief Get topic
     * @return topic
     */
    std::string const& topic() const {
        return topic_;
    }

    /**
     * @brief Get sharename
     * @return sharename. If no sharename then return empty size std::string.
     */
    std::string const& sharename() const {
        return sharename_;
    }

    /**
     * @brief Get all_topic
     *
     * If sharename is contained, $share/ prefix is contained.
     * @return all_topic that is given to the constructor.
     */
    std::string const& all_topic() const {
        return all_topic_;
    }

    /**
     * @brief bool conversion
     *
     * @return if topic is empty (invalid) then return false, otherwise true.
     */
    operator bool() const {
        return !topic_.empty();
    }

    /**
     * @brief less than operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
     */
    friend
    bool operator<(topic_sharename const& lhs, topic_sharename const& rhs) {
        return
            std::tie(lhs.topic_, lhs.sharename_) <
            std::tie(rhs.topic_, rhs.sharename_);
    }

    /**
     * @brief equal operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
    friend
    bool operator==(topic_sharename const& lhs, topic_sharename const& rhs) {
        return
            std::tie(lhs.topic_, lhs.sharename_) ==
            std::tie(rhs.topic_, rhs.sharename_);
    }

private:
    std::string all_topic_;
    std::string topic_;
    std::string sharename_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_TOPIC_SHARENAME_HPP
