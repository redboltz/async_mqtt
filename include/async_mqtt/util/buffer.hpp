// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BUFFER_HPP)
#define ASYNC_MQTT_BUFFER_HPP

#include <string_view>
#include <memory>


#include <boost/asio/buffer.hpp>
#include <boost/container_hash/hash.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/is_iterator.hpp>

/**
 * @defgroup buffer
 * @ingroup packet_detail
 */

namespace async_mqtt {

namespace as = boost::asio;

/**
 * @ingroup buffer
 * @brief buffer that has string_view interface and shared ownership
 * This class is only for advanced usecase such as developping high performance MQTT broker.
 * Typical MQTT client developpers don't need to care about the buffer.
 * This class provides string_view interface.
 * This class holds string_view pointee's lifetime optionally.
 */
class buffer {
public:
    using life_type = std::shared_ptr<void>;
    using traits_type = std::string_view::traits_type;
    using value_type = std::string_view::value_type;
    using pointer = std::string_view::pointer;
    using const_pointer = std::string_view::const_pointer;
    using reference = std::string_view::reference;
    using const_reference = std::string_view::const_reference;
    using iterator = std::string_view::iterator;
    using const_iterator = std::string_view::const_iterator;
    using const_reverse_iterator = std::string_view::const_reverse_iterator;
    using reverse_iterator = std::string_view::reverse_iterator;
    using size_type = std::string_view::size_type;
    using difference_type = std::string_view::difference_type;

    static constexpr size_type npos = std::string_view::npos;

    /**
     * @brief default constructor
     */
    constexpr buffer() noexcept = default;

    /**
     * @brief copy constructor
     */
    buffer(buffer const& other) = default;

    /**
     * @brief move constructor
     */
    buffer(buffer&& other) = default;

    /**
     * @brief copy assign operator
     */
    buffer& operator=(buffer const& other) = default;

    /**
     * @brief move assign operator
     */
    buffer& operator=(buffer&& other) = default;

    /**
     * @brief constructor
     *        The buffer doesn't manage the lifetime
     * @param s the begin pointer of continuous  memory that contains characters
     * @param count the length of the continuours memory
     */
    constexpr buffer(char const* s, std::size_t count)
        :view_{s, count}
    {}

    /**
     * @brief constructor
     *        The buffer doesn't manage the lifetime
     * @param s the begin pointer of continuous  memory that contains characters.
     *          The characters must be null terminated.
     */
    constexpr buffer(char const* s)
        :view_{s}
    {}

    /**
     * @brief move assign operator
     *        The buffer doesn't manage the lifetime.
     *        The memory between first and last must be continuous.
     * @param first the begin pointer of continuous  memory that contains characters
     * @param last  the end pointer of continuous  memory that contains characters
     */
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
     * @brief std::string_view constructor
     * @param sv std::string_view
     * This constructor doesn't hold the sv target's lifetime.
     * It behaves as std::string_view. Caller needs to manage the target lifetime.
     */
    explicit constexpr buffer(std::string_view sv)
        : view_{force_move(sv)}
    {}

    /**
     * @brief string constructor
     * @param string
     */
    explicit buffer(std::string s) {
        auto str = std::make_shared<std::string>(force_move(s));
        view_ = *str;
        life_ = force_move(str);
    }

    /**
     * @brief std::string_view and lifetime constructor
     * @param sv std::string_view
     * @param life sv target's lifetime keeping object by shared ownership
     * If user creates buffer via this constructor, sp's lifetime is held by the buffer.
     */
    buffer(std::string_view sv, std::shared_ptr<void> life)
        : view_{force_move(sv)},
          life_{force_move(life)}
    {}

    /**
     * @brief pointer, size,  and lifetime constructor
     * @param p     pointer to the beginning of the view
     * @param count size of the view
     * @param life sv target's lifetime keeping object by shared ownership
     * If user creates buffer via this constructor, sp's lifetime is held by the buffer.
     */
    buffer(char const* s, std::size_t count,std::shared_ptr<void> life)
        : view_{s, count},
          life_{force_move(life)}
    {}

    /**
     * @brief range and lifetime constructor
     * @param first  iterator to the beginning of the view
     * @param last   iterator to the end of the view
     * @param life sv target's lifetime keeping object by shared ownership
     * If user creates buffer via this constructor, sp's lifetime is held by the buffer.
     */
    template <
        typename It,
        typename End,
        typename std::enable_if_t<
            is_input_iterator<It>::value &&
            is_input_iterator<End>::value
        >* = nullptr
    >
    buffer(It first, End last, std::shared_ptr<void> life)
        : view_{&*first, static_cast<std::size_t>(std::distance(first, last))},
          life_{force_move(life)}

    {}

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
     * The returned buffer ragnge is the same as std::string_view::substr().
     * In addition the lifetime is shared between returned buffer and this buffer.
     * @param offset offset point of the buffer
     * @param length length of the buffer, If the length is std::string_view::npos
     *               then the length is from offset to the end of string.
     */
    buffer substr(size_type pos = 0, size_type count = npos) const& {
        // range is checked in std::string_view::substr.
        return buffer(view_.substr(pos, count), life_);
    }

    /**
     * @brief get substring
     * The returned buffer ragnge is the same as std::string_view::substr().
     * In addition the lifetime is moved to returned buffer.
     * @param offset offset point of the buffer
     * @param length length of the buffer, If the length is std::string_view::npos
     *               then the length is from offset to the end of string.
     */
    buffer substr(size_type pos = 0, size_type count = npos) && {
        // range is checked in std::string_view::substr.
        return buffer(view_.substr(pos, count), force_move(life_));
    }

    constexpr int compare(buffer const& buf) const noexcept {
        return view_.compare(buf.view_);
    }
    constexpr int compare(std::string_view const& v) const noexcept {
        return view_.compare(v);
    }
    constexpr int compare(size_type pos1, size_type count1, buffer const& buf) const noexcept {
        return view_.compare(pos1, count1, buf.view_);
    }
    constexpr int compare(size_type pos1, size_type count1, std::string_view const& v) const noexcept {
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
        std::string_view const& v,
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
    constexpr size_type find(std::string_view v, size_type pos = 0) const noexcept {
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
    constexpr size_type rfind( std::string_view v, size_type pos = npos) const noexcept {
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
    constexpr size_type find_first_of( std::string_view v, size_type pos = 0) const noexcept {
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
    constexpr size_type find_last_of( std::string_view v, size_type pos = npos) const noexcept {
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
    constexpr size_type find_first_not_of( std::string_view v, size_type pos = 0) const noexcept {
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
    constexpr size_type find_last_not_of( std::string_view v, size_type pos = npos) const noexcept {
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

    life_type const& get_life() const {
        return life_;
    }

    operator as::const_buffer() const {
        return as::buffer(view_.data(), view_.size());
    }

    operator std::string_view() const {
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

    bool has_life() const noexcept {
        return bool(life_);
    }

private:
    std::string_view view_;
    life_type life_;
};

inline std::size_t hash_value(buffer const& v) noexcept {
    std::size_t result = 0;
    boost::hash_combine(result, static_cast<std::string_view const&>(v));
    return result;
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
 * @ingroup buffer
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
