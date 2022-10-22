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
    will(buffer topic,
         buffer message,
         pub::opts pubopts = {},
         properties props = {}
    )
        :topic_{force_move(topic)},
         message_{force_move(message)},
         pubopts_{pubopts},
         props_(force_move(props))
    {}

    constexpr buffer const& topic() const {
        return topic_;
    }
    constexpr buffer& topic() {
        return topic_;
    }
    constexpr buffer const& message() const {
        return message_;
    }
    constexpr buffer& message() {
        return message_;
    }
    constexpr pub::retain get_retain() const {
        return pubopts_.get_retain();
    }
    constexpr qos get_qos() const {
        return pubopts_.get_qos();
    }
    constexpr properties const& props() const {
        return props_;
    }
    constexpr properties& props() {
        return props_;
    }

    friend
    bool operator==(will const& lhs, will const& rhs) {
        return
            std::tie(lhs.topic_, lhs.message_, lhs.pubopts_, lhs.props_) ==
            std::tie(rhs.topic_, rhs.message_, rhs.pubopts_, rhs.props_);
    }

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

inline std::ostream& operator<<(std::ostream& o, will const& val) {
    o << "{" <<
        "topic:" << val.topic() << "," <<
        "message:" << json_like_out(val.message()) << "," <<
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
