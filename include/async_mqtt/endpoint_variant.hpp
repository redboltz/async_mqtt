// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ENDPOINT_VARIANT_HPP)
#define ASYNC_MQTT_ENDPOINT_VARIANT_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename... NextLayer>
class basic_endpoint_wp_variant;

template <role Role, std::size_t PacketIdBytes, typename... NextLayer>
class basic_endpoint_sp_variant {
    using ep_sp_t =
        std::variant<
            std::shared_ptr<
                basic_endpoint<Role, PacketIdBytes, NextLayer>
            >...
        >;

public:
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;
    using packet_variant_type = basic_packet_variant<PacketIdBytes>;

    template <typename Endpoint>
    basic_endpoint_sp_variant(std::shared_ptr<Endpoint> ep)
        : ep_{force_move(ep)}
    {}

    decltype(auto) stream() const {
        return std::visit(
            [&](auto& ep) -> decltype(auto) {
                return ep->stream();
            },
            ep_
        );
    }

    decltype(auto) stream() {
        return std::visit(
            [&](auto& ep) -> decltype(auto) {
                return ep->stream();
            },
            ep_
        );
    }

    decltype(auto) strand() const {
        return std::visit(
            [&](auto& ep) -> decltype(auto) {
                return ep->stream().strand();
            },
            ep_
        );
    }
    decltype(auto) strand() {
        return std::visit(
            [&](auto& ep) -> decltype(auto) {
                return ep->stream().strand();
            },
            ep_
        );
    }

    // async functions

    template <
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken, optional<packet_id_t>>::value
        >* = nullptr
    >
    auto acquire_unique_packet_id(
        CompletionToken&& token
    ) {
        return std::visit(
            [&](auto& ep) {
                return ep->acquire_unique_packet_id(
                    std::forward<CompletionToken>(token)
                );
            },
            ep_
        );
    }

    template <
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken, bool>::value
        >* = nullptr
    >
    auto register_packet_id(
        packet_id_t packet_id,
        CompletionToken&& token
    ) {
        return std::visit(
            [&](auto& ep) {
                return ep->register_packet_id(
                    packet_id,
                    std::forward<CompletionToken>(token)
                );
            },
            ep_
        );
    }

    template <
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken>::value
        >* = nullptr
    >
    auto release_packet_id(
        packet_id_t packet_id,
        CompletionToken&& token
    ) {
        return std::visit(
            [&](auto& ep) {
                return ep->release_packet_id(
                    packet_id,
                    std::forward<CompletionToken>(token)
                );
            },
            ep_
        );
    }

    template <
        typename Packet,
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken, system_error>::value
        >* = nullptr
    >
    auto send(
        Packet&& packet,
        CompletionToken&& token
    ) {
        return std::visit(
            [&](auto& ep) {
                return ep->send(
                    std::forward<Packet>(packet),
                    std::forward<CompletionToken>(token)
                );
            },
            ep_
        );
    }

    template <
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken, packet_variant_type>::value
        >* = nullptr
    >
    auto recv(
        CompletionToken&& token
    ) {
        return std::visit(
            [&](auto& ep) {
                return ep->recv(
                    std::forward<CompletionToken>(token)
                );
            },
            ep_
        );
    }

    template <
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken>::value
        >* = nullptr
    >
    auto restore(
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs,
        CompletionToken&& token
    ) {
        return std::visit(
            [&](auto& ep) {
                return ep->restore(
                    force_move(pvs),
                    std::forward<CompletionToken>(token)
                );
            },
            ep_
        );
    }

    operator bool() const {
        return std::visit(
            [&](auto& ep) {
                return static_cast<bool>(ep);
            },
            ep_
        );
    }

private:
    friend
    class basic_endpoint_wp_variant<Role, PacketIdBytes, NextLayer...>;

    ep_sp_t ep_;
};

template <role Role, typename... NextLayer>
using endpoint_sp_variant = basic_endpoint_sp_variant<Role, 2, NextLayer...>;


template <role Role, std::size_t PacketIdBytes, typename... NextLayer>
class basic_endpoint_wp_variant {
    using this_type = basic_endpoint_wp_variant<Role, PacketIdBytes, NextLayer...>;
    using ep_wp_t =
        std::variant<
            std::weak_ptr<
                basic_endpoint<Role, PacketIdBytes, NextLayer>
            >...
        >;
    using sp_t = basic_endpoint_sp_variant<Role, PacketIdBytes, NextLayer...>;

public:
    template <typename Endpoint>
    basic_endpoint_wp_variant(std::shared_ptr<Endpoint> sp)
        : ep_{sp}
    {}

    basic_endpoint_wp_variant(sp_t& sp) {
        std::visit(
            [&](auto& ep) {
                ep_ = ep;
            },
            sp.ep_
        );
    }

public:
    sp_t
    lock() {
        return std::visit(
            [&](auto& ep) -> sp_t {
                return ep.lock();
            },
            ep_
        );
    }

    bool owner_before(this_type const& other) const {
        return std::visit(
            [&](auto const& lhs, auto const& rhs) {
                return lhs.owner_before(rhs);
            },
            ep_,
            other.ep_
        );
    }

private:
    ep_wp_t ep_;
};

template <role Role, typename... NextLayer>
using endpoint_wp_variant = basic_endpoint_wp_variant<Role, 2, NextLayer...>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_ENDPOINT_VARIANT_HPP
