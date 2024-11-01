// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_IMPL_STREAM_READ_HPP)
#define ASYNC_MQTT_UTIL_IMPL_STREAM_READ_HPP

#include <async_mqtt/error.hpp>
#include <async_mqtt/util/stream.hpp>
#include <async_mqtt/util/shared_ptr_array.hpp>

namespace async_mqtt {

namespace detail {

template <typename NextLayer>
template <typename MutableBufferSequence>
struct stream_impl<NextLayer>::stream_read_some_op {
    using stream_type = this_type;
    using stream_type_sp = std::shared_ptr<stream_type>;
    using next_layer_type = stream_type::next_layer_type;

    std::shared_ptr<stream_type> strm;
    MutableBufferSequence const& buffers;
    
    enum { dispatch, work, complete } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_strm{*strm};
        switch (state) {
        case dispatch: {
            state = work;
            as::dispatch(
                a_strm.get_executor(),
                force_move(self)
            );
        } break;
        case work: {
            state = complete;
            // start bulk read
            if constexpr (
                has_async_read_some<next_layer_type>::value) {
                    layer_customize<next_layer_type>::async_read_some(
                        a_strm.nl_,
                        a_strm.read_buf_.prepare(a_strm.bulk_read_buffer_size_),
                        force_move(self)
                    );
            }
            else {
                a_strm.nl_.async_read_some(
                    a_strm.read_buf_.prepare(a_strm.bulk_read_buffer_size_),
                    force_move(self)
                );
            }
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    // finish bulk read
    template <typename Self>
    void operator()(
        Self& self,
        error_code const& ec,
        std::size_t bytes_transferred
    ) {
        auto& a_strm{*strm};
        self.complete(ec, bytes_transferred);
        if (!ec) {
            a_strm.read_buf_.commit(bytes_transferred);
        }
    }
};

} // namespace detail

template <typename NextLayer>
template <
    typename MutableBufferSequence,
    typename CompletionToken
>
auto
stream<NextLayer>::async_read_some(
    MutableBufferSequence const& buffers,
    CompletionToken&& token
) {
    BOOST_ASSERT(impl_);
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::size_t)
        >(
            typename impl_type::template stream_read_some_op<MutableBufferSequence>{
                impl_,
                buffers
            },
            token,
            get_executor()
        );
}


} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_IMPL_STREAM_READ_HPP
