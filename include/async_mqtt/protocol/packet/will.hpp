// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_WILL_HPP)
#define ASYNC_MQTT_PACKET_WILL_HPP

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/protocol/packet/pubopts.hpp>
#include <async_mqtt/protocol/packet/property_variant.hpp>
#include <async_mqtt/util/json_like_out.hpp>

namespace async_mqtt {

/**
 * @ingroup connect_v3_1_1
 * @ingroup connect_v5
 * @brief MQTT will message
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/will.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
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
     * @param props
     *        will properties
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
    explicit will(
        StringViewLikeTopic&& topic,
        StringViewLikeMessage&& message,
        pub::opts pubopts = pub::opts{},
        properties props = properties{}
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
     * @brief equal operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
    friend
    bool operator==(will const& lhs, will const& rhs) {
        return
            std::tie(lhs.topic_, lhs.message_, lhs.pubopts_, lhs.props_) ==
            std::tie(rhs.topic_, rhs.message_, rhs.pubopts_, rhs.props_);
    }

    /**
     * @brief less than operator
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
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

/*
 * @ingroup connect_v3_1_1
 * @ingroup connect_v5
 * @brief output to the stream
 * @param os output stream
 * @param v  target
 * @return output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/will.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
inline std::ostream& operator<<(std::ostream& o, will const& v) {
    o << "{" <<
        "topic:" << v.topic_as_buffer() << "," <<
        "message:" << json_like_out(v.message_as_buffer()) << "," <<
        "qos:" << v.get_qos() << "," <<
        "retain:" << v.get_retain();
    if (!v.props().empty()) {
        o << ",ps:" << v.props();
    }
    o << "}";
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_WILL_HPP
