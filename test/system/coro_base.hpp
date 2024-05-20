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

template <typename Ep, std::size_t PidBytes = 2>
struct coro_base : as::coroutine {
    using packet_id_t = typename am::basic_packet_id_type<PidBytes>::type;
    coro_base(Ep& ep)
        :eps_{ep}
    {}
    coro_base(std::vector<std::reference_wrapper<Ep>> eps)
        :eps_(am::force_move(eps))
    {}
    virtual ~coro_base() = default;
    void operator()() {
        proc({}, {}, {}, {});
    }
    void operator()(am::error_code const& ec, std::size_t = 0) {
        proc(ec, {}, {}, {});
    }
    void operator()(am::system_error const& se) {
        proc({}, se, {}, {});
    }
    void operator()(am::packet_variant pv) {
        proc({}, {}, am::force_move(pv), {});
    }
    void operator()(std::optional<packet_id_t> pid) {
        proc({}, {}, {}, am::force_move(pid));
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
        std::optional<am::error_code> ec,
        std::optional<am::system_error> se,
        std::optional<am::packet_variant> pv,
        std::optional<packet_id_t> pid
    ) = 0;
    std::vector<std::reference_wrapper<Ep>> eps_;
    std::shared_ptr<bool> finish_ = std::make_shared<bool>(false);
};

#endif // ASYNC_MQTT_TEST_SYSTEM_CORO_BASE_HPP
