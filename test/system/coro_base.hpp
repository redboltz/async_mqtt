// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_TEST_SYSTEM_CORO_BASE_HPP)
#define ASYNC_MQTT_TEST_SYSTEM_CORO_BASE_HPP

#include <vector>
#include <functional>

#include <boost/asio.hpp>

#include <async_mqtt/all.hpp>

namespace as = boost::asio;
namespace am = async_mqtt;

template <typename Ep, std::size_t PacketIdBytes = 2>
struct coro_base : as::coroutine {
    coro_base(Ep& ep)
        :eps_{ep}
    {}
    coro_base(std::vector<std::reference_wrapper<Ep>> eps)
        :eps_(am::force_move(eps))
    {}

    virtual ~coro_base() = default;

    void operator()(am::error_code const& ec = am::error_code{}) {
        proc(ec, am::basic_packet_variant<PacketIdBytes>{}, 0);
    }
    void operator()(am::error_code const& ec, am::basic_packet_variant<PacketIdBytes> pv) {
        proc(ec, am::force_move(pv), 0);
    }
    void operator()(am::error_code const& ec, typename am::basic_packet_id_type<PacketIdBytes>::type pid) {
        proc(ec, am::basic_packet_variant<PacketIdBytes>{}, am::force_move(pid));
    }
    bool finish() const {
        return *finish_;
    }
protected:
    Ep& ep(std::size_t idx = 0) { return eps_.at(idx).get(); }
    Ep const& ep(std::size_t idx = 0) const { return eps_.at(idx).get(); }
    void set_finish() {
        *finish_ = true;
    }
private:
    virtual void proc(
        am::error_code ec,
        am::basic_packet_variant<PacketIdBytes> pv,
        typename am::basic_packet_id_type<PacketIdBytes>::type pid
    ) = 0;
    std::vector<std::reference_wrapper<Ep>> eps_;
    std::shared_ptr<bool> finish_ = std::make_shared<bool>(false);
};

#endif // ASYNC_MQTT_TEST_SYSTEM_CORO_BASE_HPP
