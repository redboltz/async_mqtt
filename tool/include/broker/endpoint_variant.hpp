// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_ENDPOINT_VARIANT_HPP)
#define ASYNC_MQTT_BROKER_ENDPOINT_VARIANT_HPP

#include <variant>
#include <memory>
#include <async_mqtt/asio_bind/endpoint.hpp>
#include <async_mqtt/protocol/packet/packet_id_type.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename... NextLayer>
struct basic_endpoint_variant :
        std::variant<std::shared_ptr<basic_endpoint<Role, PacketIdBytes, NextLayer>>...> {
    using this_type = basic_endpoint_variant<Role, PacketIdBytes, NextLayer...>;
    using base_type = std::variant<std::shared_ptr<basic_endpoint<Role, PacketIdBytes, NextLayer>>...>;
    using base_type::base_type;

    static constexpr role role_value = Role;
    static constexpr std::size_t packet_id_bytes = PacketIdBytes;

    template <typename ActualNextLayer>
    basic_endpoint<Role, PacketIdBytes, ActualNextLayer> const& as() const {
        return *std::get<std::shared_ptr<basic_endpoint<Role, PacketIdBytes, ActualNextLayer>>>(*this);
    }
    template <typename ActualNextLayer>
    basic_endpoint<Role, PacketIdBytes, ActualNextLayer>& as() {
        return *std::get<std::shared_ptr<basic_endpoint<Role, PacketIdBytes, ActualNextLayer>>>(*this);
    }

    struct weak_type : std::variant<std::weak_ptr<basic_endpoint<Role, PacketIdBytes, NextLayer>>...> {
        using base_type = std::variant<std::weak_ptr<basic_endpoint<Role, PacketIdBytes, NextLayer>>...>;
        using base_type::base_type;
        using shared_type = std::variant<std::shared_ptr<basic_endpoint<Role, PacketIdBytes, NextLayer>>...>;
        this_type lock() {
            return std::visit(
                [&](auto& wp) -> this_type {
                    return wp.lock();
                },
                *this
            );
        }
        bool operator<(weak_type const& other) const {
            return std::visit(
                [&](auto& lhs) {
                    return std::visit(
                        [&](auto& rhs) {
                            return lhs.owner_before(rhs);
                        },
                        other
                    );
                },
                *this
            );
        }
    };
};

template <role Role, typename... NextLayer>
using endpoint_variant = basic_endpoint_variant<Role, 2, NextLayer...>;

template <typename Epsp>
class epsp_wrap {
public:
    using epsp_type = Epsp;
    using this_type = epsp_wrap<Epsp>;
    static constexpr std::size_t packet_id_bytes = epsp_type::packet_id_bytes;
    using packet_variant_type = basic_packet_variant<packet_id_bytes>;
    using weak_type = typename epsp_type::weak_type;

    epsp_wrap(epsp_type epsp)
        : epsp_{force_move(epsp)}
    {
    }

    template <typename Func>
    decltype(auto) visit(Func&& func) const {
        return std::visit(
            [&](auto& ep) -> decltype(auto) {
                return std::forward<Func>(func)(*ep);
            },
            epsp_
        );
    }

    template <typename Func>
    decltype(auto) visit(Func&& func) {
        return std::visit(
            [&](auto& ep) -> decltype(auto) {
                return std::forward<Func>(func)(*ep);
            },
            epsp_
        );
    }

    template <typename Func>
    void dispatch(Func&& func) const {
        visit(
            [&](auto& ep){
                as::dispatch(
                    as::bind_executor(
                        ep.get_executor(),
                        std::forward<Func>(func)
                    )
                );
            }
        );
    }

    as::any_io_executor get_executor() {
        return visit(
            [&](auto& ep) -> as::any_io_executor {
                return ep.get_executor();
            }
        );
    }

    // async functions

    template <typename CompletionToken>
    auto
    async_acquire_unique_packet_id(
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.async_acquire_unique_packet_id(
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    auto
    async_acquire_unique_packet_id_wait_until(
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.async_acquire_unique_packet_id_wait_until(
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    auto
    async_register_packet_id(
        typename basic_packet_id_type<packet_id_bytes>::type packet_id,
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.async_register_packet_id(
                    packet_id,
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    auto
    async_release_packet_id(
        typename basic_packet_id_type<packet_id_bytes>::type packet_id,
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.async_release_packet_id(
                    packet_id,
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename Packet, typename CompletionToken>
    auto
    async_send(
        Packet&& packet,
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.async_send(
                    std::forward<Packet>(packet),
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    auto
    async_recv(
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.async_recv(
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    auto
    async_close(
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.async_close(
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    auto
    async_restore_packets(
        std::vector<basic_store_packet_variant<packet_id_bytes>> pvs,
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.async_restore_packets(
                    force_move(pvs),
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    template <typename CompletionToken>
    auto
    async_get_stored_packets(
        CompletionToken&& token
    ) {
        return visit(
            [&](auto& ep) {
                return ep.async_get_stored_packets(
                    std::forward<CompletionToken>(token)
                );
            }
        );
    }

    // sync APIs (Thread unsafe without strand)

    std::optional<typename basic_packet_id_type<packet_id_bytes>::type> acquire_unique_packet_id() {
        return visit(
            [&](auto& ep) {
                return ep.acquire_unique_packet_id();
            }
        );
    }

    bool register_packet_id(typename basic_packet_id_type<packet_id_bytes>::type pid) {
        return visit(
            [&](auto& ep) {
                return ep.register_packet_id(pid);
            }
        );
    }

    void release_packet_id(typename basic_packet_id_type<packet_id_bytes>::type pid) {
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
    std::set<typename basic_packet_id_type<packet_id_bytes>::type> get_qos2_publish_handled_pids() const {
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
    void restore_qos2_publish_handled_pids(std::set<typename basic_packet_id_type<packet_id_bytes>::type> pids) {
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

    void set_preauthed_user_name(std::optional<std::string> user_name) {
        preauthed_user_name_ = force_move(user_name);
    }

    std::optional<std::string> const& get_preauthed_user_name() const {
        return preauthed_user_name_;
    }

    protocol_version get_protocol_version() const {
        if (!protocol_version_) {
            // On multi threaded environment,
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

    bool is_publish_processing(typename basic_packet_id_type<packet_id_bytes>::type pid) const {
        return visit(
            [&](auto& ep) {
                return ep.is_publish_processing(pid);
            }
        );
    }

    void set_client_id(std::string cid) {
        client_id_ = force_move(cid);
    }

    std::string const& get_client_id() const {
        return client_id_;
    }

    operator bool() const {
        return std::visit(
            [&](auto const& epsp) {
                return static_cast<bool>(epsp);
            },
            epsp_
        );
    }

    operator weak_type() const {
        return std::visit(
            [&](auto& ep) -> weak_type {
                return ep;
            },
            epsp_
        );
    }

    void const* get_address() const {
        return std::visit(
            [&](auto const& epsp) -> void const*{
                return epsp.get();
            },
            epsp_
        );
    }

    epsp_type get_epvsp() {
        return epsp_;
    }

private:
    epsp_type epsp_;
    std::string client_id_;
    std::optional<std::string> preauthed_user_name_;
    mutable std::optional<protocol_version> protocol_version_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_ENDPOINT_VARIANT_HPP
