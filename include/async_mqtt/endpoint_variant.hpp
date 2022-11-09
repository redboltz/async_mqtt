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
class basic_endpoint_variant {
    using endpoint_variant_type = std::variant<basic_endpoint<NextLayer, Role, PacketIdBytes>...>;

public:
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;
    using packet_variant_type = basic_packet_variant<PacketIdBytes>;

    template <typename NextLayerArg>
    basic_endpoint_variant(basic_endpoint<NextLayerArg, Role, PacketIdBytes> ep)
        : ep_{force_move(ep)}
    {}

    auto const& stream() const {
        return std::visit(
            [&](auto const& ep) {
                return ep.stream();
            },
            ep_
        );
    }

    auto& stream() {
        return std::visit(
            [&](auto& ep) {
                return ep.stream();
            },
            ep_
        );
    }

    auto const& strand() const {
        return std::visit(
            [&](auto const& ep) {
                return ep.stream().strand();
            },
            ep_
        );
    }
    auto& strand() {
        return std::visit(
            [&](auto& ep) {
                return ep.stream().strand();
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
            [&](auto const& ep) {
                return ep.acquire_unique_packet_id(
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
            [&](auto const& ep) {
                return ep.register_packet_id(
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
            [&](auto const& ep) {
                return ep.release_packet_id(
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
            [&](auto const& ep) {
                return ep.send(
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
            [&](auto const& ep) {
                return ep.recv(
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
            [&](auto const& ep) {
                return ep.restore(
                    force_move(pvs),
                    std::forward<CompletionToken>(token)
                );
            },
            ep_
        );
    }

private:
    endpoint_variant_type ep_;
};

template <role Role, typename... NextLayer>
using endpoint_variant = basic_endpoint_variant<Role, 2, NextLayer...>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_ENDPOINT_VARIANT_HPP
