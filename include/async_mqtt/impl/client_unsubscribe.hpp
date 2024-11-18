// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_UNSUBSCRIBE_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_UNSUBSCRIBE_HPP

#include <boost/hana/tuple.hpp>
#include <boost/hana/back.hpp>
#include <boost/hana/drop_back.hpp>
#include <boost/hana/unpack.hpp>

#include <async_mqtt/impl/client_impl.hpp>
#include <async_mqtt/util/log.hpp>

namespace async_mqtt {

namespace hana = boost::hana;

namespace detail {

template <protocol_version Version, typename NextLayer>
struct client_impl<Version, NextLayer>::
unsubscribe_op {
    this_type_sp cl;
    error_code ec;
    std::optional<typename client_type::unsubscribe_packet> packet;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_cl{*cl};
        if (ec) {
            self.complete(ec, std::nullopt);
            return;
        }
        auto pid = packet->packet_id();
        auto a_packet{force_move(*packet)};
        a_cl.ep_.async_send(
            force_move(a_packet),
            as::append(
                force_move(self),
                pid
            )
        );
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code const& ec,
        packet_id_type pid
    ) {
        auto& a_cl{*cl};
        if (ec) {
            self.complete(ec, std::nullopt);
            return;
        }

        auto tim = std::make_shared<as::steady_timer>(a_cl.ep_.get_executor());
        tim->expires_at(std::chrono::steady_clock::time_point::max());
        a_cl.pid_tim_pv_res_col_.get_tim_idx().emplace(pid, tim);
        tim->async_wait(
            as::append(
                force_move(self),
                tim
            )
        );
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code /* ec */,
        std::shared_ptr<as::steady_timer> tim
    ) {
        auto& a_cl{*cl};
        auto& idx = a_cl.pid_tim_pv_res_col_.get_tim_idx();
        auto it = idx.find(tim);
        if (it == idx.end()) {
            self.complete(
                make_error_code(as::error::operation_aborted),
                std::nullopt
            );
        }
        else {
            auto pv = it->pv;
            idx.erase(it);
            if (auto *p = pv->template get_if<typename client_type::unsuback_packet>()) {
                auto ec =
                    [&] {
                        if constexpr(Version == protocol_version::v5) {
                            switch (p->entries().size()) {
                            case 0:
                                return make_error_code(disconnect_reason_code::protocol_error);
                            case 1:
                                return make_error_code(p->entries().back());
                            default: {
                                bool all_error = true;
                                bool any_error = false;
                                for (auto code : p->entries()) {
                                    auto ec = make_error_code(code);
                                    all_error = all_error && ec;
                                    any_error = any_error || ec;
                                }
                                if (all_error) return make_error_code(mqtt_error::all_error_detected);
                                if (any_error) return make_error_code(mqtt_error::partial_error_detected);
                                return error_code{};
                            } break;
                            }
                        }
                        else {
                            return error_code{};
                        }
                    }();
                self.complete(ec, *p);
            }
            else {
                self.complete(
                    make_error_code(disconnect_reason_code::protocol_error),
                    std::nullopt
                );
            }
        }
    }
};

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
auto
client_impl<Version, NextLayer>::async_unsubscribe_impl(
    this_type_sp impl,
    error_code ec,
    std::optional<typename client_type::unsubscribe_packet> packet,
    CompletionToken&& token
) {
    if (packet) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, impl.get())
            << *packet;
    }
    BOOST_ASSERT(impl);
    auto exe = impl->get_executor();
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::optional<typename client_type::unsuback_packet>)
        >(
            unsubscribe_op{
                force_move(impl),
                ec,
                force_move(packet)
            },
            token,
            exe
        );
}

} // namespace detail

template <protocol_version Version, typename NextLayer>
template <typename... Args>
auto
client<Version, NextLayer>::async_unsubscribe(Args&&... args) {
    BOOST_ASSERT(impl_);
    if constexpr (std::is_constructible_v<unsubscribe_packet, decltype(std::forward<Args>(args))...>) {
        try {
            return impl_type::async_unsubscribe_impl(
                impl_,
                error_code{},
                unsubscribe_packet{std::forward<Args>(args)...}
            );
        }
        catch (system_error const& se) {
            return impl_type::async_unsubscribe_impl(
                impl_,
                se.code(),
                std::nullopt
            );
        }
    }
    else {
        auto all = hana::tuple<Args...>(std::forward<Args>(args)...);
        auto back = hana::back(all);
        auto rest = hana::drop_back(all, hana::size_c<1>);
        return hana::unpack(
            std::move(rest),
            [&](auto&&... rest_args) {
                static_assert(
                    std::is_constructible_v<
                        unsubscribe_packet,
                        decltype(rest_args)...
                    >,
                    "unsubscribe_packet is not constructible"
                );
                try {
                    return impl_type::async_unsubscribe_impl(
                        impl_,
                        error_code{},
                        unsubscribe_packet{std::forward<decltype(rest_args)>(rest_args)...},
                        force_move(back)
                    );
                }
                catch (system_error const& se) {
                    return impl_type::async_unsubscribe_impl(
                        impl_,
                        se.code(),
                        std::nullopt,
                        force_move(back)
                    );
                }
            }
        );
    }
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_UNSUBSCRIBE_HPP
