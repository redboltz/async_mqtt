// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_ID_MANAGER_HPP)
#define ASYNC_MQTT_PACKET_ID_MANAGER_HPP

#include <async_mqtt/util/value_allocator.hpp>

namespace async_mqtt {

template <typename PacketId>
class packet_id_manager {
    using packet_id_type = PacketId;

public:

    /**
     * @brief Acquire the new unique packet id.
     *        If all packet ids are already in use, then returns nullopt
     *        After acquiring the packet id, you can call acquired_* functions.
     *        The ownership of packet id is moved to the library.
     *        Or you can call release_packet_id to release it.
     * @return packet id
     */
    optional<packet_id_type> acquire_unique_id() {
        return va_.allocate();
    }

    /**
     * @brief Register packet_id to the library.
     *        After registering the packet_id, you can call acquired_* functions.
     *        The ownership of packet id is moved to the library.
     *        Or you can call release_packet_id to release it.
     * @return If packet_id is successfully registerd then return true, otherwise return false.
     */
    bool register_id(packet_id_type packet_id) {
        return va_.use(packet_id);
    }

    /**
     * @brief Check packet_id is used.
     * @return If packet_id is used then return true, otherwise return false.
     */
    bool is_used_id(packet_id_type packet_id) const {
        return va_.is_used(packet_id);
    }

    /**
     * @brief Release packet_id.
     * @param packet_id packet id to release.
     *                   only the packet_id gotten by acquire_unique_packet_id, or
     *                   register_packet_id is permitted.
     */
    void release_id(packet_id_type packet_id) {
        va_.deallocate(packet_id);
    }

    /**
     * @brief Clear all packet ids.
     */
    void clear() {
        va_.clear();
    }

private:
    value_allocator<packet_id_type> va_ {1, std::numeric_limits<packet_id_type>::max()};
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_ID_MANAGER_HPP
