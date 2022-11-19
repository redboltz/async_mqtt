// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_STUB_SOCKET_HPP)
#define ASYNC_MQTT_STUB_SOCKET_HPP

#include <deque>

#include <boost/asio.hpp>

#include <async_mqtt/buffer_to_packet_variant.hpp>

namespace async_mqtt {

namespace as = boost::asio;

struct stub_socket {
    using executor_type = as::io_context::executor_type;
    using pv_queue_t = std::deque<packet_variant>;
    using packet_iterator = packet_iterator<std::vector, as::const_buffer>;
    using packet_range = std::pair<packet_iterator, packet_iterator>;

    stub_socket(
        protocol_version version,
        as::io_context& ioc
    )
        :version_{version},
         ioc_{ioc}
    {}

    void set_write_packet_checker(std::function<void(packet_variant const& pv)> c) {
        write_packet_checker_ = force_move(c);
    }

    void set_recv_packets(std::deque<packet_variant> recv_pvs) {
        recv_pvs_ = force_move(recv_pvs);
        recv_pvs_it_ = recv_pvs_.begin();
        pv_r_ = nullopt;

    }

    void set_close_checker(std::function<void()> c) {
        close_checker_ = force_move(c);
    }

#if 0
    auto const& lowest_layer() const {
        return ioc_;
    }
    auto& lowest_layer() {
        return ioc_;
    }
#endif
    auto get_executor() const {
        return ioc_.get_executor();
    }
    auto get_executor() {
        return ioc_.get_executor();
    }
    template <typename ConstBufferSequence, typename CompletionToken>
    void async_write_some(
        ConstBufferSequence const& buffers,
        CompletionToken&& token
    ) {
        auto it = as::buffers_iterator<ConstBufferSequence>::begin(buffers);
        auto end = as::buffers_iterator<ConstBufferSequence>::end(buffers);
        auto buf = allocate_buffer(it, end);
        auto pv = buffer_to_packet_variant(buf, version_);
        if (write_packet_checker_) write_packet_checker_(pv);
        token(boost::system::error_code{}, std::distance(it, end));
    }

    template <typename MutableBufferSequence, typename CompletionToken>
    void async_read_some(
        MutableBufferSequence const& mb,
        CompletionToken&& token
    ) {
        // empty
        if (recv_pvs_it_ == recv_pvs_.end()) {
            token(errc::make_error_code(errc::no_message), 0);
            return;
        }
        if (auto* ec = recv_pvs_it_->get_if<system_error>()) {
            ++recv_pvs_it_;
            token(ec->code(), 0);
            return;
        }

        if (!pv_r_) {
            cbs_ = recv_pvs_it_->const_buffer_sequence();
            pv_r_ = make_packet_range(cbs_);
        }

        while (pv_r_->first == pv_r_->second) {
            pv_r_ = nullopt;
            ++recv_pvs_it_;
            if (recv_pvs_it_ == recv_pvs_.end()) {
                token(errc::make_error_code(errc::no_message), 0);
                return;
            }
            if (auto* ec = recv_pvs_it_->get_if<system_error>()) {
                ++recv_pvs_it_;
                token(ec->code(), 0);
                return;
            }
            cbs_ = recv_pvs_it_->const_buffer_sequence();
            pv_r_ = make_packet_range(cbs_);
        }

        BOOST_ASSERT(static_cast<std::size_t>(std::distance(pv_r_->first, pv_r_->second)) >= mb.size());
        as::mutable_buffer mb_copy = mb;
        std::copy(pv_r_->first, std::next(pv_r_->first, mb.size()), static_cast<char*>(mb_copy.data()));
        std::advance(pv_r_->first, mb.size());
        token(errc::make_error_code(errc::success), mb.size());
    }

    template<typename TeardownHandler>
    void async_teardown(TeardownHandler&& handler) const {
        if (close_checker_) close_checker_();
        handler(boost::system::error_code{});
    }

private:

    protocol_version version_;
    as::io_context& ioc_;
    pv_queue_t recv_pvs_;
    pv_queue_t::iterator recv_pvs_it_ = recv_pvs_.begin();
    std::vector<as::const_buffer> cbs_;
    optional<packet_range> pv_r_;
    std::function<void(packet_variant const& pv)> write_packet_checker_;
    std::function<void()> close_checker_;
};

template <typename MutableBufferSequence, typename CompletionToken>
void async_read(
    stub_socket& socket,
    MutableBufferSequence const& mb,
    CompletionToken&& token
) {
    socket.async_read_some(mb, std::forward<CompletionToken>(token));
}

template<typename TeardownHandler>
void async_teardown(
    role_type,
    stub_socket& socket,
    TeardownHandler&& handler) {
    socket.async_teardown(std::forward<TeardownHandler>(handler));
 }

} // namespace async_mqtt

#endif // ASYNC_MQTT_STUB_SOCKET_HPP
