// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_PUBLISH_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_PUBLISH_HPP

#include <boost/hana/tuple.hpp>
#include <boost/hana/back.hpp>
#include <boost/hana/drop_back.hpp>
#include <boost/hana/unpack.hpp>

#include <async_mqtt/client.hpp>
#include <async_mqtt/impl/client_impl.hpp>
#include <async_mqtt/util/log.hpp>

namespace async_mqtt {

namespace hana = boost::hana;

namespace detail {

template <protocol_version Version, typename NextLayer>
struct client_impl<Version, NextLayer>::
publish_op {
    this_type_sp cl;
    error_code ec;
    std::optional<typename client_type::publish_packet> packet;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_cl{*cl};
        if (ec) {
            self.complete(ec, typename client_type::pubres_type{});
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
            self.complete(ec, typename client_type::pubres_type{});
            return;
        }
        if (pid == 0) {
            // QoS: at_most_once
            self.complete(ec, typename client_type::pubres_type{});
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
                typename client_type::pubres_type{}
            );
        }
        else {
            auto res = it->res;
            idx.erase(it);
            auto ec =
                [&] {
                    if constexpr(Version == protocol_version::v5) {
                        if (res.puback_opt) {
                            return make_error_code(res.puback_opt->code());
                        }
                        else if (res.pubrec_opt) {
                            auto ec = make_error_code(res.pubrec_opt->code());
                            if (ec) return ec;
                            if (res.pubcomp_opt) {
                                return make_error_code(res.pubcomp_opt->code());
                            }
                        }
                        return make_error_code(disconnect_reason_code::protocol_error);
                    }
                    else {
                        return error_code{};
                    }
                }();
            self.complete(ec, force_move(res));
        }
    }
};

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
auto
client_impl<Version, NextLayer>::async_publish_impl(
    this_type_sp impl,
    error_code ec,
    std::optional<typename client_type::publish_packet> packet,
    CompletionToken&& token
) {
    BOOST_ASSERT(impl);
    if (packet) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, impl.get())
            << *packet;
    }
    auto exe = impl->get_executor();
    return
        as::async_compose<
            CompletionToken,
            void(error_code, typename client_type::pubres_type)
        >(
            publish_op{
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
client<Version, NextLayer>::async_publish(Args&&... args) {
    BOOST_ASSERT(impl_);
    if constexpr (std::is_constructible_v<publish_packet, decltype(std::forward<Args>(args))...>) {
        try {
            return impl_type::async_publish_impl(
                impl_,
                error_code{},
                publish_packet{std::forward<Args>(args)...}
            );
        }
        catch (system_error const& se) {
            return impl_type::async_publish_impl(
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
                        publish_packet,
                        decltype(rest_args)...
                    >,
                    "publish_packet is not constructible"
                );
                try {
                    return impl_type::async_publish_impl(
                        impl_,
                        error_code{},
                        publish_packet{std::forward<decltype(rest_args)>(rest_args)...},
                        force_move(back)
                    );
                }
                catch (system_error const& se) {
                    return impl_type::async_publish_impl(
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

#endif // ASYNC_MQTT_IMPL_CLIENT_PUBLISH_HPP
