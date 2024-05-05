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

    null_strand(Executor exe) noexcept
        :exe_{std::move(exe)}
    {
    }
    null_strand() noexcept = default;
    null_strand(null_strand const&) noexcept = default;
    null_strand(null_strand&&) noexcept = default;
    template <typename OtherExecutor>
    null_strand(null_strand<OtherExecutor> const& other) noexcept
        :exe_{other.exe_}
    {
    }
    template <typename OtherExecutor>
    null_strand(null_strand<OtherExecutor>&& other) noexcept
        :exe_{std::move(other.exe_)}
    {
    }

    null_strand<Executor>& operator=(null_strand const& other) noexcept {
        exe_ = other.exe_;
        return *this;
    }
    null_strand<Executor>& operator=(null_strand&& other) noexcept {
        exe_ = std::move(other.exe_);
        return *this;
    }
    template <typename OtherExecutor>
    null_strand<Executor>& operator=(null_strand<OtherExecutor> const& other) noexcept {
        exe_ = other.exe_;
        return *this;
    }
    template <typename OtherExecutor>
    null_strand<Executor>& operator=(null_strand<OtherExecutor>&& other) noexcept {
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
        as::dispatch(
            as::bind_executor(
                exe_,
                as::bind_allocator(
                    a,
                    std::forward<Function>(f)
                )
            )
        );
    }
    template<
        typename Function,
        typename Allocator
    >
    void post(
        Function&& f,
        Allocator const& a) const {
        as::post(
            as::bind_executor(
                exe_,
                as::bind_allocator(
                    a,
                    std::forward<Function>(f)
                )
            )
        );
    }
    template<
        typename Function,
        typename Allocator
    >
    void defer(
        Function&& f,
        Allocator const& a) const {
        as::defer(
            as::bind_executor(
                exe_,
                as::bind_allocator(
                    a,
                    std::forward<Function>(f)
                )
            )
        );
    }

    void on_work_started() const noexcept {
    }
    void on_work_finished() const noexcept {
    }
    bool running_in_this_thread() const noexcept {
        return true;
    }

    template <typename Function>
    void execute(Function&& f) const {
        exe_.execute(std::forward<Function>(f));
    }


    as::execution_context& context() const noexcept {
        return exe_.context();
    }

    inner_executor_type get_inner_executor() const noexcept {
        return exe_;
    }

    friend bool operator==(
        null_strand<Executor> const& lhs,
        null_strand<Executor> const& rhs
    ) noexcept {
        return lhs.exe_ == rhs.exe_;
    }
    friend bool operator!=(
        null_strand<Executor> const& lhs,
        null_strand<Executor> const& rhs
    ) noexcept {
        return lhs.exe_ != rhs.exe_;
    }


    // customization points

    template <typename Property>
    typename as::query_result<const Executor&, Property>::type
    query_helper(
        std::false_type,
        Property const& property
    ) const {
        return as::query(exe_, property);
    }

    template <typename Property>
    as::execution::blocking_t
    query_helper(
        std::true_type,
        Property const& property
    ) const {
        as::execution::blocking_t result = as::query(exe_, property);
        return result == as::execution::blocking.always
            ? as::execution::blocking.possibly : result;
    }

    template <typename Property>
    typename as::constraint<
        as::can_query<Executor const&, Property>::value,
        typename std::conditional<
            std::is_convertible<Property, as::execution::blocking_t>::value,
            as::execution::blocking_t,
            typename as::query_result<Executor const&, Property>::type
        >::type
    >::type
    query(Property const& p) const
        BOOST_ASIO_NOEXCEPT_IF((as::is_nothrow_query<Executor const&, Property>::value))
    {
        return
            query_helper(
                std::is_convertible<Property, as::execution::blocking_t>(),
                p
            );
    }


    template <typename Property>
    typename as::constraint<
        as::can_prefer<Executor const&, Property>::value
        && !std::is_convertible<Property, as::execution::blocking_t::always_t>::value,
        null_strand<
            typename std::decay<
                typename as::prefer_result<Executor const&, Property>::type
            >::type
        >
    >::type
    prefer(Property const& p) const
        BOOST_ASIO_NOEXCEPT_IF((as::is_nothrow_prefer<Executor const&, Property>::value))
    {
        return
            null_strand<
                typename std::decay<
                    typename as::prefer_result<Executor const&, Property>::type
                >::type
            >(as::prefer(exe_, p));
    }

    template <typename Property>
    typename as::constraint<
        as::can_require<Executor const&, Property>::value
        && !std::is_convertible<Property, as::execution::blocking_t::always_t>::value,
        null_strand<
            typename std::decay<
                typename as::require_result<Executor const&, Property>::type
            >::type
        >
    >::type
    require(Property const& p) const
        BOOST_ASIO_NOEXCEPT_IF((as::is_nothrow_require<Executor const&, Property>::value))
    {
        return
            null_strand<
                typename std::decay<
                    typename as::require_result<Executor const&, Property>::type
                >::type
            >(as::require(exe_, p));
    }

private:
    inner_executor_type exe_;
};


} // namespace async_mqtt

#endif // ASYNC_MQTT_NULL_STRAND_HPP
