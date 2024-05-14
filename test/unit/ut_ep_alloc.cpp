// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <thread>

#include <boost/asio.hpp>

#include <async_mqtt/endpoint.hpp>

#include "stub_socket.hpp"
#include <async_mqtt/util/packet_variant_operator.hpp>

BOOST_AUTO_TEST_SUITE(ut_ep_alloc)

namespace am = async_mqtt;
namespace as = boost::asio;

// packet_id is hard coded in this test case for just testing.
// but users need to get packet_id via ep->acquire_unique_packet_id(...)
// see other test cases.

static constexpr std::size_t check_digit_read = 12345;
static bool allocate_called_read = false;
static bool deallocate_called_read = false;

template <typename T>
struct my_alloc_read : std::allocator<T> {
    using base_type = std::allocator<T>;
    using base_type::base_type;

    template <typename U>
    struct rebind {
        using other = my_alloc_read<U>;
    };

    T* allocate(std::size_t n, void const* hint = nullptr) {
        (void)hint;
        if (n == check_digit_read) {
            allocate_called_read = true;
        }
        return base_type::allocate(n);
    }
    void deallocate(T* p, std::size_t n) {
        if (n == check_digit_read) {
            deallocate_called_read = true;
        }
        return base_type::deallocate(p, n);
    }

};

BOOST_AUTO_TEST_CASE(custom_read) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;

    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    );

    auto connect = am::v3_1_1::connect_packet{
        true,   // clean_session
        0x0, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
    };
    auto connack = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::accepted
    };
    ep->next_layer().set_recv_packets(
        {
            // receive packets
            connack
        }
    );
    ep->next_layer().set_associated_cheker_for_read(check_digit_read);
    my_alloc_read<int> ma;
    BOOST_CHECK(!allocate_called_read);
    BOOST_CHECK(!deallocate_called_read);
    ep->send(
        connect,
        [&](auto se) {
            BOOST_CHECK(!se);
            ep->recv(
                as::bind_allocator(
                    ma,
                    [&](auto pv) {
                        BOOST_TEST(pv == connack);
                    }
                )
            );
        }
    );
    ioc.run();
    BOOST_CHECK(allocate_called_read);
    BOOST_CHECK(deallocate_called_read);
}

static constexpr std::size_t check_digit_write = 23456;
static bool allocate_called_write = false;
static bool deallocate_called_write = false;

template <typename T>
struct my_alloc_write : std::allocator<T> {
    using base_type = std::allocator<T>;
    using base_type::base_type;

    template <typename U>
    struct rebind {
        using other = my_alloc_write<U>;
    };

    T* allocate(std::size_t n, void const* hint = nullptr) {
        (void)hint;
        if (n == check_digit_write) {
            allocate_called_write = true;
        }
        return base_type::allocate(n);
    }
    void deallocate(T* p, std::size_t n) {
        if (n == check_digit_write) {
            deallocate_called_write = true;
        }
        return base_type::deallocate(p, n);
    }

};

BOOST_AUTO_TEST_CASE(custom_write) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;

    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    );

    auto connect = am::v3_1_1::connect_packet{
        true,   // clean_session
        0x0, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
    };
    auto connack = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::accepted
    };
    ep->next_layer().set_recv_packets(
        {
            // receive packets
            connack
        }
    );
    ep->next_layer().set_associated_cheker_for_write(check_digit_write);
    my_alloc_write<int> ma;
    BOOST_CHECK(!allocate_called_write);
    BOOST_CHECK(!deallocate_called_write);
    ep->send(
        connect,
        as::bind_allocator(
            ma,
            [&](auto se) {
                BOOST_CHECK(!se);
                ep->recv(
                    [&](auto pv) {
                        BOOST_TEST(pv == connack);
                    }
                );
            }
        )
    );
    ioc.run();
    BOOST_CHECK(allocate_called_write);
    BOOST_CHECK(deallocate_called_write);
}

BOOST_AUTO_TEST_SUITE_END()
