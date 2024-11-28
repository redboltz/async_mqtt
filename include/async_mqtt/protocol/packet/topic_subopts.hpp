// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP)
#define ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP

#include <async_mqtt/packet/topic_sharename.hpp>
#include <async_mqtt/packet/subopts.hpp>

namespace async_mqtt {

/**
 * @ingroup packet
 * @brief subsscription entry
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/topic_subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
class topic_subopts {
public:

    /**
     * @brief constructor
     * @param all_topic TopicFilter. It could contain sharename on MQTT v5.0.
     * @param opts      subscribe options
     */
    topic_subopts(
        std::string all_topic,
        sub::opts opts
    ): topic_sharename_{force_move(all_topic)},
       opts_{opts}
    {
    }

    /**
     * @brief Get topic
     * @return topic
     */
    std::string const& topic() const {
        return topic_sharename_.topic();
    }

    /**
     * @brief Get sharename
     * @return sharename. If no sharename then return empty size std::string.
     */
    std::string const& sharename() const {
        return topic_sharename_.sharename();
    }

    /**
     * @brief Get all_topic
     *
     * If sharename is contained, $share/ prefix is contained.
     * @return all_topic that is given to the constructor.
     */
    std::string const& all_topic() const {
        return topic_sharename_.all_topic();
    }

    /**
     * @brief Get subscribe options
     * @return subscribe options
     */
    sub::opts const& opts() const { // return reference in mandatory
        return opts_;
    }

    /**
     * @brief bool conversion
     *
     * @return if topic is empty (invalid) then return false, otherwise true.
     */
    operator bool() const {
        return static_cast<bool>(topic_sharename_);
    }

    /**
     * @brief less than operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
     */
    friend
    bool operator<(topic_subopts const& lhs, topic_subopts const& rhs) {
        return
            std::tie(lhs.topic_sharename_, lhs.opts_) <
            std::tie(rhs.topic_sharename_, rhs.opts_);
    }

    /**
     * @brief equal operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
    friend
    bool operator==(topic_subopts const& lhs, topic_subopts const& rhs) {
        return
            std::tie(lhs.topic_sharename_, lhs.opts_) ==
            std::tie(rhs.topic_sharename_, rhs.opts_);
    }

private:
    topic_sharename topic_sharename_;
    sub::opts opts_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP
