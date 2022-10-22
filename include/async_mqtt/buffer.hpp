// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BUFFER_HPP)
#define ASYNC_MQTT_BUFFER_HPP

#include <string_view>

#include <boost/asio/buffer.hpp>
#include <boost/container_hash/hash.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/any.hpp>
#include <async_mqtt/util/shared_ptr_array.hpp>
#include <async_mqtt/util/is_iterator.hpp>
#include <async_mqtt/util/string_view.hpp>

namespace async_mqtt {

namespace as = boost::asio;

/**
 * @brief buffer that has string_view interface
 * This class provides string_view interface.
 * This class hold string_view target's lifetime optionally.
 */
class buffer {
public:
    using traits_type = string_view::traits_type;
    using value_type = string_view::value_type;
    using pointer = string_view::pointer;
    using const_pointer = string_view::const_pointer;
    using reference = string_view::reference;
    using const_reference = string_view::const_reference;
    using iterator = string_view::iterator;
    using const_iterator = string_view::const_iterator;
    using const_reverse_iterator = string_view::const_reverse_iterator;
    using reverse_iterator = string_view::reverse_iterator;
    using size_type = string_view::size_type;
    using difference_type = string_view::difference_type;

    static constexpr size_type npos = string_view::npos;

    constexpr buffer() noexcept = default;
    buffer(buffer const& other) noexcept = default;
    constexpr buffer(char const* s, std::size_t count)
        :view_{s, count}
    {}
    constexpr buffer(char const* s)
        :view_{s}
    {}

    template <
        typename It,
        typename End,
        typename std::enable_if_t<
            is_input_iterator<It>::value &&
            is_input_iterator<End>::value
        >* = nullptr
    >
    constexpr buffer(It first, End last)
        :view_{&*first, &*first + std::distance(first, last)}
    {}

    /**
     * @brief string_view constructor
     * @param sv string_view
     * This constructor doesn't hold the sv target's lifetime.
     * It behaves as string_view. Caller needs to manage the target lifetime.
     */
    explicit constexpr buffer(string_view sv)
        : view_{force_move(sv)}
    {}

    /**
     * @brief string constructor (deleted)
     * @param string
     * This constructor is intentionally deleted.
     * Consider `buffer(std::string("ABC"))`, the buffer points to dangling reference.
     */
    explicit buffer(std::string) = delete; // to avoid misuse

    /**
     * @brief string_view and lifetime constructor
     * @param sv string_view
     * @param sp shared_ptr_array that holds sv target's lifetime
     * If user creates buffer via this constructor, sp's lifetime is held by the buffer.
     */
    buffer(string_view sv, any life)
        : view_{force_move(sv)},
          life_{force_move(life)}
    {
    }

    buffer(char const* s, std::size_t count, any life)
        : view_{s, count},
          life_{force_move(life)}
    {}
    buffer(char const* s, any life)
        : view_{s},
          life_{force_move(life)}
    {}

    template <
        typename It,
        typename End,
        typename std::enable_if_t<
            is_input_iterator<It>::value &&
            is_input_iterator<End>::value
        >* = nullptr
    >
    buffer(It first, End last, any life)
        : view_{&*first, static_cast<std::size_t>(std::distance(first, last))},
          life_{force_move(life)}
    {}

    buffer& operator=(buffer const& buf) noexcept = default;

    constexpr const_iterator begin() const noexcept {
        return view_.begin();
    }
    constexpr const_iterator cbegin() const noexcept {
        return view_.cbegin();
    }
    constexpr const_iterator end() const noexcept {
        return view_.end();
    }
    constexpr const_iterator cend() const noexcept {
        return view_.cend();
    }
    constexpr const_reverse_iterator rbegin() const noexcept {
        return view_.rbegin();
    }
    constexpr const_reverse_iterator crbegin() const noexcept {
        return view_.crbegin();
    }
    constexpr const_reverse_iterator rend() const noexcept {
        return view_.rend();
    }
    constexpr const_reverse_iterator crend() const noexcept {
        return view_.crend();
    }

    constexpr const_reference operator[](size_type pos) const {
        return view_[pos];
    }
    constexpr const_reference at(size_type pos) const {
        return view_.at(pos);
    }

    constexpr const_reference front() const {
        return view_.front();
    }
    constexpr const_reference back() const {
        return view_.back();
    }

    constexpr const_pointer data() const noexcept {
        return view_.data();
    }

    constexpr size_type size() const noexcept {
        return view_.size();
    }
    constexpr size_type length() const noexcept {
        return view_.length();
    }
    constexpr size_type max_size() const noexcept {
        return view_.max_size();
    }

    constexpr bool empty() const noexcept {
        return view_.empty();
    }

    constexpr void remove_prefix(size_type n) {
        view_.remove_prefix(n);
    }

    constexpr void remove_suffix(size_type n) {
        view_.remove_suffix(n);
    }

    void swap(buffer& buf) noexcept {
        view_.swap(buf.view_);
        life_.swap(buf.life_);
    }

    size_type copy(char* dest, size_type count, size_type pos = 0 ) const {
        return view_.copy(dest, count, pos);
    }

    /**
     * @brief get substring
     * The returned buffer ragnge is the same as string_view::substr().
     * In addition the lifetime is shared between returned buffer and this buffer.
     * @param offset offset point of the buffer
     * @param length length of the buffer, If the length is string_view::npos
     *               then the length is from offset to the end of string.
     */
    buffer substr(size_type pos = 0, size_type count = npos) const& {
        // range is checked in string_view::substr.
        return buffer(view_.substr(pos, count), life_);
    }

    /**
     * @brief get substring
     * The returned buffer ragnge is the same as string_view::substr().
     * In addition the lifetime is moved to returned buffer.
     * @param offset offset point of the buffer
     * @param length length of the buffer, If the length is string_view::npos
     *               then the length is from offset to the end of string.
     */
    buffer substr(size_type pos = 0, size_type count = npos) && {
        // range is checked in string_view::substr.
        return buffer(view_.substr(pos, count), force_move(life_));
    }

    constexpr int compare(buffer const& buf) const noexcept {
        return view_.compare(buf.view_);
    }
    constexpr int compare(string_view const& v) const noexcept {
        return view_.compare(v);
    }
    constexpr int compare(size_type pos1, size_type count1, buffer const& buf) const noexcept {
        return view_.compare(pos1, count1, buf.view_);
    }
    constexpr int compare(size_type pos1, size_type count1, string_view const& v) const noexcept {
        return view_.compare(pos1, count1, v);
    }
    constexpr int compare(
        size_type pos1,
        size_type count1,
        buffer const& buf,
        size_type pos2,
        size_type count2
    ) const noexcept {
        return view_.compare(pos1, count1, buf.view_, pos2, count2);
    }
    constexpr int compare(
        size_type pos1,
        size_type count1,
        string_view const& v,
        size_type pos2,
        size_type count2
    ) const noexcept {
        return view_.compare(pos1, count1, v, pos2, count2);
    }
    constexpr int compare(char const* s) const noexcept {
        return view_.compare(s);
    }
    constexpr int compare(size_type pos1, size_type count1, char const* s) const noexcept {
        return view_.compare(pos1, count1, s);
    }
    constexpr int compare(
        size_type pos1,
        size_type count1,
        char const* s,
        size_type pos2,
        size_type count2
    ) const noexcept {
        return view_.compare(pos1, count1, s, pos2, count2);
    }

    constexpr size_type find(buffer const& buf, size_type pos = 0) const noexcept {
        return view_.find(buf.view_, pos);
    }
    constexpr size_type find(string_view v, size_type pos = 0) const noexcept {
        return view_.find(v, pos);
    }
    constexpr size_type find(char ch, size_type pos = 0) const noexcept {
        return view_.find(ch, pos);
    }
    constexpr size_type find(char const* s, size_type pos, size_type count) const {
        return view_.find(s, pos, count);
    }
    constexpr size_type find(char const* s, size_type pos = 0) const {
        return view_.find(s, pos);
    }

    constexpr size_type rfind(buffer const& buf, size_type pos = npos) const noexcept {
        return view_.rfind(buf.view_, pos);
    }
    constexpr size_type rfind( string_view v, size_type pos = npos) const noexcept {
        return view_.rfind(v, pos);
    }
    constexpr size_type rfind(char ch, size_type pos = npos) const noexcept {
        return view_.rfind(ch, pos);
    }
    constexpr size_type rfind(char const* s, size_type pos, size_type count) const {
        return view_.rfind(s, pos, count);
    }
    constexpr size_type rfind(char const* s, size_type pos = npos) const {
        return view_.rfind(s, pos);
    }

