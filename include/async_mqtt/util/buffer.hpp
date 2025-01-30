// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_BUFFER_HPP)
#define ASYNC_MQTT_UTIL_BUFFER_HPP

#include <string_view>
#include <memory>


#include <boost/asio/buffer.hpp>
#include <boost/container_hash/hash.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/detail/is_iterator.hpp>

/**
 * @defgroup buffer Reference counting immutable buffer
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
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
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
            detail::is_input_iterator<It>::value &&
            detail::is_input_iterator<End>::value
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
     * @param s string
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
    explicit buffer(std::string_view sv, std::shared_ptr<void> life)
        : view_{force_move(sv)},
          life_{force_move(life)}
    {}

    /**
     * @brief pointer, size,  and lifetime constructor
     * @param s     pointer to the beginning of the view
     * @param count size of the view
     * @param life sv target's lifetime keeping object by shared ownership
     * If user creates buffer via this constructor, sp's lifetime is held by the buffer.
     */
    explicit buffer(char const* s, std::size_t count,std::shared_ptr<void> life)
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
            detail::is_input_iterator<It>::value &&
            detail::is_input_iterator<End>::value
        >* = nullptr
    >
    explicit buffer(It first, End last, std::shared_ptr<void> life)
        : view_{&*first, static_cast<std::size_t>(std::distance(first, last))},
          life_{force_move(life)}

    {}

    /**
     * @brief get an iterator to the beginning
     * @return iterator
     */
    constexpr const_iterator begin() const noexcept {
        return view_.begin();
    }

    /**
     * @brief get an iterator to the beginning
     * @return iterator
     */
    constexpr const_iterator cbegin() const noexcept {
        return view_.cbegin();
    }

    /**
     * @brief get an iterator to the end
     * @return iterator
     */
    constexpr const_iterator end() const noexcept {
        return view_.end();
    }

    /**
     * @brief get an iterator to the end
     * @return iterator
     */
    constexpr const_iterator cend() const noexcept {
        return view_.cend();
    }

    /**
     * @brief get a reverse terator to the beginning
     * @return iterator
     */
    constexpr const_reverse_iterator rbegin() const noexcept {
        return view_.rbegin();
    }

    /**
     * @brief get a reverse terator to the beginning
     * @return iterator
     */
    constexpr const_reverse_iterator crbegin() const noexcept {
        return view_.crbegin();
    }

    /**
     * @brief get a reverse terator to the end
     * @return iterator
     */
    constexpr const_reverse_iterator rend() const noexcept {
        return view_.rend();
    }

    /**
     * @brief get a reverse terator to the end
     * @return iterator
     */
    constexpr const_reverse_iterator crend() const noexcept {
        return view_.crend();
    }

    /**
     * @brief get a reference of the specific character
     * @param pos the position of the character
     * @return reference of the character
     */
    constexpr const_reference operator[](size_type pos) const {
        return view_[pos];
    }

    /**
     * @brief get a reference of the specific character with bounds checking
     * @param pos the position of the character
     * @return reference of the character
     */
    constexpr const_reference at(size_type pos) const {
        return view_.at(pos);
    }

    /**
     * @brief get a reference of the first character
     * @return reference of the character
     */
    constexpr const_reference front() const {
        return view_.front();
    }

    /**
     * @brief get a reference of the last character
     * @return reference of the character
     */
    constexpr const_reference back() const {
        return view_.back();
    }

    /**
     * @brief get a pointer of the first character of the buffer
     * @return pointer
     */
    constexpr const_pointer data() const noexcept {
        return view_.data();
    }

    /**
     * @brief get the number of characters
     * @return the number of characters
     */
    constexpr size_type size() const noexcept {
        return view_.size();
    }

    /**
     * @brief get the number of characters
     * @return the number of characters
     */
    constexpr size_type length() const noexcept {
        return view_.length();
    }

    /**
     * @brief get the largeset possible number of the buffer
     * @return the number of characters
     */
    constexpr size_type max_size() const noexcept {
        return view_.max_size();
    }

    /**
     * @brief checking the buffer is empty
     * @return true if the buffer is empty, otherwise false
     */
    constexpr bool empty() const noexcept {
        return view_.empty();
    }

    /**
     * @brief remove the first n characters from the buffer
     * @param n remove length
     */
    constexpr void remove_prefix(size_type n) {
        view_.remove_prefix(n);
    }

    /**
     * @brief remove the last n characters from the buffer
     * @param n remove length
     */
    constexpr void remove_suffix(size_type n) {
        view_.remove_suffix(n);
    }

    /**
     * @brief swap the buffer
     * @param buf target
     */
    void swap(buffer& buf) noexcept {
        view_.swap(buf.view_);
        life_.swap(buf.life_);
    }

    /**
     * @brief copy  the buffer characters
     * @param dest  destination
     * @param count copy length
     * @param pos   copy start position
     */
    size_type copy(char* dest, size_type count, size_type pos = 0 ) const {
        return view_.copy(dest, count, pos);
    }

    /**
     * @brief get substring
     * The returned buffer ragnge is the same as std::string_view::substr().
     * In addition the lifetime is shared between returned buffer and this buffer.
     * @param pos    position of the first character
     * @param count  requested length
     * @return buffer
     */
    buffer substr(size_type pos = 0, size_type count = npos) const& {
        // range is checked in std::string_view::substr.
        return buffer(view_.substr(pos, count), life_);
    }

    /**
     * @brief get substring
     * The returned buffer ragnge is the same as std::string_view::substr().
     * In addition the lifetime is moved to returned buffer.
     * @param pos    position of the first character
     * @param count  requested length
     * @return buffer
     */
    buffer substr(size_type pos = 0, size_type count = npos) && {
        // range is checked in std::string_view::substr.
        return buffer(view_.substr(pos, count), force_move(life_));
    }

    /**
     * @brief compare buffer
     * Compare only the view.
     * @param buf target
     * @return negative value if if this view is less that the buf's view.
     *         zero the both view has the same sequences of the characters.
     *         posiative value if if this view is greater that the buf's view.
     */
    constexpr int compare(buffer const& buf) const noexcept {
        return view_.compare(buf.view_);
    }

    /**
     * @brief compare buffer
     * Compare only the view.
     * @param v target
     * @return negative value if if this view is less that the v's view.
     *         zero the both view has the same sequences of the characters.
     *         posiative value if if this view is greater that the v's view.
     */
    constexpr int compare(std::string_view const& v) const noexcept {
        return view_.compare(v);
    }

    /**
     * @brief compare buffer
     * Compare only the view.
     * @param pos1   start position of the this view
     * @param count1 length of the this view
     * @param buf     target
     * @return negative value if if this view is less that the buf's view.
     *         zero the both view has the same sequences of the characters.
     *         posiative value if if this view is greater that the buf's view.
     */
    constexpr int compare(size_type pos1, size_type count1, buffer const& buf) const noexcept {
        return view_.compare(pos1, count1, buf.view_);
    }

    /**
     * @brief compare buffer
     * Compare only the view.
     * @param pos1   start position of the this view
     * @param count1 length of the this view
     * @param v      target
     * @return negative value if if this view is less that the v's view.
     *         zero the both view has the same sequences of the characters.
     *         posiative value if if this view is greater that the v's view.
     */
    constexpr int compare(size_type pos1, size_type count1, std::string_view const& v) const noexcept {
        return view_.compare(pos1, count1, v);
    }

    /**
     * @brief compare buffer
     * Compare only the view.
     * @param pos1   start position of the this view
     * @param count1 length of the this view
     * @param buf     target
     * @param pos2   start position of the buf's view
     * @param count2 length of the buf's view
     * @return negative value if if this view is less that the buf's view.
     *         zero the both view has the same sequences of the characters.
     *         posiative value if if this view is greater that the buf's view.
     */
    constexpr int compare(
        size_type pos1,
        size_type count1,
        buffer const& buf,
        size_type pos2,
        size_type count2
    ) const noexcept {
        return view_.compare(pos1, count1, buf.view_, pos2, count2);
    }

    /**
     * @brief compare buffer
     * Compare only the view.
     * @param pos1   start position of the this view
     * @param count1 length of the this view
     * @param v      target
     * @param pos2   start position of the buf's view
     * @param count2 length of the buf's view
     * @return negative value if if this view is less that the v's view.
     *         zero the both view has the same sequences of the characters.
     *         posiative value if if this view is greater that the v's view.
     */
    constexpr int compare(
        size_type pos1,
        size_type count1,
        std::string_view const& v,
        size_type pos2,
        size_type count2
    ) const noexcept {
        return view_.compare(pos1, count1, v, pos2, count2);
    }

    /**
     * @brief compare buffer
     * Compare only the view.
     * @param s      target
     * @return negative value if if this view is less that the buf's view.
     *         zero the both view has the same sequences of the characters.
     *         posiative value if if this view is greater that the buf's view.
     */
    constexpr int compare(char const* s) const noexcept {
        return view_.compare(s);
    }

    /**
     * @brief compare buffer
     * Compare only the view.
     * @param pos1   start position of the this view
     * @param count1 length of the this view
     * @param s      target
     * @return negative value if if this view is less that the buf's view.
     *         zero the both view has the same sequences of the characters.
     *         posiative value if if this view is greater that the buf's view.
     */
    constexpr int compare(size_type pos1, size_type count1, char const* s) const noexcept {
        return view_.compare(pos1, count1, s);
    }

    /**
     * @brief compare buffer
     * Compare only the view.
     * @param pos1   start position of the this view
     * @param count1 length of the this view
     * @param s      target
     * @param pos2   start position of the s
     * @param count2 length of the s
     * @return negative value if if this view is less that the buf's view.
     *         zero the both view has the same sequences of the characters.
     *         posiative value if if this view is greater that the buf's view.
     */
    constexpr int compare(
        size_type pos1,
        size_type count1,
        char const* s,
        size_type pos2,
        size_type count2
    ) const noexcept {
        return view_.compare(pos1, count1, s, pos2, count2);
    }

    /**
     * @brief find the first substring equal to the given view
     * @param buf  view to search for
     * @param pos  position at which to start the search
     * @return position of the first character of the found substring, or npos if no such substring is found.
     */
    constexpr size_type find(buffer const& buf, size_type pos = 0) const noexcept {
        return view_.find(buf.view_, pos);
    }

    /**
     * @brief find the first substring equal to the given view
     * @param v    view to search for
     * @param pos  position at which to start the search
     * @return position of the first character of the found substring, or npos if no such substring is found.
     */
    constexpr size_type find(std::string_view v, size_type pos = 0) const noexcept {
        return view_.find(v, pos);
    }

    /**
     * @brief find the first substring equal to the given view
     * @param ch   character to search for
     * @param pos  position at which to start the search
     * @return position of the first character of the found substring, or npos if no such substring is found.
     */
    constexpr size_type find(char ch, size_type pos = 0) const noexcept {
        return view_.find(ch, pos);
    }

    /**
     * @brief find the first substring equal to the given view
     * @param s    pointer to a string of characters to search for
     * @param pos   position at which to start the search
     * @param count length of substring to search for
     * @return position of the first character of the found substring, or npos if no such substring is found.
     */
    constexpr size_type find(char const* s, size_type pos, size_type count) const {
        return view_.find(s, pos, count);
    }

    /**
     * @brief find the first substring equal to the given view
     * @param s    pointer to a string of characters to search for
     * @param pos  position at which to start the search
     * @return position of the first character of the found substring, or npos if no such substring is found.
     */
    constexpr size_type find(char const* s, size_type pos = 0) const {
        return view_.find(s, pos);
    }

    /**
     * @brief find the last substring equal to the given view
     * @param buf  view to search for
     * @param pos  position at which to start the search
     * @return position of the first character of the found substring or npos if no such substring is found.
     */
    constexpr size_type rfind(buffer const& buf, size_type pos = npos) const noexcept {
        return view_.rfind(buf.view_, pos);
    }

    /**
     * @brief find the last substring equal to the given view
     * @param v    view to search for
     * @param pos  position at which to start the search
     * @return position of the first character of the found substring or npos if no such substring is found.
     */
    constexpr size_type rfind( std::string_view v, size_type pos = npos) const noexcept {
        return view_.rfind(v, pos);
    }

    /**
     * @brief find the last substring equal to the given view
     * @param ch   character to search for
     * @param pos  position at which to start the search
     * @return position of the first character of the found substring or npos if no such substring is found.
     */
    constexpr size_type rfind(char ch, size_type pos = npos) const noexcept {
        return view_.rfind(ch, pos);
    }

    /**
     * @brief find the last substring equal to the given view
     * @param s     pointer to a string of characters to search for
     * @param pos   position at which to start the search
     * @param count length of substring to search for
     * @return position of the first character of the found substring or npos if no such substring is found.
     */
    constexpr size_type rfind(char const* s, size_type pos, size_type count) const {
        return view_.rfind(s, pos, count);
    }

    /**
     * @brief find the last substring equal to the given view
     * @param s     pointer to a string of characters to search for
     * @param pos   position at which to start the search
     * @return position of the first character of the found substring or npos if no such substring is found.
     */
    constexpr size_type rfind(char const* s, size_type pos = npos) const {
        return view_.rfind(s, pos);
    }

    /**
     * @brief find the first character equal to any of the characters in the given view
     * @param buf  view to search for
     * @param pos  position at which to start the search
     * @return position of the first occurrence of any character of the substring, or npos if no such character is found.
     */
    constexpr size_type find_first_of(buffer const& buf, size_type pos = 0) const noexcept {
        return view_.find_first_of(buf.view_, pos);
    }

    /**
     * @brief find the first character equal to any of the characters in the given view
     * @param v    view to search for
     * @param pos  position at which to start the search
     * @return position of the first occurrence of any character of the substring, or npos if no such character is found.
     */
    constexpr size_type find_first_of( std::string_view v, size_type pos = 0) const noexcept {
        return view_.find_first_of(v, pos);
    }

    /**
     * @brief find the first character equal to any of the characters in the given view
     * @param ch   character to search for
     * @param pos  position at which to start the search
     * @return position of the first occurrence of any character of the substring, or npos if no such character is found.
     */
    constexpr size_type find_first_of(char ch, size_type pos = 0) const noexcept {
        return view_.find_first_of(ch, pos);
    }

    /**
     * @brief find the first character equal to any of the characters in the given view
     * @param s    pointer to a string of characters to search for
     * @param pos   position at which to start the search
     * @param count length of the string of characters to search for
     * @return position of the first occurrence of any character of the substring, or npos if no such character is found.
     */
    constexpr size_type find_first_of(char const* s, size_type pos, size_type count) const {
        return view_.find_first_of(s, pos, count);
    }

    /**
     * @brief find the first character equal to any of the characters in the given view
     * @param s    pointer to a string of characters to search for
     * @param pos  position at which to start the search
     * @return position of the first occurrence of any character of the substring, or npos if no such character is found.
     */
    constexpr size_type find_first_of(char const* s, size_type pos = 0) const {
        return view_.find_first_of(s, pos);
    }

    /**
     * @brief find the last character equal to any of the characters in the given view
     * @param buf  view to search for
     * @param pos  position at which to start the search
     * @return position of the last occurrence of any character of the substring, or npos if no such character is found.
     */
    constexpr size_type find_last_of(buffer const& buf, size_type pos = npos) const noexcept {
        return view_.find_last_of(buf.view_, pos);
    }

    /**
     * @brief find the last character equal to any of the characters in the given view
     * @param v    view to search for
     * @param pos  position at which to start the search
     * @return position of the last occurrence of any character of the substring, or npos if no such character is found.
     */
    constexpr size_type find_last_of( std::string_view v, size_type pos = npos) const noexcept {
        return view_.find_last_of(v, pos);
    }

    /**
     * @brief find the last character equal to any of the characters in the given view
     * @param ch   character to search for
     * @param pos  position at which to start the search
     * @return position of the last occurrence of any character of the substring, or npos if no such character is found.
     */
    constexpr size_type find_last_of(char ch, size_type pos = npos) const noexcept {
        return view_.find_last_of(ch, pos);
    }

    /**
     * @brief find the last character equal to any of the characters in the given view
     * @param s    pointer to a string of characters to search for
     * @param pos   position at which to start the search
     * @param count length of the string of characters to search for
     * @return position of the last occurrence of any character of the substring, or npos if no such character is found.
     */
    constexpr size_type find_last_of(char const* s, size_type pos, size_type count) const {
        return view_.find_last_of(s, pos, count);
    }
    /**
     * @brief find the last character equal to any of the characters in the given view
     * @param s    pointer to a string of characters to search for
     * @param pos  position at which to start the search
     * @return position of the last occurrence of any character of the substring, or npos if no such character is found.
     */
    constexpr size_type find_last_of(char const* s, size_type pos = npos) const {
        return view_.find_last_of(s, pos);
    }

    /**
     * @brief find the first character not equal to any of the characters in the given view
     * @param buf  view to search for
     * @param pos  position at which to start the search
     * @return position of the first character not equal to any of the characters in the given string,
     *         or npos if no such character is found.
     */
    constexpr size_type find_first_not_of(buffer const& buf, size_type pos = 0) const noexcept {
        return view_.find_first_not_of(buf.view_, pos);
    }

    /**
     * @brief find the first character not equal to any of the characters in the given view
     * @param v    view to search for
     * @param pos  position at which to start the search
     * @return position of the first character not equal to any of the characters in the given string,
     *         or npos if no such character is found.
     */
    constexpr size_type find_first_not_of( std::string_view v, size_type pos = 0) const noexcept {
        return view_.find_first_not_of(v, pos);
    }

    /**
     * @brief find the first character not equal to any of the characters in the given view
     * @param ch   character to search for
     * @param pos  position at which to start the search
     * @return position of the first character not equal to any of the characters in the given string,
     *         or npos if no such character is found.
     */
    constexpr size_type find_first_not_of(char ch, size_type pos = 0) const noexcept {
        return view_.find_first_not_of(ch, pos);
    }

    /**
     * @brief find the first character not equal to any of the characters in the given view
     * @param s     view to search for
     * @param pos   position at which to start the search
     * @param count length of the string of characters to compare
     * @return position of the first character not equal to any of the characters in the given string,
     *         or npos if no such character is found.
     */
    constexpr size_type find_first_not_of(char const* s, size_type pos, size_type count) const {
        return view_.find_first_not_of(s, pos, count);
    }

    /**
     * @brief find the first character not equal to any of the characters in the given view
     * @param s    pointer to a string of characters to compare
     * @param pos  position at which to start the search
     * @return position of the first character not equal to any of the characters in the given string,
     *         or npos if no such character is found.
     */
    constexpr size_type find_first_not_of(char const* s, size_type pos = 0) const {
        return view_.find_first_not_of(s, pos);
    }

    /**
     * @brief find the last character not equal to any of the characters in the given view
     * @param buf  view to search for
     * @param pos  position at which to start the search
     * @return position of the last character not equal to any of the characters in the given string,
     *         or npos if no such character is found.
     */
    constexpr size_type find_last_not_of(buffer const& buf, size_type pos = npos) const noexcept {
        return view_.find_last_not_of(buf.view_, pos);
    }

    /**
     * @brief find the last character not equal to any of the characters in the given view
     * @param v    view to search for
     * @param pos  position at which to start the search
     * @return position of the last character not equal to any of the characters in the given string,
     *         or npos if no such character is found.
     */
    constexpr size_type find_last_not_of( std::string_view v, size_type pos = npos) const noexcept {
        return view_.find_last_not_of(v, pos);
    }

    /**
     * @brief find the last character not equal to any of the characters in the given view
     * @param ch   character to search for
     * @param pos  position at which to start the search
     * @return position of the last character not equal to any of the characters in the given string,
     *         or npos if no such character is found.
     */
    constexpr size_type find_last_not_of(char ch, size_type pos = npos) const noexcept {
        return view_.find_last_not_of(ch, pos);
    }

    /**
     * @brief find the last character not equal to any of the characters in the given view
     * @param s    pointer to a string of characters to compare
     * @param pos   position at which to start the search
     * @param count length of the string of characters to compare
     * @return position of the last character not equal to any of the characters in the given string,
     *         or npos if no such character is found.
     */
    constexpr size_type find_last_not_of(char const* s, size_type pos, size_type count) const {
        return view_.find_last_not_of(s, pos, count);
    }

    /**
     * @brief find the last character not equal to any of the characters in the given view
     * @param s    pointer to a string of characters to compare
     * @param pos  position at which to start the search
     * @return position of the last character not equal to any of the characters in the given string,
     *         or npos if no such character is found.
     */
    constexpr size_type find_last_not_of(char const* s, size_type pos = npos) const {
        return view_.find_last_not_of(s, pos);
    }

    /**
     * @brief conversion operator to the buffer as asio const_buffer
     * @return asio boost::asio::const_buffer
     */
    operator as::const_buffer() const {
        return as::buffer(view_.data(), view_.size());
    }

    /**
     * @brief conversion operator to the buffer as asio const_buffer
     * @return asio std::string_view
     */
    operator std::string_view() const {
        return view_;
    }

    /**
     * @brief equal operator
     *        comparison target is the view of the buffer. life holder is not compared.
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs equal to the rhs, otherwise false.
     */
    friend
    constexpr bool operator==(buffer const& lhs, buffer const& rhs) noexcept {
        return lhs.view_ == rhs.view_;
    }

    /**
     * @brief not equal operator
     *        comparison target is the view of the buffer. life holder is not compared.
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs not equal to the rhs, otherwise false.
     */
    friend
    constexpr bool operator!=(buffer const& lhs, buffer const& rhs) noexcept {
        return lhs.view_ != rhs.view_;
    }

    /**
     * @brief less than operator
     *        comparison target is the view of the buffer. life holder is not compared.
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than the rhs, otherwise false.
     */
    friend
    constexpr bool operator<(buffer const& lhs, buffer const& rhs) noexcept {
        return lhs.view_ < rhs.view_;
    }

    /**
     * @brief less than or equal to operator
     *        comparison target is the view of the buffer. life holder is not compared.
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs less than or equal to the rhs, otherwise false.
     */
    friend
    constexpr bool operator<=(buffer const& lhs, buffer const& rhs) noexcept {
        return lhs.view_ <= rhs.view_;
    }

    /**
     * @brief greater than operator
     *        comparison target is the view of the buffer. life holder is not compared.
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs greater than the rhs, otherwise false.
     */
    friend
    constexpr bool operator>(buffer const& lhs, buffer const& rhs) noexcept {
        return lhs.view_ > rhs.view_;
    }

    /**
     * @brief greater than or equal to operator
     *        comparison target is the view of the buffer. life holder is not compared.
     * @param lhs compare target
     * @param rhs compare target
     * @return true if the lhs greater than or equal to the rhs, otherwise false.
     */
    friend
    constexpr bool operator>=(buffer const& lhs, buffer const& rhs) noexcept {
        return lhs.view_ >= rhs.view_;
    }

    /**
     * @brief output to the stream
     * @param o output stream
     * @param v target
     * @return output stream
     */
    friend
    std::ostream& operator<<(std::ostream& o, buffer const& v) {
        o << v.view_;
        return o;
    }

    /**
     * @brief life checking
     * @return true if the buffer has life, otherwise false.
     */
    bool has_life() const noexcept {
        return bool(life_);
    }

private:
    std::string_view view_;
    life_type life_;
};

/**
 * @brief hashing function
 * @param v target
 * @return hash value
 *
 */
inline std::size_t hash_value(buffer const& v) noexcept {
    std::size_t result = 0;
    boost::hash_combine(result, static_cast<std::string_view const&>(v));
    return result;
}

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
 *
 */
inline const_buffer buffer(async_mqtt::buffer const& data) {
    return buffer(data.data(), data.size());
}

} // namespace asio
} // namespace boost

namespace std {

/**
 * @ingroup buffer
 * @brief class template hash specilization for the buffer
 *
 */
template <>
class hash<async_mqtt::buffer> {
public:
    /**
     * @brief hashing operator
     * @param v target
     * @return hash value
     */
    std::uint64_t operator()(async_mqtt::buffer const& v) const noexcept {
        return std::hash<std::string_view>()(static_cast<std::string_view const&>(v));
    }
};

} // namespace std

#endif // ASYNC_MQTT_UTIL_BUFFER_HPP
