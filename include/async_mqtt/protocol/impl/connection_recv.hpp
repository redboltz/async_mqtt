// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_RECV_HPP)
#define ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_RECV_HPP

#include <deque>

#include <async_mqtt/protocol/connection.hpp>
#include <async_mqtt/protocol/event_packet_received.hpp>
#include <async_mqtt/protocol/event_packet_id_released.hpp>
#include <async_mqtt/protocol/event_send.hpp>
#include <async_mqtt/protocol/event_recv.hpp>
#include <async_mqtt/protocol/event_close.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/shared_ptr_array.hpp>
#include <async_mqtt/util/inline.hpp>

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/impl/buffer_to_packet_variant.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)


namespace async_mqtt {

namespace detail {

struct error_packet {
    error_packet(error_code ec)
        :ec{ec} {}
    error_packet(buffer packet)
        :packet{force_move(packet)} {}

    error_code ec;
    buffer packet;
};

template <role Role, std::size_t PacketIdBytes>
class
basic_connection_impl<Role, PacketIdBytes>::
recv_packet_builder {
public:
    template <typename Begin, typename End>
    void recv(Begin b, End e) {
        using diff_type = typename std::iterator_traits<Begin>::difference_type;
        auto size = static_cast<std::size_t>(std::distance(b, e));
        while (b != e) {
            switch (read_state_) {
            case read_state::fixed_header: {
                auto fixed_header = *b++;
                --size;
                header_remaining_length_buf_.push_back(fixed_header);
                read_state_ = read_state::remaining_length;
            } break;
            case read_state::remaining_length: {
                while (size != 0) {
                    auto encoded_byte = *b++;
                    --size;
                    header_remaining_length_buf_.push_back(encoded_byte);
                    remaining_length_ += (std::uint8_t(encoded_byte) & 0b0111'1111) * multiplier_;
                    multiplier_ *= 128;
                    if ((encoded_byte & 0b1000'0000) == 0) {
                        raw_buf_size_ = header_remaining_length_buf_.size() + remaining_length_;
                        raw_buf_ = make_shared_ptr_char_array(raw_buf_size_);
                        raw_buf_ptr_ = raw_buf_.get();
                        std::copy_n(
                            header_remaining_length_buf_.data(),
                            header_remaining_length_buf_.size(),
                            raw_buf_ptr_
                        );
                        raw_buf_ptr_ += header_remaining_length_buf_.size();
                        if (remaining_length_ == 0) {
                            auto ptr = raw_buf_.get();
                            read_packets_.emplace_back(
                                buffer{ptr, raw_buf_size_, force_move(raw_buf_)}
                            );
                            initialize();
                            return;
                        }
                        else {
                            read_state_ = read_state::payload;
                        }
                        break;
                    }
                    if (multiplier_ == 128 * 128 * 128 * 128) {
                        read_packets_.emplace_back(make_error_code(disconnect_reason_code::packet_too_large));
                        initialize();
                        return;
                    }
                }
                if (read_state_ != read_state::payload) {
                    return;
                }
            } break;
            case read_state::payload: {
                if (size >= remaining_length_) {
                    auto end_pos = std::next(b, static_cast<diff_type>(remaining_length_));
                    std::copy(
                        b,
                        end_pos,
                        raw_buf_ptr_
                    );
                    b = end_pos;
                    raw_buf_ptr_ += remaining_length_;
                    size -= remaining_length_;
                    auto ptr = raw_buf_.get();
                    read_packets_.emplace_back(
                        buffer{ptr, raw_buf_size_, force_move(raw_buf_)}
                    );
                    initialize();
                }
                else {
                    std::copy(
                        b,
                        e,
                        raw_buf_ptr_
                    );
                    b = e;
                    raw_buf_ptr_ += size;
                    remaining_length_ -= size;
                    return;
                }
            } break;
            }
        }
    }

    error_packet front() const {
        return read_packets_.front();
    }

    void pop_front() {
        read_packets_.pop_front();
    }

    bool empty() const {
        return read_packets_.empty();
    }

    void initialize() {
        read_state_ = read_state::fixed_header;
        header_remaining_length_buf_.clear();
        remaining_length_ = 0;
        multiplier_ = 1;
        raw_buf_.reset();
        raw_buf_size_ = 0;
        raw_buf_ptr_ = nullptr;
    }


private:
    enum class read_state{fixed_header, remaining_length, payload} read_state_ = read_state::fixed_header;
    std::size_t remaining_length_ = 0;
    std::size_t multiplier_ = 1;
    static_vector<char, 5> header_remaining_length_buf_;
    std::shared_ptr<char []> raw_buf_;
    std::size_t raw_buf_size_ = 0;
    char* raw_buf_ptr_ = nullptr;
    std::deque<error_packet> read_packets_;

};

template <role Role, std::size_t PacketIdBytes>
template <typename Begin, typename End>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_connection_impl<Role, PacketIdBytes>::
recv(Begin b, End e) {
    rpb_.recv(b, e);
    return process_recv_packet();
}

} // namespace detail

template <role Role, std::size_t PacketIdBytes>
template <typename Begin, typename End>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_connection<Role, PacketIdBytes>::
recv(Begin b, End e) {
    BOOST_ASSERT(impl_);
    return impl_->recv(b, e);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_RECV_HPP