    constexpr size_type find_first_of(buffer const& buf, size_type pos = 0) const noexcept {
        return view_.find_first_of(buf.view_, pos);
    }
    constexpr size_type find_first_of( string_view v, size_type pos = 0) const noexcept {
        return view_.find_first_of(v, pos);
    }
    constexpr size_type find_first_of(char ch, size_type pos = 0) const noexcept {
        return view_.find_first_of(ch, pos);
    }
    constexpr size_type find_first_of(char const* s, size_type pos, size_type count) const {
        return view_.find_first_of(s, pos, count);
    }
    constexpr size_type find_first_of(char const* s, size_type pos = 0) const {
        return view_.find_first_of(s, pos);
    }

    constexpr size_type find_last_of(buffer const& buf, size_type pos = npos) const noexcept {
        return view_.find_last_of(buf.view_, pos);
    }
    constexpr size_type find_last_of( string_view v, size_type pos = npos) const noexcept {
        return view_.find_last_of(v, pos);
    }
    constexpr size_type find_last_of(char ch, size_type pos = npos) const noexcept {
        return view_.find_last_of(ch, pos);
    }
    constexpr size_type find_last_of(char const* s, size_type pos, size_type count) const {
        return view_.find_last_of(s, pos, count);
    }
    constexpr size_type find_last_of(char const* s, size_type pos = npos) const {
        return view_.find_last_of(s, pos);
    }

    constexpr size_type find_first_not_of(buffer const& buf, size_type pos = 0) const noexcept {
        return view_.find_first_not_of(buf.view_, pos);
    }
    constexpr size_type find_first_not_of( string_view v, size_type pos = 0) const noexcept {
        return view_.find_first_not_of(v, pos);
    }
    constexpr size_type find_first_not_of(char ch, size_type pos = 0) const noexcept {
        return view_.find_first_not_of(ch, pos);
    }
    constexpr size_type find_first_not_of(char const* s, size_type pos, size_type count) const {
        return view_.find_first_not_of(s, pos, count);
    }
    constexpr size_type find_first_not_of(char const* s, size_type pos = 0) const {
        return view_.find_first_not_of(s, pos);
    }

    constexpr size_type find_last_not_of(buffer const& buf, size_type pos = npos) const noexcept {
        return view_.find_last_not_of(buf.view_, pos);
    }
    constexpr size_type find_last_not_of( string_view v, size_type pos = npos) const noexcept {
        return view_.find_last_not_of(v, pos);
    }
    constexpr size_type find_last_not_of(char ch, size_type pos = npos) const noexcept {
        return view_.find_last_not_of(ch, pos);
    }
    constexpr size_type find_last_not_of(char const* s, size_type pos, size_type count) const {
        return view_.find_last_not_of(s, pos, count);
    }
    constexpr size_type find_last_not_of(char const* s, size_type pos = npos) const {
        return view_.find_last_not_of(s, pos);
    }

    any const& get_life() const {
        return life_;
    }

    operator as::const_buffer() const {
        return as::buffer(view_.data(), view_.size());
    }

    operator string_view() const {
        return view_;
    }

    friend
    constexpr bool operator==(buffer const& lhs, buffer const& rhs ) noexcept {
        return lhs.view_ == rhs.view_;
    }
    friend
    constexpr bool operator!=(buffer const& lhs, buffer const& rhs ) noexcept {
        return lhs.view_ != rhs.view_;
    }
    friend
    constexpr bool operator<(buffer const& lhs, buffer const& rhs ) noexcept {
        return lhs.view_ < rhs.view_;
    }
    friend
    constexpr bool operator<=(buffer const& lhs, buffer const& rhs ) noexcept {
        return lhs.view_ <= rhs.view_;
    }
    friend
    constexpr bool operator>(buffer const& lhs, buffer const& rhs ) noexcept {
        return lhs.view_ > rhs.view_;
    }
    friend
    constexpr bool operator>=(buffer const& lhs, buffer const& rhs ) noexcept {
        return lhs.view_ >= rhs.view_;
    }

