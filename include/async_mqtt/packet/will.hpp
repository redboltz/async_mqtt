// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_WILL_HPP)
#define ASYNC_MQTT_PACKET_WILL_HPP

#include <async_mqtt/buffer.hpp>
#include <async_mqtt/packet/pubopts.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/util/json_like_out.hpp>

namespace async_mqtt {

class will {
public:
    /**
     * @brief constructor
     * @param topic
     *        A topic name to publish as a will
     * @param message
     *        The contents to publish as a will
     * @param pubopts
     *        Qos and retain flag
     * @param qos
     *        qos
     */
    template <
        typename StringViewLikeTopic,
        typename StringViewLikeMessage,
        std::enable_if_t<
            std::is_convertible_v<std::decay_t<StringViewLikeTopic>, std::string_view> &&
            std::is_convertible_v<std::decay_t<StringViewLikeMessage>, std::string_view>,
            std::nullptr_t
        > = nullptr
    >
    will(
        StringViewLikeTopic&& topic,
        StringViewLikeMessage&& message,
        pub::opts pubopts = {},
        properties props = {}
    ) :
        topic_{std::string{std::forward<StringViewLikeTopic>(topic)}},
        message_{std::string{std::forward<StringViewLikeMessage>(message)}},
        pubopts_{pubopts},
        props_(force_move(props))
    {}

    /**
     * @brief Get topic
     * @return topic
     */
    std::string topic() const {
        return std::string{topic_};
    }

    /**
     * @brief Get topic as a buffer
     * @return topic
     */
    buffer const& topic_as_buffer() const {
        return topic_;
    }

    /**
     * @brief Get topic as a buffer
     * @return topic
     */
    buffer& topic_as_buffer() {
        return topic_;
    }

    /**
     * @brief Get message
     * @return message
     */
    std::string message() const {
        return std::string{message_};
    }

    /**
     * @brief Get message as a buffer
     * @return message
     */
    buffer const& message_as_buffer() const {
        return message_;
    }

    /**
     * @brief Get message as a buffer
     * @return message
     */
    buffer& message_as_buffer() {
        return message_;
    }

    /**
     * @brief Get retain
     * @return retain
     */
    constexpr pub::retain get_retain() const {
        return pubopts_.get_retain();
    }

    /**
     * @brief Get QoS
     * @return QoS
     */
    constexpr qos get_qos() const {
        return pubopts_.get_qos();
    }

    /**
     * @brief Get properties
     * @return properties
     */
    constexpr properties const& props() const {
        return props_;
    }

    /**
     * @brief Get properties
     * @return properties
     */
    constexpr properties& props() {
        return props_;
    }

    /**
     * @brief equality comparison operator
     */
    friend
    bool operator==(will const& lhs, will const& rhs) {
        return
            std::tie(lhs.topic_, lhs.message_, lhs.pubopts_, lhs.props_) ==
            std::tie(rhs.topic_, rhs.message_, rhs.pubopts_, rhs.props_);
    }

    /**
     * @brief less than comparison operator
     */
    friend
    bool operator<(will const& lhs, will const& rhs) {
        return
            std::tie(lhs.topic_, lhs.message_, lhs.pubopts_, lhs.props_) <
            std::tie(rhs.topic_, rhs.message_, rhs.pubopts_, rhs.props_);
    }

private:
    buffer topic_;
    buffer message_;
    pub::opts pubopts_;
    properties props_;
};

/**
 * @ingroup packet
 * @brief stream output operator
 */
inline std::ostream& operator<<(std::ostream& o, will const& val) {
    o << "{" <<
        "topic:" << val.topic_as_buffer() << "," <<
        "message:" << json_like_out(val.message_as_buffer()) << "," <<
        "qos:" << val.get_qos() << "," <<
        "retain:" << val.get_retain();
    if (!val.props().empty()) {
        o << ",ps:" << val.props();
    }
    o << "}";
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_WILL_HPP
