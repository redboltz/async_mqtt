// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_NULL_STRAND_HPP)
#define ASYNC_MQTT_NULL_STRAND_HPP

#include <cstddef>
#include <chrono>

#include <boost/asio.hpp>

#include <async_mqtt/type.hpp>

namespace async_mqtt {

namespace as = boost::asio;

template <typename Executor>
class null_strand {
public:
    using inner_executor_type = Executor;

    null_strand(Executor exe)
        :exe_{std::move(exe)}
    {
    }
    null_strand() = default;
    null_strand(null_strand<Executor> const&) = default;
    null_strand(null_strand<Executor>&&) = default;
    template <typename OtherExecutor>
    null_strand(null_strand<OtherExecutor> const& other)
        :exe_{std::move(other.exe_)}
    {
    }
    template <typename OtherExecutor>
    null_strand(null_strand<OtherExecutor>&& other)
        :exe_{std::move(other.exe_)}
    {
    }

    null_strand<Executor>& operator=(null_strand<Executor> const& other) {
        exe_ = other.exe_;
        return *this;
    }
    null_strand<Executor>& operator=(null_strand<Executor>&& other) {
        exe_ = std::move(other.exe_);
        return *this;
    }
    template <typename OtherExecutor>
    null_strand<Executor>& operator=(null_strand<OtherExecutor> const& other) {
        exe_ = other.exe_;
        return *this;
    }
    template <typename OtherExecutor>
    null_strand<Executor>& operator=(null_strand<OtherExecutor>&& other) {
        exe_ = std::move(other.exe_);
        return *this;
    }

    template<
        typename Function,
        typename Allocator
    >
    void dispatch(
        Function&& f,
        Allocator const& a) const {
        exe_.dispatch(std::forward<Function>(f), a);
    }
    template<
        typename Function,
        typename Allocator
    >
    void post(
        Function&& f,
        Allocator const& a) const {
        exe_.post(std::forward<Function>(f), a);
    }
    template<
        typename Function,
        typename Allocator
    >
    void defer(
        Function&& f,
        Allocator const& a) const {
        exe_.defer(std::forward<Function>(f), a);
    }

    void on_work_started() const {
        exe_.on_work_started();
    }
    void on_work_finished() const {
        exe_.on_work_finished();
    }
    bool running_in_this_thread() const {
        return true;
    }

    template <typename Function>
    void execute(Function&& f) const {
        exe_.execute(std::forward<Function>(f));
    }


    as::execution_context& context() const {
        return exe_.context();
    }

    inner_executor_type get_inner_executor() const {
        return exe_;
    }

    friend bool operator==(
        null_strand<Executor> const& lhs,
        null_strand<Executor> const& rhs
    ) {
        return lhs.exe_ == rhs.exe_;
    }
    friend bool operator!=(
        null_strand<Executor> const& lhs,
        null_strand<Executor> const& rhs
    ) {
        return lhs.exe_ != rhs.exe_;
    }

private:
    inner_executor_type exe_;
};


} // namespace async_mqtt

#endif // ASYNC_MQTT_NULL_STRAND_HPP
