// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_REGULATE_FOR_STORE_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_REGULATE_FOR_STORE_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint<Role, PacketIdBytes, NextLayer>::
regulate_for_store_op {
    this_type const& ep;
    v5::basic_publish_packet<PacketIdBytes> packet;
    enum { dispatch, complete } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        switch (state) {
        case dispatch: {
            state = complete;
            auto& a_ep{ep};
            as::dispatch(
                a_ep.get_executor(),
                force_move(self)
            );
        } break;
        case complete: {
            error_code ec;
            ep.regulate_for_store(packet, ec);
            self.complete(ec, force_move(packet));
        } break;
        }
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(v5::basic_publish_packet<PacketIdBytes>)
)
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_regulate_for_store(
    v5::basic_publish_packet<PacketIdBytes> packet,
    CompletionToken&& token
) const {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "regulate_for_store:" << packet;
    return
        as::async_compose<
            CompletionToken,
            void(error_code, v5::basic_publish_packet<PacketIdBytes>)
        >(
            regulate_for_store_op{
                *this,
                force_move(packet)
            },
            token,
            get_executor()
        );
}

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::regulate_for_store(
    v5::basic_publish_packet<PacketIdBytes>& packet,
    error_code& ec
) const {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "regulate_for_store:" << packet;
    if (packet.topic().empty()) {
        if (auto ta_opt = get_topic_alias(packet.props())) {
            auto topic = topic_alias_send_->find_without_touch(*ta_opt);
            if (topic.empty()) {
                ec = make_error_code(
                    mqtt_error::packet_not_regulated
                );
                return;
            }
            packet.remove_topic_alias_add_topic(force_move(topic));
        }
        else {
            ec = make_error_code(
                mqtt_error::packet_not_regulated
            );
            return;
        }
    }
    else {
        packet.remove_topic_alias();
    }
    ec = error_code{};
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_REGULATE_FOR_STORE_HPP
