// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_IOC_QUEUE_HPP)
#define ASYNC_MQTT_UTIL_IOC_QUEUE_HPP

#include <boost/asio.hpp>

#include <async_mqtt/util/optional.hpp>

namespace async_mqtt {

namespace as = boost::asio;

class ioc_queue {
public:
    ioc_queue() {
        queue_.stop();
    }

    void start_work() {
        working_ = true;
        guard_.emplace(queue_.get_executor());
    }

    void stop_work() {
        guard_.reset();
    }

    template <typename CompletionToken>
    bool post(CompletionToken&& token) {
        as::post(
            queue_,
            std::forward<CompletionToken>(token)
        );
        if (!working_ && queue_.stopped()) {
            queue_.restart();
            queue_.poll_one();
            return true;
        }
        return false;
    }

    bool stopped() const {
        return queue_.stopped();
    }

    std::size_t poll_one() {
        working_ = false;
        if (queue_.stopped()) queue_.restart();
        return queue_.poll_one();
    }

    std::size_t poll() {
        working_ = false;
        if (queue_.stopped()) queue_.restart();
        return queue_.poll();
    }

private:
    as::io_context queue_{BOOST_ASIO_CONCURRENCY_HINT_UNSAFE};
    bool working_ = false;
    optional<as::executor_work_guard<as::io_context::executor_type>> guard_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_IOC_QUEUE_HPP