    friend
    std::ostream& operator<<(std::ostream& o, buffer const& buf) {
        o << buf.view_;
        return o;
    }
private:
    string_view view_;
    any life_;
};

inline std::size_t hash_value(buffer const& v) noexcept {
    std::size_t result = 0;
    boost::hash_combine(result, static_cast<string_view const&>(v));
    return result;
}

inline namespace literals {

/**
 * @brief user defined literals for buffer
 * If user use this out of mqtt scope, then user need to declare
 * `using namespace literals`.
 * When user write "ABC"_mb, then this function is called.
 * The created buffer doesn't hold any lifetimes because the string literals
 * has static strage duration, so buffer doesn't need to hold the lifetime.
 *
 * @param str     the address of the string literal
 * @param length  the length of the string literal
 * @return buffer
 */
inline buffer operator""_mb(char const* str, std::size_t length) {
    return buffer(str, length);
}

} // namespace literals

/**
 * @brief create buffer from the pair of iterators
 * It copies string that from b to e into shared_ptr_array.
 * Then create buffer and return it.
 * The buffer holds the lifetime of shared_ptr_array.
 *
 * @param b  begin position iterator
 * @param e  end position iterator
 * @return buffer
 */
template <typename Iterator>
inline buffer allocate_buffer(Iterator b, Iterator e) {
    auto size = static_cast<std::size_t>(std::distance(b, e));
    if (size == 0) return buffer(&*b, size);
    auto spa = make_shared_ptr_array(size);
    std::copy(b, e, spa.get());
    auto p = spa.get();
    return buffer(p, size, force_move(spa));
}

/**
 * @brief create buffer from the string_view
 * It copies string that from string_view into shared_ptr_array.
 * Then create buffer and return it.
 * The buffer holds the lifetime of shared_ptr_array.
 *
 * @param sv  the source string_view
 * @return buffer
 */
inline buffer allocate_buffer(string_view sv) {
    return allocate_buffer(sv.begin(), sv.end());
}

inline buffer const* buffer_sequence_begin(buffer const& buf) {
    return std::addressof(buf);
}

inline buffer const* buffer_sequence_end(buffer const& buf) {
    return std::addressof(buf) + 1;
}

template <typename Col>
inline typename Col::const_iterator buffer_sequence_begin(Col const& col) {
    return col.cbegin();
}

template <typename Col>
inline typename Col::const_iterator buffer_sequence_end(Col const& col) {
    return col.cend();
}

namespace detail {

template <typename>
char buffer_sequence_begin_helper(...);

template <typename T>
char (&buffer_sequence_begin_helper(
    T* t,
    typename std::enable_if<
        !std::is_same<
            decltype(buffer_sequence_begin(*t)),
            void
        >::value
    >::type*)
)[2];

template <typename>
char buffer_sequence_end_helper(...);

template <typename T>
char (&buffer_sequence_end_helper(
    T* t,
    typename std::enable_if<
        !std::is_same<
            decltype(buffer_sequence_end(*t)),
            void
        >::value
    >::type*)
)[2];

template <typename, typename>
char (&buffer_sequence_element_type_helper(...))[2];

template <typename T, typename Buffer>
char buffer_sequence_element_type_helper(
    T* t,
    typename std::enable_if<
        std::is_convertible<
            decltype(*buffer_sequence_begin(*t)),
            Buffer
        >::value
    >::type*
);

template <typename T, typename Buffer>
struct is_buffer_sequence_class
    : std::integral_constant<bool,
      sizeof(buffer_sequence_begin_helper<T>(0, 0)) != 1 &&
      sizeof(buffer_sequence_end_helper<T>(0, 0)) != 1 &&
      sizeof(buffer_sequence_element_type_helper<T, Buffer>(0, 0)) == 1>
{
};

} // namespace detail

template <typename T>
struct is_buffer_sequence :
    std::conditional<
        std::is_class<T>::value,
        detail::is_buffer_sequence_class<T, buffer>,
        std::false_type
    >::type
{
};

template <>
struct is_buffer_sequence<buffer> : std::true_type
{
};

} // namespace async_mqtt

namespace boost {
namespace asio {

/**
 * @brief create boost::asio::const_buffer from the async_mqtt::buffer
 * boost::asio::const_buffer is a kind of view class.
 * So the class doesn't hold any lifetimes.
 * The caller needs to manage data's lifetime.
 *
 * @param  data  source async_mqtt::buffer
 * @return boost::asio::const_buffer
 */
inline const_buffer buffer(async_mqtt::buffer const& data) {
    return buffer(data.data(), data.size());
}

} // namespace asio
} // namespace boost

namespace std {

template <>
class hash<async_mqtt::buffer> {
public:
    std::uint64_t operator()(async_mqtt::buffer const& v) const noexcept {
        return std::hash<std::string_view>()(static_cast<std::string_view const&>(v));
    }
};

} // namespace std

#endif // ASYNC_MQTT_BUFFER_HPP
