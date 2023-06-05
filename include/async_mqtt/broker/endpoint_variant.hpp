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
struct basic_endpoint_variant : std::variant<basic_endpoint<Role, PacketIdBytes, NextLayer>...> {
    using this_type = basic_endpoint_variant<Role, PacketIdBytes, NextLayer...>;
    using base_type = std::variant<basic_endpoint<Role, PacketIdBytes, NextLayer>...>;
    using base_type::base_type;
    using shared_type = std::shared_ptr<this_type>;
    using weak_type = std::weak_ptr<this_type>;

    static constexpr role role_value = Role;
    static constexpr std::size_t packet_id_bytes = PacketIdBytes;
    using packet_id_t = typename packet_id_type<packet_id_bytes>::type;

    template <typename ActualNextLayer>
    basic_endpoint<Role, PacketIdBytes, ActualNextLayer> const& as() const {
        return std::get<basic_endpoint<Role, PacketIdBytes, ActualNextLayer>>(*this);
    }
    template <typename ActualNextLayer>
    basic_endpoint<Role, PacketIdBytes, ActualNextLayer>& as() {
        return std::get<basic_endpoint<Role, PacketIdBytes, ActualNextLayer>>(*this);
    }

    template <typename Actual, typename... Args>
    static std::shared_ptr<this_type> make_shared(
        Args&&... args
    ) {
        return
            std::make_shared<this_type>(
                Actual(std::forward<Args>(args)...)
            );
    }
};

template <role Role, typename... NextLayer>
using endpoint_variant = basic_endpoint_variant<Role, 2, NextLayer...>;

template <typename Epsp>
class epsp_wrap {
public:
    using epsp_t = Epsp;
    using this_type = epsp_wrap<Epsp>;
    using packet_id_t = typename epsp_t::element_type::packet_id_t;
    static constexpr std::size_t packet_id_bytes = epsp_t::element_type::packet_id_bytes;
    using packet_variant_type = basic_packet_variant<packet_id_bytes>;
    using weak_type = typename epsp_t::weak_type;

    epsp_wrap(epsp_t&& epsp)
        : epsp_{force_move(epsp)}
    {
    }

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

    template <typename Func>
    void dispatch(Func&& func) const {
        visit(
            [&](auto& ep){
                as::dispatch(
                    ep.strand(),
                    std::forward<Func>(func)
                );
            }
        );
    }

    bool running_in_this_thread() const {
        return visit(
            [&](auto& ep) -> bool {
                return ep.strand().running_in_this_thread();
            }
        );
    }

#if 0
    decltype(auto) strand() const {
        return visit(
            [&](auto& ep) -> decltype(auto) {
                return ep.strand();
            }
        );
    }
    decltype(auto) strand() {
        return visit(
            [&](auto& ep) -> decltype(auto) {
                return ep.strand();
            }
        );
    }
#endif
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
    restore_packets(
        std::vector<basic_store_packet_variant<packet_id_bytes>> pvs,
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.restore_packets(
                    force_move(pvs),
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    typename as::async_result<
        std::decay_t<CompletionToken>,
        void(std::vector<basic_store_packet_variant<packet_id_bytes>>)
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

    void restore_packets(
        std::vector<basic_store_packet_variant<packet_id_bytes>> pvs
    ) {
        visit(
            [&](auto& ep) {
                ep.restore(pvs);
            }
        );
    }

    std::vector<basic_store_packet_variant<packet_id_bytes>> get_stored_packets() const {
        return visit(
            [&](auto& ep) {
                return ep.get_stored_packets();
            }
        );
    }

    void set_preauthed_user_name(optional<std::string> user_name) {
        preauthed_user_name_ = force_move(user_name);
    }

    optional<std::string> const& get_preauthed_user_name() const {
        return preauthed_user_name_;
    }

    protocol_version get_protocol_version() const {
        if (!protocol_version_) {
            // The following code requires running in ep's strand.
            // It is safe because it is always called from ep's strand
            // in connect_handler for the first time.
            protocol_version_.emplace(
                visit(
                    [&](auto& ep) {
                        return ep.get_protocol_version();
                    }
                )
            );
        }
        return *protocol_version_;
    }

    bool is_publish_processing(packet_id_t pid) const {
        return visit(
            [&](auto& ep) {
                return ep.is_publish_processing(pid);
            }
        );
    }

    void set_client_id(buffer cid) {
        client_id_ = force_move(cid);
    }

    buffer const& get_client_id() const {
        return client_id_;
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
    buffer client_id_;
    optional<std::string> preauthed_user_name_;
    mutable optional<protocol_version> protocol_version_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_ENDPOINT_VARIANT_HPP
