// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_IMPL_IPP)
#define ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_IMPL_IPP

#include <deque>
#include <istream>

#include <async_mqtt/protocol/connection.hpp>
#include <async_mqtt/protocol/impl/connection_impl.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/shared_ptr_array.hpp>
#include <async_mqtt/util/inline.hpp>

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/impl/buffer_to_packet_variant.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

namespace async_mqtt {

namespace detail {

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::recv_packet_builder::
recv(std::istream& is) {
    BOOST_ASSERT(is);
    auto size = static_cast<std::size_t>(is.rdbuf()->in_avail());
    while (size != 0) {
        switch (read_state_) {
        case read_state::fixed_header: {
            char fixed_header;
            auto ret = is.readsome(&fixed_header, 1);
            BOOST_ASSERT(ret == 1);
            --size;
            header_remaining_length_buf_.push_back(fixed_header);
            read_state_ = read_state::remaining_length;
        } break;
        case read_state::remaining_length: {
            while (size != 0) {
                char encoded_byte;
                auto ret = is.readsome(&encoded_byte, 1);
                BOOST_ASSERT(ret == 1);
                --size;
                header_remaining_length_buf_.push_back(encoded_byte);
                remaining_length_ += (std::uint8_t(encoded_byte) & 0b0111'1111) * multiplier_;
                BOOST_ASSERT(remaining_length_ < 2000);
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
                        BOOST_ASSERT(read_packets_.size() < 100000);
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
                    BOOST_ASSERT(read_packets_.size() < 100000);
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
            auto copied = is.readsome(raw_buf_ptr_, static_cast<std::streamsize>(remaining_length_));
            BOOST_ASSERT(copied > 0);
            auto copied_size = static_cast<std::size_t>(copied);
            size -= copied_size;
            if (copied_size == remaining_length_) {
                auto ptr = raw_buf_.get();
                BOOST_ASSERT(read_packets_.size() < 100000);
                read_packets_.emplace_back(
                    buffer{ptr, raw_buf_size_, force_move(raw_buf_)}
                );
                initialize();
            }
            else {
                raw_buf_ptr_ += copied_size;
                remaining_length_ -= copied_size;
                return;
            }
        } break;
        }
    }
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
typename basic_connection_impl<Role, PacketIdBytes>::error_packet
basic_connection_impl<Role, PacketIdBytes>::recv_packet_builder::
front() const {
    return read_packets_.front();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::recv_packet_builder::
pop_front() {
    read_packets_.pop_front();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_connection_impl<Role, PacketIdBytes>::recv_packet_builder::
empty() const {
    return read_packets_.empty();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::recv_packet_builder::
initialize() {
    read_state_ = read_state::fixed_header;
    header_remaining_length_buf_.clear();
    remaining_length_ = 0;
    multiplier_ = 1;
    raw_buf_.reset();
    raw_buf_size_ = 0;
    raw_buf_ptr_ = nullptr;
}

// public
template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_connection_impl<Role, PacketIdBytes>::
basic_connection_impl(
    protocol_version ver,
    basic_connection<Role, PacketIdBytes>& con
)
    : con_{con},
      protocol_version_{ver}
{
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
recv(std::istream& is) {
    rpb_.recv(is);
    return process_recv_packet();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
notify_timer_fired(timer_kind kind) {
    switch (kind) {
    case timer_kind::pingreq_send:
        switch (protocol_version_) {
        case protocol_version::v3_1_1:
            send(v3_1_1::pingreq_packet{});
            break;
        case protocol_version::v5:
            send(v5::pingreq_packet{});
            break;
        default:
            BOOST_ASSERT(false);
            break;
        }
        break;
    case timer_kind::pingreq_recv:
        switch (protocol_version_) {
        case protocol_version::v3_1_1:
            con_.on_close();
            break;
        case protocol_version::v5:
            send(
                v5::disconnect_packet{
                    disconnect_reason_code::keep_alive_timeout,
                    properties{}
                }
            );
            break;
        default:
            BOOST_ASSERT(false);
            break;
        }
        break;
    case timer_kind::pingresp_recv:
        switch (protocol_version_) {
        case protocol_version::v3_1_1:
            con_.on_close();
            break;
        case protocol_version::v5:
            send(
                v5::disconnect_packet{
                    disconnect_reason_code::keep_alive_timeout,
                    properties{}
                }
            );
            break;
        default:
            BOOST_ASSERT(false);
            break;
        }
        break;
    default:
        BOOST_ASSERT(false);
        break;
    }
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
notify_closed() {
    status_ = connection_status::disconnected;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
set_pingreq_send_interval(
    std::chrono::milliseconds duration
) {
    if (duration == std::chrono::milliseconds::zero()) {
        pingreq_send_interval_ms_.reset();
        con_.on_timer_op(
            timer_op::cancel,
            timer_kind::pingreq_send
        );
    }
    else {
        pingreq_send_interval_ms_.emplace(duration);
        con_.on_timer_op(
            timer_op::reset,
            timer_kind::pingreq_send,
            duration
        );
    }
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t
basic_connection_impl<Role, PacketIdBytes>::
get_receive_maximum_vacancy_for_send() const {
    return publish_send_max_ - publish_send_count_;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
set_auto_pub_response(bool val) {
    auto_pub_response_ = val;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
set_offline_publish(bool val) {
    offline_publish_ = val;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
set_auto_ping_response(bool val) {
    auto_ping_response_ = val;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
set_auto_map_topic_alias_send(bool val) {
    auto_map_topic_alias_send_ = val;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
set_auto_replace_topic_alias_send(bool val) {
    auto_replace_topic_alias_send_ = val;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
set_pingresp_recv_timeout(std::chrono::milliseconds duration) {
    if (duration == std::chrono::milliseconds::zero()) {
        pingresp_recv_timeout_ms_.reset();
    }
    else {
        pingresp_recv_timeout_ms_.emplace(duration);
    }
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
basic_connection_impl<Role, PacketIdBytes>::
acquire_unique_packet_id() {
    return pid_man_.acquire_unique_id();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_connection_impl<Role, PacketIdBytes>::
register_packet_id(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id
) {
    return pid_man_.register_id(packet_id);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
release_packet_id(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id
) {
    pid_man_.release_id(packet_id);
    con_.on_packet_id_release(packet_id);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::set<typename basic_packet_id_type<PacketIdBytes>::type>
basic_connection_impl<Role, PacketIdBytes>::
get_qos2_publish_handled_pids() const {
    return qos2_publish_handled_;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
restore_qos2_publish_handled_pids(
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> pids
) {
    qos2_publish_handled_ = force_move(pids);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
restore_packets(
    std::vector<basic_store_packet_variant<PacketIdBytes>> pvs
) {
    auto add_publish =
        [&](auto const& p) {
            if (p.opts().get_qos() == qos::at_least_once) {
                pid_puback_.insert(p.packet_id());
            }
            else if (p.opts().get_qos() == qos::exactly_once) {
                pid_pubrec_.insert(p.packet_id());
            }
        };
    auto add_store =
        [&] (auto&& p) {
            if (pid_man_.register_id(p.packet_id())) {
                store_.add(std::forward<decltype(p)>(p));
            }
            else {
                ASYNC_MQTT_LOG("mqtt_impl", error)
                    << "packet_id:" << p.packet_id()
                    << " has already been used. Skip it";
            }
        };

    for (auto& pv : pvs) {
        pv.visit(
            overload {
                [&](v3_1_1::basic_publish_packet<PacketIdBytes>& p) {
                    add_publish(p);
                    add_store(force_move(p));
                },
                [&](v5::basic_publish_packet<PacketIdBytes>& p) {
                    add_publish(p);
                    add_store(force_move(p));
                },
                [&](v3_1_1::basic_pubrel_packet<PacketIdBytes>& p) {
                    pid_pubcomp_.insert(p.packet_id());
                    add_store(force_move(p));
                },
                [&](v5::basic_pubrel_packet<PacketIdBytes>& p) {
                    pid_pubcomp_.insert(p.packet_id());
                    add_store(force_move(p));
                }
            }
        );
    }
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_store_packet_variant<PacketIdBytes>>
basic_connection_impl<Role, PacketIdBytes>::
get_stored_packets() const {
    return store_.get_stored();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
protocol_version
basic_connection_impl<Role, PacketIdBytes>::
get_protocol_version() const {
    return protocol_version_;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_connection_impl<Role, PacketIdBytes>::
is_publish_processing(typename basic_packet_id_type<PacketIdBytes>::type pid) const {
    return qos2_publish_processing_.find(pid) != qos2_publish_processing_.end();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
error_code
basic_connection_impl<Role, PacketIdBytes>::
regulate_for_store(
    v5::basic_publish_packet<PacketIdBytes>& packet
) const {
    if (packet.topic().empty()) {
        if (auto ta_opt =
            get_topic_alias(packet.props())) {
            auto topic = topic_alias_send_->find_without_touch(*ta_opt);
            if (topic.empty()) {
                return make_error_code(
                    mqtt_error::packet_not_regulated
                );
            }
            packet.remove_topic_alias_add_topic(force_move(topic));
        }
        else {
            return make_error_code(
                mqtt_error::packet_not_regulated
            );
        }
    }
    else {
        packet.remove_topic_alias();
    }
    return error_code{};
}

// private

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
send_stored() {
    store_.for_each(
        [&](basic_store_packet_variant<PacketIdBytes> const& pv) mutable {
            if (pv.size() > maximum_packet_size_send_) {
                pid_man_.release_id(pv.packet_id());
                con_.on_packet_id_release(pv.packet_id());
                return false;
            }
            pv.visit(
                // copy packet because the stored packets need to be preserved
                // until receiving puback/pubrec/pubcomp
                [&](auto const& p) {
                    con_.on_send(p);
                }
            );
            return true;
        }
   );
}




// 1. ec, close (netowork level error, v3.1.1 packet error)
// 2. ec, send_disconnect, close (packet error after connected
// 3. ec, send_connack, close (packet error before connect)
// 4. packet_received, [auto_res,] [pingreq_recv_reset]

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
process_recv_packet() {
    while (!rpb_.empty()) {
        auto ep = rpb_.front();
        rpb_.pop_front();
        if (ep.ec) {
            con_.on_error(ep.ec);
            con_.on_close();
            status_ = connection_status::disconnected;
            return;
        }
        auto& buf{ep.packet};

        // Checking maximum_packet_size
        if (buf.size() > maximum_packet_size_recv_) {
            // on v3.1.1 maximum_packet_size_recv_ is initialized as packet_size_no_limit
            BOOST_ASSERT(protocol_version_ == protocol_version::v5);
            con_.on_error(
                make_error_code(
                    disconnect_reason_code::packet_too_large
                )
            );
            con_.on_send(
                v5::disconnect_packet{
                    disconnect_reason_code::packet_too_large
                }
            );
            return;
        }

        error_code ec;
        auto pv_opt = buffer_to_basic_packet_variant<PacketIdBytes>(buf, protocol_version_, ec);
        if (ec) {
            if (status_ == connection_status::disconnected &&
                ec.category() == get_connect_reason_code_category()
            ) {
                // first received connect packet with error

                // packet is error but connack needs to be sent
                status_ = connection_status::connecting;
                con_.on_error(
                    make_error_code(
                        static_cast<connect_reason_code>(ec.value())
                    )
                );
                if (protocol_version_ == protocol_version::v5) {
                    con_.on_send(
                        v5::connack_packet{
                            false, // session_present
                            static_cast<connect_reason_code>(ec.value())
                        }
                    );
                }
                else {
                    con_.on_send(
                        v3_1_1::connack_packet{
                            false, // session_present
                            static_cast<connect_return_code>(ec.value())
                        }
                    );
                }
            }
            else {
                con_.on_error(
                    make_error_code(
                        static_cast<disconnect_reason_code>(ec.value())
                    )
                );
                if (status_ == connection_status::connected &&
                    protocol_version_ == protocol_version::v5
                ) {
                    con_.on_send(
                        v5::disconnect_packet{
                            static_cast<disconnect_reason_code>(ec.value())
                        }
                    );
                }
                else {
                    con_.on_close();
                }
            }
            return;
        }

        // no errors on packet creation phase
        BOOST_ASSERT(pv_opt);
        auto& pv{*pv_opt};
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << "recv:" << pv;
        auto result = pv.visit(
            // do internal protocol processing
            overload {
                [&](v3_1_1::connect_packet& p) {
                    initialize(false);
                    protocol_version_ = protocol_version::v3_1_1;
                    status_ = connection_status::connecting;
                    auto keep_alive = p.keep_alive();
                    if (keep_alive != 0) {
                        pingreq_recv_timeout_ms_.emplace(
                            std::chrono::milliseconds{
                                keep_alive * 1000 * 3 / 2
                            }
                        );
                    }
                    if (p.clean_session()) {
                        need_store_ = false;
                        pid_puback_.clear();
                        pid_pubrec_.clear();
                        pid_pubcomp_.clear();
                    }
                    else {
                        need_store_ = true;
                    }
                    con_.on_receive(pv);
                    return true;
                },
                [&](v5::connect_packet& p) {
                    initialize(false);
                    protocol_version_ = protocol_version::v5;
                    status_ = connection_status::connecting;
                    auto keep_alive = p.keep_alive();
                    if (keep_alive != 0) {
                        pingreq_recv_timeout_ms_.emplace(
                            std::chrono::milliseconds{
                                keep_alive * 1000 * 3 / 2
                            }
                        );
                    }
                    if (p.clean_start()) {
                        pid_puback_.clear();
                        pid_pubrec_.clear();
                        pid_pubcomp_.clear();
                    }
                    for (auto const& prop : p.props()) {
                        prop.visit(
                            overload {
                                [&](property::topic_alias_maximum const& p) {
                                    if (p.val() > 0) {
                                        topic_alias_send_.emplace(p.val());
                                    }
                                },
                                [&](property::receive_maximum const& p) {
                                    BOOST_ASSERT(p.val() != 0);
                                    publish_send_max_ = p.val();
                                },
                                [&](property::maximum_packet_size const& p) {
                                    BOOST_ASSERT(p.val() != 0);
                                    maximum_packet_size_send_ = p.val();
                                },
                                [&](property::session_expiry_interval const& p) {
                                    if (p.val() != 0) {
                                        need_store_ = true;
                                    }
                                },
                                [](auto const&) {
                                }
                            }
                        );
                    }
                    con_.on_receive(p);
                    return true;
                },
                [&](v3_1_1::connack_packet& p) {
                    if (p.code() == connect_return_code::accepted) {
                        status_ = connection_status::connected;
                        if (p.session_present()) {
                            send_stored();
                        }
                        else {
                            pid_man_.clear();
                            store_.clear();
                            pid_puback_.clear();
                            pid_pubrec_.clear();
                            pid_pubcomp_.clear();
                        }
                    }
                    con_.on_receive(p);
                    return true;
                },
                [&](v5::connack_packet& p) {
                    if (p.code() == connect_reason_code::success) {
                        status_ = connection_status::connected;
                         for (auto const& prop : p.props()) {
                            prop.visit(
                                overload {
                                    [&](property::topic_alias_maximum const& p) {
                                        if (p.val() > 0) {
                                            topic_alias_send_.emplace(p.val());
                                        }
                                    },
                                    [&](property::receive_maximum const& p) {
                                        BOOST_ASSERT(p.val() != 0);
                                        publish_send_max_ = p.val();
                                    },
                                    [&](property::maximum_packet_size const& p) {
                                        BOOST_ASSERT(p.val() != 0);
                                        maximum_packet_size_send_ = p.val();
                                    },
                                    [&](property::server_keep_alive const& p) {
                                        if constexpr (can_send_as_client(Role)) {
                                            set_pingreq_send_interval(
                                                std::chrono::seconds{p.val()}
                                            );
                                        }
                                    },
                                    [](auto const&) {
                                    }
                                }
                            );
                        }

                        if (p.session_present()) {
                            send_stored();
                        }
                        else {
                            pid_man_.clear();
                            store_.clear();
                            pid_puback_.clear();
                            pid_pubrec_.clear();
                            pid_pubcomp_.clear();
                        }
                    }
                    con_.on_receive(p);
                    return true;
                },
                [&](v3_1_1::basic_publish_packet<PacketIdBytes>& p) {
                    switch (p.opts().get_qos()) {
                    case qos::at_most_once:
                        con_.on_receive(p);
                        break;
                    case qos::at_least_once: {
                        con_.on_receive(p);
                        auto packet_id = p.packet_id();
                        if (auto_pub_response_ &&
                            status_ == connection_status::connected) {
                            con_.on_send(
                                v3_1_1::basic_puback_packet<PacketIdBytes>(packet_id)
                            );
                        }
                    } break;
                    case qos::exactly_once: {
                        auto packet_id = p.packet_id();
                        bool already_handled = false;
                        if (qos2_publish_handled_.find(packet_id) == qos2_publish_handled_.end()) {
                            qos2_publish_handled_.emplace(packet_id);
                        }
                        else {
                            already_handled = true;
                        }
                        if (!already_handled) {
                            con_.on_receive(p);
                        }
                        if (status_ == connection_status::connected &&
                            (auto_pub_response_ ||
                             already_handled) // already_handled is true only if the pubrec packet
                        ) {                   // corresponding to the publish packet has already
                            con_.on_send(
                                v3_1_1::basic_pubrec_packet<PacketIdBytes>(packet_id)
                            );
                        }
                    } break;
                    default:
                        break;
                    }
                    return true;
                },
                [&](v5::basic_publish_packet<PacketIdBytes>& p) {
                    bool already_handled = false;
                    bool puback_send = false;
                    bool pubrec_send = false;
                    auto packet_id = p.packet_id();
                    switch (p.opts().get_qos()) {
                    case qos::at_least_once: {
                        if (publish_recv_.size() == publish_recv_max_) {
                            con_.on_error(
                                make_error_code(
                                    disconnect_reason_code::receive_maximum_exceeded
                                )
                            );
                            con_.on_send(
                                v5::disconnect_packet{
                                    disconnect_reason_code::receive_maximum_exceeded
                                }
                            );
                            return false;
                        }
                        publish_recv_.insert(packet_id);
                        if (auto_pub_response_ && status_ == connection_status::connected) {
                            puback_send = true;
                        }
                    } break;
                    case qos::exactly_once: {
                        auto packet_id = p.packet_id();
                        if (publish_recv_.size() == publish_recv_max_) {
                            con_.on_error(
                                make_error_code(
                                    disconnect_reason_code::receive_maximum_exceeded
                                )
                            );
                            con_.on_send(
                                v5::disconnect_packet{
                                    disconnect_reason_code::receive_maximum_exceeded
                                }
                            );
                            return false;
                        }
                        publish_recv_.insert(packet_id);

                        if (qos2_publish_handled_.find(packet_id) == qos2_publish_handled_.end()) {
                            qos2_publish_handled_.emplace(packet_id);
                        }
                        else {
                            already_handled = true;
                        }
                        if (status_ == connection_status::connected &&
                            (auto_pub_response_ ||
                             already_handled) // already_handled is true only if the pubrec packet
                        ) {                   // corresponding to the publish packet has already
                            pubrec_send = true;
                        }
                    } break;
                    default:
                        break;
                    }

                    if (p.topic().empty()) {
                        if (auto ta_opt = get_topic_alias(p.props())) {
                            // extract topic from topic_alias
                            if (*ta_opt == 0 ||
                                !topic_alias_recv_ || // topic_alias_maximum is 0
                                *ta_opt > topic_alias_recv_->max()) {
                                con_.on_error(
                                    make_error_code(
                                        disconnect_reason_code::topic_alias_invalid
                                    )
                                );
                                con_.on_send(
                                    v5::disconnect_packet{
                                        disconnect_reason_code::topic_alias_invalid
                                    }
                                );
                                return false;
                            }
                            BOOST_ASSERT(topic_alias_recv_);
                            auto topic = topic_alias_recv_->find(*ta_opt);
                            if (topic.empty()) {
                                ASYNC_MQTT_LOG("mqtt_impl", error)
                                    << "no matching topic alias: "
                                    << *ta_opt;
                                con_.on_error(
                                    make_error_code(
                                        disconnect_reason_code::topic_alias_invalid
                                    )
                                );
                                con_.on_send(
                                    v5::disconnect_packet{
                                        disconnect_reason_code::topic_alias_invalid
                                    }
                                );
                                return false;
                            }
                            else {
                                p.add_topic(force_move(topic));
                            }
                        }
                        else {
                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                << "topic is empty but topic_alias isn't set";
                            con_.on_error(
                                make_error_code(
                                    disconnect_reason_code::topic_alias_invalid
                                )
                            );
                            con_.on_send(
                                v5::disconnect_packet{
                                    disconnect_reason_code::topic_alias_invalid
                                }
                            );
                            return false;
                        }
                    }
                    else {
                        if (auto ta_opt = get_topic_alias(p.props())) {
                            if (*ta_opt == 0 ||
                                !topic_alias_recv_ || // topic_alias_maximum is 0
                                *ta_opt > topic_alias_recv_->max()) {
                                con_.on_error(
                                    make_error_code(
                                        disconnect_reason_code::topic_alias_invalid
                                    )
                                );
                                con_.on_send(
                                    v5::disconnect_packet{
                                        disconnect_reason_code::topic_alias_invalid
                                    }
                                );
                                return false;
                            }
                            BOOST_ASSERT(topic_alias_recv_);
                            // extract topic from topic_alias
                            topic_alias_recv_->insert_or_update(p.topic(), *ta_opt);
                        }
                    }
                    if (!already_handled) {
                        con_.on_receive(p);
                    }
                    if (puback_send) {
                        con_.on_send(
                            v5::basic_puback_packet<PacketIdBytes>(packet_id)
                        );
                    }
                    if (pubrec_send) {
                        con_.on_send(
                            v5::basic_pubrec_packet<PacketIdBytes>(packet_id)
                        );
                    }
                    return true;
                },
                [&](v3_1_1::basic_puback_packet<PacketIdBytes>& p) {
                    auto packet_id = p.packet_id();
                    if (pid_puback_.erase(packet_id)) {
                        con_.on_receive(p);
                        store_.erase(response_packet::v3_1_1_puback, packet_id);
                        pid_man_.release_id(packet_id);
                        con_.on_packet_id_release(packet_id);
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", error)
                            << "invalid packet_id puback received packet_id:" << packet_id;
                        con_.on_error(
                            make_error_code(
                                disconnect_reason_code::protocol_error
                            )
                        );
                        con_.on_close();
                        return false;
                    }
                    return true;
                },
                [&](v5::basic_puback_packet<PacketIdBytes>& p) {
                    auto packet_id = p.packet_id();
                    if (pid_puback_.erase(packet_id)) {
                        con_.on_receive(p);
                        store_.erase(response_packet::v5_puback, packet_id);
                        pid_man_.release_id(packet_id);
                        con_.on_packet_id_release(packet_id);
                        --publish_send_count_;
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", error)
                            << "invalid packet_id puback received packet_id:" << packet_id;
                        con_.on_error(
                            make_error_code(
                                disconnect_reason_code::protocol_error
                            )
                        );
                        con_.on_send(
                            v5::disconnect_packet{
                                disconnect_reason_code::protocol_error
                            }
                        );
                        return false;
                    }
                    return true;
                },
                [&](v3_1_1::basic_pubrec_packet<PacketIdBytes>& p) {
                    auto packet_id = p.packet_id();
                    if (pid_pubrec_.erase(packet_id)) {
                        store_.erase(response_packet::v3_1_1_pubrec, packet_id);
                        con_.on_receive(p);
                        if (auto_pub_response_ && status_ == connection_status::connected) {
                            con_.on_send(
                                v3_1_1::basic_pubrel_packet<PacketIdBytes>{packet_id}
                            );
                        }
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", error)
                            << "invalid packet_id pubrec received packet_id:" << packet_id;
                        con_.on_error(
                            make_error_code(
                                disconnect_reason_code::protocol_error
                            )
                        );
                        con_.on_close();
                        return false;
                    }
                    return true;
                },
                [&](v5::basic_pubrec_packet<PacketIdBytes>& p) {
                    auto packet_id = p.packet_id();
                    if (pid_pubrec_.erase(packet_id)) {
                        store_.erase(response_packet::v5_pubrec, packet_id);
                        con_.on_receive(p);
                        if (make_error_code(p.code())) {
                            pid_man_.release_id(packet_id);
                            con_.on_packet_id_release(packet_id);
                            qos2_publish_processing_.erase(packet_id);
                            --publish_send_count_;
                        }
                        else if (auto_pub_response_ && status_ == connection_status::connected) {
                            con_.on_send(
                                v5::basic_pubrel_packet<PacketIdBytes>{packet_id}
                            );
                        }
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", error)
                            << "invalid packet_id pubrec received packet_id:" << packet_id;
                        con_.on_error(
                            make_error_code(
                                disconnect_reason_code::protocol_error
                            )
                        );
                        con_.on_send(
                            v5::disconnect_packet{
                                disconnect_reason_code::protocol_error
                            }
                        );
                        return false;
                    }
                    return true;
                },
                [&](v3_1_1::basic_pubrel_packet<PacketIdBytes>& p) {
                    auto packet_id = p.packet_id();
                    con_.on_receive(p);
                    qos2_publish_handled_.erase(packet_id);
                    if (auto_pub_response_ && status_ == connection_status::connected) {
                        con_.on_send(
                            v3_1_1::basic_pubcomp_packet<PacketIdBytes>{packet_id}
                        );
                    }
                    return true;
                },
                [&](v5::basic_pubrel_packet<PacketIdBytes>& p) {
                    auto packet_id = p.packet_id();
                    con_.on_receive(p);
                    qos2_publish_handled_.erase(packet_id);
                    if (auto_pub_response_ && status_ == connection_status::connected) {
                        con_.on_send(
                            v5::basic_pubcomp_packet<PacketIdBytes>{packet_id}
                        );
                    }
                    return true;
                },
                [&](v3_1_1::basic_pubcomp_packet<PacketIdBytes>& p) {
                    auto packet_id = p.packet_id();
                    if (pid_pubcomp_.erase(packet_id)) {
                        store_.erase(response_packet::v3_1_1_pubcomp, packet_id);
                        con_.on_receive(p);
                        pid_man_.release_id(packet_id);
                        con_.on_packet_id_release(packet_id);
                        qos2_publish_processing_.erase(packet_id);
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", error)
                            << "invalid packet_id pubcomp received packet_id:" << packet_id;
                        con_.on_error(
                            make_error_code(
                                disconnect_reason_code::protocol_error
                            )
                        );
                        con_.on_close();
                        return false;
                    }
                    return true;
                },
                [&](v5::basic_pubcomp_packet<PacketIdBytes>& p) {
                    auto packet_id = p.packet_id();
                    if (pid_pubcomp_.erase(packet_id)) {
                        store_.erase(response_packet::v5_pubcomp, packet_id);
                        con_.on_receive(p);
                        pid_man_.release_id(packet_id);
                        con_.on_packet_id_release(packet_id);
                        qos2_publish_processing_.erase(packet_id);
                        --publish_send_count_;
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", error)
                            << "invalid packet_id pubcomp received packet_id:" << packet_id;
                        con_.on_error(
                            make_error_code(
                                disconnect_reason_code::protocol_error
                            )
                        );
                        con_.on_send(
                            v5::disconnect_packet{
                                disconnect_reason_code::protocol_error
                            }
                        );
                        return false;
                    }
                    return true;
                },
                [&](v3_1_1::basic_subscribe_packet<PacketIdBytes>& p) {
                    con_.on_receive(p);
                    return true;
                },
                [&](v5::basic_subscribe_packet<PacketIdBytes>& p) {
                    con_.on_receive(p);
                    return true;
                },
                [&](v3_1_1::basic_suback_packet<PacketIdBytes>& p) {
                    auto packet_id = p.packet_id();
                    if (pid_suback_.erase(packet_id)) {
                        con_.on_receive(p);
                        pid_man_.release_id(packet_id);
                        con_.on_packet_id_release(packet_id);
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", error)
                            << "invalid packet_id suback received packet_id:" << packet_id;
                        con_.on_error(
                            make_error_code(
                                disconnect_reason_code::protocol_error
                            )
                        );
                        con_.on_close();
                        return false;
                    }
                    return true;
                },
                [&](v5::basic_suback_packet<PacketIdBytes>& p) {
                    auto packet_id = p.packet_id();
                    if (pid_suback_.erase(packet_id)) {
                        con_.on_receive(p);
                        pid_man_.release_id(packet_id);
                        con_.on_packet_id_release(packet_id);
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", error)
                            << "invalid packet_id suback received packet_id:" << packet_id;
                        con_.on_error(
                            make_error_code(
                                disconnect_reason_code::protocol_error
                            )
                        );
                        con_.on_send(
                            v5::disconnect_packet{
                                disconnect_reason_code::protocol_error
                            }
                        );
                        con_.on_close();
                        return false;
                    }
                    return true;
                },
                [&](v3_1_1::basic_unsubscribe_packet<PacketIdBytes>& p) {
                    con_.on_receive(p);
                    return true;
                },
                [&](v5::basic_unsubscribe_packet<PacketIdBytes>& p) {
                    con_.on_receive(p);
                    return true;
                },
                [&](v3_1_1::basic_unsuback_packet<PacketIdBytes>& p) {
                    auto packet_id = p.packet_id();
                    if (pid_unsuback_.erase(packet_id)) {
                        con_.on_receive(p);
                        pid_man_.release_id(packet_id);
                        con_.on_packet_id_release(packet_id);
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", error)
                            << "invalid packet_id unsuback received packet_id:" << packet_id;
                        con_.on_error(
                            make_error_code(
                                disconnect_reason_code::protocol_error
                            )
                        );
                        con_.on_close();
                        return false;
                    }
                    return true;
                },
                [&](v5::basic_unsuback_packet<PacketIdBytes>& p) {
                    auto packet_id = p.packet_id();
                    if (pid_unsuback_.erase(packet_id)) {
                        con_.on_receive(p);
                        pid_man_.release_id(packet_id);
                        con_.on_packet_id_release(packet_id);
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", error)
                            << "invalid packet_id unsuback received packet_id:" << packet_id;
                        con_.on_error(
                            make_error_code(
                                disconnect_reason_code::protocol_error
                            )
                        );
                        con_.on_send(
                            v5::disconnect_packet{
                                disconnect_reason_code::protocol_error
                            }
                        );
                        con_.on_close();
                        return false;
                    }
                    return true;
                },
                [&](v3_1_1::pingreq_packet& p) {
                    con_.on_receive(p);
                    if constexpr(can_send_as_server(Role)) {
                        if (auto_ping_response_ &&
                            status_ == connection_status::connected) {
                            con_.on_send(
                                v3_1_1::pingresp_packet{}
                            );
                        }
                    }
                    return true;
                },
                [&](v5::pingreq_packet& p) {
                    con_.on_receive(p);
                    if constexpr(can_send_as_server(Role)) {
                        if (auto_ping_response_ &&
                            status_ == connection_status::connected) {
                            con_.on_send(
                                v5::pingresp_packet{}
                            );
                        }
                    }
                    return true;
                },
                [&](v3_1_1::pingresp_packet& p) {
                    con_.on_receive(p);
                    con_.on_timer_op(
                        timer_op::cancel,
                        timer_kind::pingresp_recv
                    );
                    return true;
                },
                [&](v5::pingresp_packet& p) {
                    con_.on_receive(p);
                    con_.on_timer_op(
                        timer_op::cancel,
                        timer_kind::pingresp_recv
                    );
                    return true;
                },
                [&](v3_1_1::disconnect_packet& p) {
                    con_.on_receive(p);
                    status_ = connection_status::disconnected;
                    return true;
                },
                [&](v5::disconnect_packet& p) {
                    con_.on_receive(p);
                    status_ = connection_status::disconnected;
                    return true;
                },
                [&](v5::auth_packet& p) {
                    con_.on_receive(p);
                    return true;
                },
                [&](std::monostate&) {
                    return false;
                }
            }
        );

        if (!result) return;
        if (pingreq_recv_timeout_ms_) {
            con_.on_timer_op(
                timer_op::cancel,
                timer_kind::pingreq_recv
            );
            if (status_ == connection_status::connecting ||
                status_ == connection_status::connected
            ) {
                con_.on_timer_op(
                    timer_op::set,
                    timer_kind::pingreq_recv,
                    *pingreq_recv_timeout_ms_
                );
            }
        }
    }
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
initialize(bool is_client) {
    publish_send_count_ = 0;
    topic_alias_send_ = std::nullopt;
    topic_alias_recv_ = std::nullopt;
    publish_recv_.clear();
    qos2_publish_processing_.clear();
    need_store_ = false;
    pid_suback_.clear();
    pid_unsuback_.clear();
    is_client_ = is_client;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<std::string>
basic_connection_impl<Role, PacketIdBytes>::
validate_topic_alias(std::optional<topic_alias_type> ta_opt) {
    if (!ta_opt) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << "topic is empty but topic_alias isn't set";
        return std::nullopt;
    }

    if (!validate_topic_alias_range(*ta_opt)) {
        return std::nullopt;
    }

    auto topic = topic_alias_send_->find(*ta_opt);
    if (topic.empty()) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << "topic is empty but topic_alias is not registered";
        return std::nullopt;
    }
    return topic;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_connection_impl<Role, PacketIdBytes>::
validate_topic_alias_range(topic_alias_type ta) {
    if (!topic_alias_send_) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << "topic_alias is set but topic_alias_maximum is 0";
        return false;
    }
    if (ta == 0 || ta > topic_alias_send_->max()) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << "topic_alias is set but out of range";
        return false;
    }
    return true;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_connection_impl<Role, PacketIdBytes>::
validate_maximum_packet_size(std::size_t size) {
    if (size > maximum_packet_size_send_) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << "packet size over maximum_packet_size for sending";
        return false;
    }
    return true;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<topic_alias_type>
basic_connection_impl<Role, PacketIdBytes>::
get_topic_alias(properties const& props) {
    std::optional<topic_alias_type> ta_opt;
    for (auto const& prop : props) {
        prop.visit(
            overload {
                [&](property::topic_alias const& p) {
                    ta_opt.emplace(p.val());
                },
                [](auto const&) {
                }
            }
        );
        if (ta_opt) return ta_opt;
    }
    return ta_opt;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
connection_status
basic_connection_impl<Role, PacketIdBytes>::
get_connection_status() const {
    return status_;
}

} // namespace detail

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_connection<Role, PacketIdBytes>::
basic_connection(protocol_version ver)
    :
    impl_{
        std::make_shared<impl_type>(
            ver,
            *this
        )
    }
{
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
recv(std::istream& is) {
    BOOST_ASSERT(impl_);
    return impl_->recv(is);
}


template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
notify_timer_fired(timer_kind kind) {
    BOOST_ASSERT(impl_);
    return impl_->notify_timer_fired(kind);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
notify_closed() {
    BOOST_ASSERT(impl_);
    return impl_->notify_closed();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
set_pingreq_send_interval(
    std::chrono::milliseconds duration
) {
    BOOST_ASSERT(impl_);
    impl_->set_pingreq_send_interval(duration);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t
basic_connection<Role, PacketIdBytes>::
get_receive_maximum_vacancy_for_send() const {
    BOOST_ASSERT(impl_);
    return impl_->get_receive_maximum_vacancy_for_send();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
set_offline_publish(
    bool val
) {
    BOOST_ASSERT(impl_);
    impl_->set_offline_publish(val);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
set_auto_pub_response(
    bool val
) {
    BOOST_ASSERT(impl_);
    impl_->set_auto_pub_response(val);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
set_auto_ping_response(
    bool val
) {
    BOOST_ASSERT(impl_);
    impl_->set_auto_ping_response(val);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
set_auto_map_topic_alias_send(
    bool val
) {
    BOOST_ASSERT(impl_);
    impl_->set_auto_map_topic_alias_send(val);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
set_auto_replace_topic_alias_send(
    bool val
) {
    BOOST_ASSERT(impl_);
    impl_->set_auto_replace_topic_alias_send(val);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
set_pingresp_recv_timeout(
    std::chrono::milliseconds duration
) {
    BOOST_ASSERT(impl_);
    impl_->set_pingresp_recv_timeout(duration);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
basic_connection<Role, PacketIdBytes>::
acquire_unique_packet_id() {
    BOOST_ASSERT(impl_);
    return impl_->acquire_unique_packet_id();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_connection<Role, PacketIdBytes>::
register_packet_id(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id
) {
    BOOST_ASSERT(impl_);
    return impl_->register_packet_id(packet_id);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
release_packet_id(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id
) {
    BOOST_ASSERT(impl_);
    return impl_->release_packet_id(packet_id);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::set<typename basic_packet_id_type<PacketIdBytes>::type>
basic_connection<Role, PacketIdBytes>::
get_qos2_publish_handled_pids() const {
    BOOST_ASSERT(impl_);
    return impl_->get_qos2_publish_handled_pids();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
restore_qos2_publish_handled_pids(
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> pids
) {
    BOOST_ASSERT(impl_);
    impl_->restore_qos2_publish_handled_pids(force_move(pids));
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
restore_packets(
    std::vector<basic_store_packet_variant<PacketIdBytes>> pvs
) {
    BOOST_ASSERT(impl_);
    impl_->restore_packets(force_move(pvs));
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_store_packet_variant<PacketIdBytes>>
basic_connection<Role, PacketIdBytes>::
get_stored_packets() const {
    BOOST_ASSERT(impl_);
    return impl_->get_stored_packets();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
protocol_version
basic_connection<Role, PacketIdBytes>::
get_protocol_version() const {
    BOOST_ASSERT(impl_);
    return impl_->get_protocol_version();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_connection<Role, PacketIdBytes>::
is_publish_processing(typename basic_packet_id_type<PacketIdBytes>::type pid) const {
    BOOST_ASSERT(impl_);
    return impl_->is_publish_processing(pid);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
error_code
basic_connection<Role, PacketIdBytes>::
regulate_for_store(
    v5::basic_publish_packet<PacketIdBytes>& packet
) const {
    BOOST_ASSERT(impl_);
    return impl_->regulate_for_store(packet);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
connection_status
basic_connection<Role, PacketIdBytes>::
get_connection_status() const {
    BOOST_ASSERT(impl_);
    return impl_->get_connection_status();
}

} // namespace async_mqtt

#include <async_mqtt/protocol/impl/connection_instantiate.hpp>

#endif // ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_IMPL_IPP
