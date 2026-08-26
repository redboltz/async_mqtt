// Copyright Takatoshi Kondo 2026
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Mutation test for all packet parsers.
//
// Every packet type is parsed from truncated, extended, length-lying and
// byte-mutated inputs. Each input is placed in an exactly-sized heap
// allocation (see exact_buffer.hpp) so that any read past the end of the
// buffer is caught by AddressSanitizer / UndefinedBehaviorSanitizer builds.
// This test has no expectation on ec: a parser may accept or reject a mutated
// packet, but it must never read out of bounds. See GHSA-fwh8-wcr5-q65c.

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"
#include "../common/exact_buffer.hpp"

#include <cstdint>
#include <initializer_list>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include <boost/lexical_cast.hpp>

#include <async_mqtt/protocol/packet/packet_variant.hpp>
#include <async_mqtt/protocol/impl/buffer_to_packet_variant.ipp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;

namespace {

using bytes = std::vector<unsigned char>;

constexpr unsigned char u8(int v) { return static_cast<unsigned char>(v); }

template <std::size_t PacketIdBytes>
void parse_one(bytes const& b, am::protocol_version ver) {
    auto buf = make_exact_buffer(
        std::string_view{reinterpret_cast<char const*>(b.data()), b.size()}
    );
    am::error_code ec;
    auto pv_opt = am::buffer_to_basic_packet_variant<PacketIdBytes>(am::force_move(buf), ver, ec);
    if (!ec) {
        BOOST_TEST(pv_opt.has_value());
        // exercise accessors and re-serialization of an accepted packet
        auto cbs = pv_opt->const_buffer_sequence();
        BOOST_TEST(!cbs.empty());
        auto str = boost::lexical_cast<std::string>(*pv_opt);
        BOOST_TEST(!str.empty());
    }
}

void parse_all(bytes const& b) {
    parse_one<2>(b, am::protocol_version::v3_1_1);
    parse_one<2>(b, am::protocol_version::v5);
    parse_one<4>(b, am::protocol_version::v3_1_1);
    parse_one<4>(b, am::protocol_version::v5);
}

bytes varint(std::size_t v) {
    bytes out;
    do {
        auto d = static_cast<unsigned char>(v & 0x7f);
        v >>= 7;
        if (v) d |= 0x80;
        out.push_back(d);
    } while (v);
    return out;
}

// fixed header byte + body -> full packet with correct remaining length
bytes mk(unsigned char fh, bytes const& body) {
    bytes out{fh};
    auto vl = varint(body.size());
    out.insert(out.end(), vl.begin(), vl.end());
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

// MQTT string (2 byte length prefix)
bytes str(std::string_view s) {
    bytes out{
        static_cast<unsigned char>(s.size() >> 8),
        static_cast<unsigned char>(s.size() & 0xff)
    };
    out.insert(out.end(), s.begin(), s.end());
    return out;
}

bytes cat(std::initializer_list<bytes> parts) {
    bytes out;
    for (auto const& p : parts) out.insert(out.end(), p.begin(), p.end());
    return out;
}

// property block with variable length prefix
bytes props(bytes const& p) {
    return cat({varint(p.size()), p});
}

std::vector<bytes> corpus() {
    std::vector<bytes> c;

    // ---- v3.1.1
    // CONNECT with will, user name, password
    c.push_back(mk(0x10, cat({str("MQTT"), {4, 0xf6, 0x00, 0x0a}, str("cid"), str("will/t"), str("wm"), str("user"), str("pw")})));
    c.push_back(mk(0x10, cat({str("MQTT"), {4, 0x02, 0x00, 0x0a}, str("")})));
    c.push_back(mk(0x20, {0x01, 0x00}));                                        // CONNACK
    c.push_back(mk(0x3d, cat({str("a/b"), {0x12, 0x34}, {'p', 'a', 'y'}})));   // PUBLISH qos2 dup retain
    c.push_back(mk(0x30, cat({str("a/b"), {'p'}})));                            // PUBLISH qos0
    c.push_back(mk(0x40, {0x12, 0x34}));                                        // PUBACK
    c.push_back(mk(0x50, {0x12, 0x34}));                                        // PUBREC
    c.push_back(mk(0x62, {0x12, 0x34}));                                        // PUBREL
    c.push_back(mk(0x70, {0x12, 0x34}));                                        // PUBCOMP
    c.push_back(mk(0x82, cat({{0x12, 0x34}, str("a/#"), {0x01}, str("+/b"), {0x02}}))); // SUBSCRIBE
    c.push_back(mk(0x90, {0x12, 0x34, 0x00, 0x02, 0x80}));                      // SUBACK
    c.push_back(mk(0xa2, cat({{0x12, 0x34}, str("a/#"), str("b")})));           // UNSUBSCRIBE
    c.push_back(mk(0xb0, {0x12, 0x34}));                                        // UNSUBACK
    c.push_back(mk(0xc0, {}));                                                  // PINGREQ
    c.push_back(mk(0xd0, {}));                                                  // PINGRESP
    c.push_back(mk(0xe0, {}));                                                  // DISCONNECT

    // ---- v5
    // property ids: 0x01 payload format indicator, 0x03 content type,
    // 0x08 response topic, 0x09 correlation data, 0x0b subscription identifier,
    // 0x11 session expiry interval, 0x12 assigned client identifier,
    // 0x15 authentication method, 0x16 authentication data, 0x18 will delay
    // interval, 0x1c server reference, 0x1f reason string, 0x21 receive
    // maximum, 0x23 topic alias, 0x26 user property
    bytes props_conn = cat({{0x11, 0, 0, 0, 10}, {0x21, 0, 5}, {0x26}, str("k"), str("v"), {0x15}, str("m"), {0x16}, str("auth")});
    bytes will_props = cat({{0x18, 0, 0, 0, 1}, {0x01, 1}, {0x09}, str("cd")});
    c.push_back(mk(0x10, cat({str("MQTT"), {5, 0xf6, 0x00, 0x0a}, props(props_conn), str("cid"), props(will_props), str("will/t"), str("wm"), str("user"), str("pw")})));
    c.push_back(mk(0x10, cat({str("MQTT"), {5, 0x02, 0x00, 0x0a}, {0x00}, str("")})));
    bytes props_ack = cat({{0x11, 0, 0, 0, 10}, {0x1f}, str("rs"), {0x26}, str("k"), str("v"), {0x12}, str("assigned")});
    c.push_back(mk(0x20, cat({{0x01, 0x00}, props(props_ack)})));              // CONNACK
    c.push_back(mk(0x20, {0x00, 0x87, 0x00}));
    bytes props_pub = cat({{0x01, 1}, {0x23, 0, 5}, {0x0b, 0x81, 0x01}, {0x09}, str("cd"), {0x08}, str("resp"), {0x03}, str("ct")});
    c.push_back(mk(0x3d, cat({str("a/b"), {0x12, 0x34}, props(props_pub), {'p', 'a', 'y'}})));
    c.push_back(mk(0x30, cat({str("a/b"), {0x00}, {'p'}})));
    c.push_back(mk(0x30, cat({str(""), {0x03, 0x23, 0, 1}, {'p'}})));           // topic alias only
    bytes props_rs = cat({{0x1f}, str("rs"), {0x26}, str("k"), str("v")});
    for (unsigned char fh : {u8(0x40), u8(0x50), u8(0x62), u8(0x70)}) { // PUBACK..PUBCOMP
        c.push_back(mk(fh, {0x12, 0x34}));
        c.push_back(mk(fh, {0x12, 0x34, 0x00}));
        c.push_back(mk(fh, {0x12, 0x34, 0x00, 0x00}));
        c.push_back(mk(fh, cat({{0x12, 0x34, 0x80}, props(props_rs)})));
    }
    bytes props_sub = cat({{0x0b, 0x81, 0x01}, {0x26}, str("k"), str("v")});
    c.push_back(mk(0x82, cat({{0x12, 0x34}, props(props_sub), str("a/#"), {0x2d}, str("$share/sn/+/b"), {0x02}})));
    c.push_back(mk(0x82, cat({{0x12, 0x34}, {0x00}, str("a"), {0x00}})));
    c.push_back(mk(0x90, cat({{0x12, 0x34}, props(props_rs), {0x00, 0x02, 0x80}})));
    c.push_back(mk(0x90, {0x12, 0x34, 0x00, 0x00}));
    c.push_back(mk(0xa2, cat({{0x12, 0x34}, props(props_rs), str("a/#"), str("b")})));
    c.push_back(mk(0xa2, cat({{0x12, 0x34}, {0x00}, str("a")})));
    c.push_back(mk(0xb0, cat({{0x12, 0x34}, props(props_rs), {0x00, 0x11}})));
    c.push_back(mk(0xb0, {0x12, 0x34, 0x00, 0x00}));
    c.push_back(mk(0xc0, {}));
    c.push_back(mk(0xd0, {}));
    c.push_back(mk(0xe0, {}));
    c.push_back(mk(0xe0, {0x00}));
    bytes props_disc = cat({{0x11, 0, 0, 0, 10}, {0x1f}, str("rs"), {0x1c}, str("srv")});
    c.push_back(mk(0xe0, cat({{0x8e}, props(props_disc)})));
    c.push_back(mk(0xf0, {}));                                                  // AUTH
    c.push_back(mk(0xf0, {0x18, 0x00}));
    bytes props_auth = cat({{0x15}, str("m"), {0x16}, str("data"), {0x1f}, str("rs")});
    c.push_back(mk(0xf0, cat({{0x18}, props(props_auth)})));
    return c;
}

// first `keep` bytes of p, with the remaining length rewritten to match the
// body length that is actually present
bytes truncate_truthful(bytes const& p, std::size_t keep) {
    if (keep == 0) return {};
    std::size_t rl_len = 1;
    while (rl_len < 4 && rl_len < p.size() - 1 && (p[rl_len] & 0x80)) ++rl_len;
    std::size_t hdr = 1 + rl_len;
    if (keep < hdr) return bytes(p.begin(), p.begin() + static_cast<std::ptrdiff_t>(keep));
    bytes body(p.begin() + static_cast<std::ptrdiff_t>(hdr), p.begin() + static_cast<std::ptrdiff_t>(keep));
    return mk(p[0], body);
}

// p with its remaining length rewritten to the actual body length
// (assumes a single byte remaining length at p[1])
bytes retruthful(bytes const& p) {
    if (p.size() < 2) return p;
    return mk(p[0], bytes(p.begin() + 2, p.end()));
}

} // namespace

BOOST_AUTO_TEST_CASE(mutation_truncate) {
    for (auto const& p : corpus()) {
        for (std::size_t k = 0; k <= p.size(); ++k) {
            // truthful remaining length
            parse_all(truncate_truthful(p, k));
            // lying remaining length (original, i.e. longer than present)
            bytes t(p.begin(), p.begin() + static_cast<std::ptrdiff_t>(k));
            parse_all(t);
            if (k >= 2) {
                for (unsigned char rl : {u8(0x00), u8(0x7f), u8(0xff)}) {
                    bytes t2 = t;
                    t2[1] = rl;
                    parse_all(t2);
                }
                // maximum 4 byte remaining length
                bytes t3{t[0], 0xff, 0xff, 0xff, 0x7f};
                t3.insert(t3.end(), t.begin() + 2, t.end());
                parse_all(t3);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(mutation_extend) {
    for (auto const& p : corpus()) {
        for (std::size_t extra = 1; extra <= 3; ++extra) {
            bytes e = p;
            e.insert(e.end(), extra, 0x00);
            parse_all(e);             // remaining length shorter than present
            parse_all(retruthful(e)); // remaining length matches
        }
    }
}

BOOST_AUTO_TEST_CASE(mutation_byte) {
    for (auto const& p : corpus()) {
        for (std::size_t i = 0; i < p.size(); ++i) {
            for (unsigned char v : {u8(0x00), u8(0x01), u8(0x7f), u8(0x80), u8(0xff)}) {
                bytes m = p;
                m[i] = v;
                parse_all(m);
                // internal length fields mutated, remaining length re-synced
                if (i >= 2) parse_all(retruthful(m));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(mutation_random) {
    auto c = corpus();
    std::mt19937 rng(0x5eed); // fixed seed: deterministic
    for (int it = 0; it < 100000; ++it) {
        bytes m = c[rng() % c.size()];
        auto n = 1 + rng() % 4;
        for (std::size_t j = 0; j < n; ++j) {
            switch (rng() % 4) {
            case 0:
                if (!m.empty()) m[rng() % m.size()] = static_cast<unsigned char>(rng());
                break;
            case 1:
                if (!m.empty()) m.resize(rng() % m.size());
                break;
            case 2:
                m.push_back(static_cast<unsigned char>(rng()));
                break;
            case 3:
                if (!m.empty()) m.erase(m.begin() + static_cast<std::ptrdiff_t>(rng() % m.size()));
                break;
            }
        }
        parse_all(m);
        parse_all(retruthful(m));
    }
}

BOOST_AUTO_TEST_SUITE_END()
