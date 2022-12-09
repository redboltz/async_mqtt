// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_ENDPOINT_VARIANT_HPP)
#define ASYNC_MQTT_BROKER_ENDPOINT_VARIANT_HPP

#include <variant>
#include <memory>
#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename... NextLayer>
using basic_endpoint_variant =
    std::variant<basic_endpoint<Role, PacketIdBytes, NextLayer>...>;

template <role Role, std::size_t PacketIdBytes, typename... NextLayer>
using basic_endpoint_variant_sp = std::shared_ptr<basic_endpoint_variant<Role, PacketIdBytes, NextLayer...>>;

template <role Role, std::size_t PacketIdBytes, typename... NextLayer>
using basic_endpoint_variant_wp = std::weak_ptr<basic_endpoint_variant<Role, PacketIdBytes, NextLayer...>>;

template <role Role, typename... NextLayer>
using endpoint_variant = basic_endpoint_variant<Role, 2, NextLayer...>;

template <role Role, typename... NextLayer>
using endpoint_variant_sp = std::shared_ptr<endpoint_variant<Role, NextLayer...>>;

template <role Role, typename... NextLayer>
using endpoint_variant_wp = std::weak_ptr<endpoint_variant<Role, NextLayer...>>;


template <role Role, std::size_t PacketIdBytes, typename... NextLayer>
class epsp_wrap {
public:
    using this_type = epsp_wrap<Role, PacketIdBytes, NextLayer...>;
    using epsp_t = basic_endpoint_variant_sp<Role, PacketIdBytes, NextLayer...>;
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;
    using packet_variant_type = basic_packet_variant<PacketIdBytes>;
    using weak_type = typename epsp_t::weak_type;

    epsp_wrap(epsp_t&& epsp)
        : epsp_{force_move(epsp)}
    {}

    template <typename Func>
    decltype(auto) visit(Func&& func) const {
        return std::visit(
            [&](auto& ep) -> decltype(auto) {
                return std::forward<Func>(func)(ep);
            },
            *epsp_
        );
    }

    template <typename Func>
    decltype(auto) visit(Func&& func) {
        return std::visit(
            [&](auto& ep) -> decltype(auto) {
                return std::forward<Func>(func)(ep);
            },
            *epsp_
        );
    }

    decltype(auto) strand() const {
        return visit(
            [&](auto& ep) -> decltype(auto) {
                return ep.stream().strand();
            }
        );
    }
    decltype(auto) strand() {
        return visit(
            [&](auto& ep) -> decltype(auto) {
                return ep.stream().strand();
            }
        );
    }

    // async functions

    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(optional<packet_id_t>)>::return_type
    acquire_unique_packet_id(
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.acquire_unique_packet_id(
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(bool)>::return_type
    register_packet_id(
        packet_id_t packet_id,
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.register_packet_id(
                    packet_id,
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void()>::return_type
    release_packet_id(
        packet_id_t packet_id,
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.release_packet_id(
                    packet_id,
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename Packet, typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(system_error)>::return_type
    send(
        Packet&& packet,
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.send(
                    std::forward<Packet>(packet),
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(packet_variant_type)>::return_type
    recv(
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.recv(
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void()>::return_type
    close(
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.close(
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void()>::return_type
    restore(
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs,
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.restore(
                    force_move(pvs),
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    typename as::async_result<
        std::decay_t<CompletionToken>,
        void(std::vector<basic_store_packet_variant<PacketIdBytes>>)
    >::return_type
    get_stored(
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.get_stored(
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    // sync APIs that reqire woking on strand

    optional<packet_id_t> acquire_unique_packet_id() {
        return visit(
            [&](auto& ep) {
                return ep.acquire_unique_packet_id();
            }
        );
    }

    bool register_packet_id(packet_id_t pid) {
        return visit(
            [&](auto& ep) {
                return ep.register_packet_id(pid);
            }
        );
    }

    void release_packet_id(packet_id_t pid) {
        visit(
            [&](auto& ep) {
                return ep.release_packet_id(pid);
            }
        );
    }

    /**
     * @brief Get processed but not released QoS2 packet ids
     *        This function should be called after disconnection
     * @return set of packet_ids
     */
    std::set<packet_id_t> get_qos2_publish_handled_pids() const {
        return visit(
            [&](auto& ep) {
                return ep.get_qos2_publish_handled_pids();
            }
        );
    }

    /**
     * @brief Restore processed but not released QoS2 packet ids
     *        This function should be called before receive the first publish
     * @param pids packet ids
     */
    void restore_qos2_publish_handled_pids(std::set<packet_id_t> pids) {
        visit(
            [&](auto& ep) {
                return ep.restore_qos2_publish_handled_pids(pids);
            }
        );
    }

    void restore(
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs
    ) {
        return visit(
            [&](auto& ep) {
                ep.restore(pvs);
            }
        );
    }

    std::vector<basic_store_packet_variant<PacketIdBytes>> get_stored() const {
        return visit(
            [&](auto& ep) {
                return ep.get_stored();
            }
        );
    }

    void set_preauthed_user_name(optional<std::string> user_name) {
        visit(
            [&](auto& ep) {
                ep.set_preauthed_user_name(force_move(user_name));
            }
        );
    }

    optional<std::string> get_preauthed_user_name() const {
        return visit(
            [&](auto& ep) {
                return ep.get_preauthed_user_name();
            }
        );
    }

    protocol_version get_protocol_version() const {
        return visit(
            [&](auto& ep) {
                return ep.get_protocol_version();
            }
        );
    }

    void set_client_id(buffer cid) {
        visit(
            [&](auto& ep) {
                ep.set_client_id(force_move(cid));
            }
        );
    }

    buffer const& get_client_id() const {
        return visit(
            [&](auto& ep) -> buffer const& {
                return ep.get_client_id();
            }
        );
    }

    operator bool() const {
        return static_cast<bool>(epsp_);
    }

    operator weak_type() const {
        return epsp_;
    }

    void const* get_address() const {
        return epsp_.get();
    }

    bool owner_before(this_type const& other) const noexcept {
        return epsp_.owner_before(other.epsp_);
    }

    bool owner_before(typename epsp_t::weak_type const& other) const noexcept {
        return epsp_.owner_before(other);
    }

private:
    epsp_t epsp_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_ENDPOINT_VARIANT_HPP
