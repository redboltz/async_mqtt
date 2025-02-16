// Copyright Wouter van Kleunen 2021
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_SECURITY_HPP)
#define ASYNC_MQTT_BROKER_SECURITY_HPP

#include <string>
#include <string_view>
#include <map>
#include <set>
#include <optional>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/iterator/function_output_iterator.hpp>

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif // defined(__GNUC__)

// from https://github.com/okdshin/PicoSHA2
// MIT license
#include <broker/external/picosha2.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif // defined(__GNUC__)

#if ASYNC_MQTT_USE_TLS
#include <openssl/evp.h>
#endif

#include <broker/subscription_map.hpp>
#include <async_mqtt/util/log.hpp>
#include <async_mqtt/util/string_view_helper.hpp>




namespace async_mqtt {

/** Remove comments from a JSON file (Single and multi-line comments are allowed) */
inline std::string json_remove_comments(std::istream& input) {
    bool inside_comment = false;
    bool inside_single_quote = false;
    bool inside_double_quote = false;
    bool inside_block_comment = false;

    std::ostringstream result;

    while (true) {
        char c;
        if (input.get(c).eof()) break;

        if (!inside_comment && !inside_block_comment && !inside_double_quote && !inside_single_quote && c == '/') {
            char next;
            if (input.get(next)) {
                if (next == '/') {
                    inside_comment = true;
                }
                else if (next == '*') {
                    inside_block_comment = true;
                }
                else {
                    result << c;
                    result << next;
                    continue;
                }
            }
        }

        if (inside_block_comment) {
            if (c == '*' && input.peek() == '/') {
                input.get(c); // consume '/'
                inside_block_comment = false;
            }
            continue;
        }

        if (c == '\n') inside_comment = false;

        if (!inside_comment && !inside_block_comment) {
            if (!inside_double_quote && !inside_single_quote && c == '\'') inside_single_quote = !inside_single_quote;
            if (!inside_comment && !inside_single_quote && c == '"') inside_double_quote = !inside_double_quote;
            result << c;
        }
    }

    return result.str();
}


struct security {

    static constexpr char const* any_group_name = "@any";

    struct authentication {
        enum class method {
            sha256,
            plain_password,
            client_cert,
            anonymous,
            unauthenticated
        };

        authentication(
            method auth_method = method::sha256,
            std::optional<std::string> const& digest = std::nullopt,
            std::string const& salt = std::string()
        )
            : auth_method(auth_method),
              digest(digest),
              salt(salt)
        {
        }

        method auth_method;
        std::optional<std::string> digest;
        std::string salt;

        std::vector<std::string> groups;
    };


    struct authorization {
        enum class type {
            deny, allow, none
        };

        authorization(std::string_view topic, std::size_t rule_nr)
            : topic(topic),
              rule_nr(rule_nr),
              sub_type(type::none),
              pub_type(type::none)
        {
        }

        std::vector<std::string> topic_tokens;

        std::string topic;
        std::size_t rule_nr;

        type sub_type;
        std::set<std::string> sub;

        type pub_type;
        std::set<std::string> pub;
    };

    struct group {
        std::string name;
        std::vector<std::string> members;
    };

    /** Return username of anonymous user */
    std::optional<std::string> const& login_anonymous() const {
        return anonymous;
    }

    /** Return username of unauthorized user */
    std::optional<std::string> const& login_unauthenticated() const {
        return unauthenticated;
    }

    template<typename T>
    static std::string to_hex(T start, T end) {
        std::string result;
        boost::algorithm::hex(start, end, std::back_inserter(result));
        return result;
    }

    static std::string sha256hash(std::string_view message) {
        std::vector<unsigned char> hash(picosha2::k_digest_size);
        picosha2::hash256(message.begin(), message.end(), hash.begin(), hash.end());
        return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
    }

    bool login_cert(std::string_view username) const {
        auto i = authentication_.find(std::string(username));
        return
            i != authentication_.end() &&
            i->second.auth_method == security::authentication::method::client_cert;
    }

    std::optional<std::string> login(std::string_view username, std::string_view password) const {
        auto i = authentication_.find(std::string(username));
        if (i != authentication_.end() &&
            i->second.auth_method == security::authentication::method::sha256) {
            return [&] () -> std::optional<std::string> {
                if (boost::iequals(
                        i->second.digest.value(),
                        sha256hash(i->second.salt + std::string(password))
                    )
                ) {
                    return std::string(username);
                }
                else {
                    return std::nullopt;
                }
            } ();

        }
        else if (
            i != authentication_.end() &&
            i->second.auth_method == security::authentication::method::plain_password) {
            return [&] () -> std::optional<std::string> {
                if (i->second.digest.value() == password) {
                    return std::string(username);
                }
                else {
                    return std::nullopt;
                }
            } ();
        }
        return std::nullopt;
    }

    static authorization::type get_auth_type(std::string_view type) {
        if (type == "allow") return authorization::type::allow;
        if (type == "deny") return authorization::type::deny;
        throw std::runtime_error(
            "An invalid authorization type was specified: " +
            std::string(type)
        );
    }

    static bool is_valid_group_name(std::string_view name) {
        return !name.empty() && name[0] == '@'; // TODO: validate utf-8
    }

    static bool is_valid_user_name(std::string_view name) {
        return !name.empty() && name[0] != '@'; // TODO: validate utf-8
    }

    std::size_t get_next_rule_nr() const {
        std::size_t rule_nr = 0;
        for (auto const& i: authorization_) {
            rule_nr = std::max(rule_nr, i.rule_nr);
        }
        return rule_nr + 1;
    }

    void default_config() {
        char const* username = "anonymous";
        authentication login(authentication::method::anonymous);
        authentication_.insert({ username, login});
        anonymous = username;

        char const* topic = "#";
        authorization auth(topic, get_next_rule_nr());
        auth.topic_tokens = get_topic_filter_tokens("#");
        auth.sub_type = authorization::type::allow;
        auth.sub.insert(username);
        auth.pub_type = authorization::type::allow;
        auth.pub.insert(username);
        authorization_.push_back(auth);

        groups_.insert({ std::string(any_group_name), group() });

        validate();
    }

    std::size_t add_auth(
        std::string const& topic_filter,
        std::set<std::string> const& pub,
        authorization::type auth_pub_type,
        std::set<std::string> const& sub,
        authorization::type auth_sub_type
    ) {
        for(auto const& j : pub) {
            if (!is_valid_user_name(j) && !is_valid_group_name(j)) {
                throw std::runtime_error(
                    "An invalid username or groupname was specified for the authorization: " + j
                );
            }
            validate_entry("topic " + topic_filter, j);
        }

        for(auto const& j : sub) {
            if (!is_valid_user_name(j) && !is_valid_group_name(j)) {
                throw std::runtime_error(
                    "An invalid username or groupname was specified for the authorization: " + j
                );
            }
            validate_entry("topic " + topic_filter, j);
        }

        std::size_t rule_nr = get_next_rule_nr();
        authorization auth(topic_filter, rule_nr);
        auth.topic_tokens = get_topic_filter_tokens(topic_filter);
        auth.pub = pub;
        auth.pub_type = auth_pub_type;
        auth.sub = sub;
        auth.sub_type = auth_sub_type;

        for (auto const& j: sub) {
            auth_sub_map.insert_or_assign(
                topic_filter,
                j,
                std::make_pair(auth_sub_type, rule_nr)
            );
        }
        for (auto const& j: pub) {
            auth_pub_map.insert_or_assign(
                topic_filter,
                j,
                std::make_pair(auth_pub_type, rule_nr)
            );
        }

        authorization_.push_back(auth);
        return rule_nr;
    }

    void remove_auth(std::size_t rule_nr)
    {
        for (auto i = authorization_.begin(); i != authorization_.end(); ++i) {
            if (i->rule_nr == rule_nr) {
                for (auto const& j: i->sub) {
                    auth_sub_map.erase(i->topic, j);
                }
                for (auto const& j: i->pub) {
                    auth_pub_map.erase(i->topic, j);
                }

                authorization_.erase(i);
                return;
            }
        }
    }

    void load_json(std::istream& input) {
        // Create a root
        boost::property_tree::ptree root;

        std::istringstream input_without_comments(json_remove_comments(input));
        boost::property_tree::read_json(input_without_comments, root);

        groups_.insert({ std::string(any_group_name), group() });

        for (auto const& i: root.get_child("authentication")) {
            std::string name = i.second.get<std::string>("name");
            if (!is_valid_user_name(name)) {
                throw std::runtime_error("An invalid username was specified: " + name);
            }

            std::string method = i.second.get<std::string>("method");

            if (method == "sha256") {
                std::string digest = i.second.get<std::string>("digest");
                std::string salt = i.second.get<std::string>("salt", "");

                authentication auth(authentication::method::sha256, digest, salt);
                authentication_.insert( { name, auth });
            }
            else if (method == "plain_password") {
                std::string digest = i.second.get<std::string>("password");

                authentication auth(authentication::method::plain_password, digest);
                authentication_.insert( { name, auth });
            }
            else if (method == "client_cert") {
                authentication auth(authentication::method::client_cert);
                authentication_.insert({ name, auth });
            }
            else if (method == "anonymous") {
                if(anonymous) {
                    throw std::runtime_error(
                        "Only a single anonymous user can be configured, anonymous user: " +
                        *anonymous
                    );
                }
                anonymous = name;

                authentication auth(authentication::method::anonymous);
                authentication_.insert( { name, auth });
            }
            else if (method == "unauthenticated") {
                if (unauthenticated) {
                    throw std::runtime_error(
                        "Only a single unauthenticated user can be configured, unauthenticated user: " +
                        *unauthenticated
                    );
                }
                unauthenticated = name;

                authentication auth(authentication::method::unauthenticated);
                authentication_.insert( { name, auth });
            }
            else {
                throw std::runtime_error("An invalid method was specified: " + method);
            }
        }
        if (root.get_child_optional("group")) {
            for (auto const& i: root.get_child("group")) {
                std::string name = i.second.get<std::string>("name");
                if (!is_valid_group_name(name)) {
                    throw std::runtime_error("An invalid group name was specified: " + name);
                }

                group group;
                if (i.second.get_child_optional("members")) {
                    for (auto const& j: i.second.get_child("members")) {
                        auto username = j.second.get_value<std::string>();
                        if (!is_valid_user_name(username)) {
                            throw std::runtime_error("An invalid user name was specified: " + username);
                        }
                        group.members.push_back(username);
                    }
                }

                groups_.insert({ name, group });
            }
        }

        for (auto const& i: root.get_child("authorization")) {
            std::string name = i.second.get<std::string>("topic");
            if (!validate_topic_filter(name)) {
                throw std::runtime_error("An invalid topic filter was specified: " + name);
            }

            authorization auth(name, get_next_rule_nr());
            auth.topic_tokens = get_topic_filter_tokens(name);

            if (i.second.get_child_optional("allow")) {
                auto &allow = i.second.get_child("allow");
                if (allow.get_child_optional("sub")) {
                    for (auto const& j: allow.get_child("sub")) {
                        auth.sub.insert(j.second.get_value<std::string>());
                    }
                    auth.sub_type = authorization::type::allow;
                }

                if (allow.get_child_optional("pub")) {
                    for (auto const& j: allow.get_child("pub")) {
                        auth.pub.insert(j.second.get_value<std::string>());
                    }
                    auth.pub_type = authorization::type::allow;
                }
            }

            if (i.second.get_child_optional("deny")) {
                auto &deny = i.second.get_child("deny");
                if (deny.get_child_optional("sub")) {
                    for (auto const& j: deny.get_child("sub")) {
                        auth.sub.insert(j.second.get_value<std::string>());
                    }
                    auth.sub_type = authorization::type::deny;
                }

                if (deny.get_child_optional("pub")) {
                    for (auto const& j: deny.get_child("pub")) {
                        auth.pub.insert(j.second.get_value<std::string>());
                    }
                    auth.pub_type = authorization::type::deny;
                }

            }
            authorization_.push_back(auth);
        }

        validate();
    }

    template<typename T>
    void get_auth_sub_by_user(std::string_view username, T&& callback) const {
        std::set<std::string> username_and_groups;
        username_and_groups.insert(std::string(username));

        for (auto const& i: groups_) {
            if (i.first == any_group_name ||
                std::find(
                    i.second.members.begin(),
                    i.second.members.end(),
                    username
                ) != i.second.members.end()
            ) {
                username_and_groups.insert(i.first);
            }
        }

        for (auto const& i: authorization_) {
            if (i.sub_type != authorization::type::none) {
                bool sets_intersect = false;
                auto store_intersect =
                    [&sets_intersect]
                    (std::string const &) mutable {
                        sets_intersect = true;
                    };

                std::set_intersection(
                    i.sub.begin(),
                    i.sub.end(),
                    username_and_groups.begin(),
                    username_and_groups.end(),
                    boost::make_function_output_iterator(std::ref(store_intersect))
                );

                if (sets_intersect) {
                    std::forward<T>(callback)(i);
                }
            }
        }
    }

    authorization::type auth_pub(std::string_view topic, std::string_view username) const {
        authorization::type result_type = authorization::type::deny;

        std::set<std::string> username_and_groups;
        username_and_groups.insert(std::string(username));

        for (auto const& i : groups_) {
            if (i.first == any_group_name ||
                std::find(
                    i.second.members.begin(),
                    i.second.members.end(),
                    username
                ) != i.second.members.end()
            ) {
                username_and_groups.insert(i.first);
            }
        }

        std::size_t priority = 0;
        auth_pub_map.find(
            topic,
            [&](
                std::string const& allowed_username,
                std::pair<authorization::type, std::size_t> entry
            ) {
                if (username_and_groups.find(allowed_username) != username_and_groups.end()) {
                    if (entry.second >= priority) {
                        result_type = entry.first;
                        priority = entry.second;
                    }
                }
            }
        );

        return result_type;
    }

    std::map<std::string, authorization::type> auth_sub(std::string_view topic) const {
        std::map<std::string, authorization::type> result;
        std::map<std::string, std::size_t> priorities;
        auth_sub_map.find(
            topic,
            [&](
                std::string const &allowed_username,
                std::pair<authorization::type, std::size_t> entry
            ) {
                auto rit = result.find(allowed_username);
                if (rit == result.end()) {
                    result.emplace(allowed_username, entry.first);
                    priorities.emplace(allowed_username, entry.second);
                }
                else {
                    auto pit = priorities.find(allowed_username);
                    BOOST_ASSERT(pit != priorities.end());
                    if (pit->second <= entry.second) {
                        pit->second = entry.second;
                        rit->second = entry.first;
                    }
                }
            }
        );

        return result;
    }

    authorization::type auth_sub_user(
        std::map<std::string, authorization::type> const& result,
        std::string const& username) const {
        auto i = result.find(username);
        if (i != result.end()) return i->second;

        for (auto const& i: groups_) {
            if (i.first == any_group_name ||
                std::find(
                    i.second.members.begin(),
                    i.second.members.end(),
                    username
                ) != i.second.members.end()
            ) {
                auto j = result.find(i.first);
                if (j != result.end()) return j->second;
            }
        }

        return authorization::type::deny;
    }

    static bool is_hash(std::string_view level) { return level == "#"; }
    static bool is_plus(std::string_view level) { return level == "+"; }
    static bool is_literal(std::string_view level) { return !is_hash(level) && !is_plus(level); }

    static std::optional<std::string>
    is_subscribe_allowed(
        std::vector<std::string> const& authorized_filter,
        std::string_view subscription_filter
    ) {
        std::optional<std::string> result;
        auto append_result =
            [&result](std::string_view token) {
                if (result) {
                    *result += topic_filter_separator;
                    result->append(token.data(), token.size());
                }
                else {
                    result = std::string(token);
                }
            };

        auto filter_begin = authorized_filter.begin();

        auto subscription_begin = subscription_filter.begin();
        auto subscription_next = topic_filter_tokenizer_next(subscription_begin, subscription_filter.end());

        while (true) {
            if (filter_begin == authorized_filter.end()) {
                return std::nullopt;
            }

            auto auth = *filter_begin;
            ++filter_begin;

            if (is_hash(auth)) {
                append_result(
                    make_string_view(
                        subscription_begin,
                        subscription_filter.end()
                    )
                );
                return result;
            }

            auto sub = make_string_view(
                subscription_begin,
                subscription_next
            );

            if (is_hash(sub)) {
                append_result(auth);

                while (filter_begin < authorized_filter.end()) {
                    append_result(*filter_begin);
                    ++filter_begin;
                }

                return result;
            }

            if (is_plus(auth)) {
                append_result(sub);
            }
            else if (is_plus(sub)) {
                append_result(auth);
            }
            else {
                if (auth != sub)  {
                    return std::nullopt;
                }

                append_result(auth);
            }

            if (subscription_next == subscription_filter.end()) break;
            subscription_begin = std::next(subscription_next);
            subscription_next =
                topic_filter_tokenizer_next(
                    subscription_begin,
                    subscription_filter.end()
                );
        }

        if (filter_begin < authorized_filter.end()) {
            return std::nullopt;
        }

        return result;
    }

    static bool is_subscribe_denied(
        std::vector<std::string> const& deny_filter,
        std::string_view subscription_filter
    ) {
        bool result = true;
        auto filter_begin = deny_filter.begin();

        auto tokens_count =
            topic_filter_tokenizer(
                subscription_filter,
                [&](auto sub) {
                    if (filter_begin == deny_filter.end()) {
                        result = false;
                        return false;
                    };

                    std::string deny = *filter_begin;
                    ++filter_begin;

                    if (deny != sub) {
                        if (is_hash(deny)) {
                            result = true;
                            return false;
                        }

                        if (is_hash(sub)) {
                            result = false;
                            return false;
                        }

                        if (is_plus(deny)) {
                            result = true;
                            return true;
                        }

                        result = false;
                        return false;
                    }

                    return true;
                }
            );

        return result && (tokens_count == deny_filter.size());
    }

    std::vector<std::string>
    get_auth_sub_topics(std::string_view username, std::string_view topic_filter) const {
        std::vector<std::string> auth_topics;
        get_auth_sub_by_user(
            username,
            [&](authorization const& i) {
                if (i.sub_type == authorization::type::allow) {
                    auto entry = is_subscribe_allowed(i.topic_tokens, topic_filter);
                    if (entry) {
                        auth_topics.push_back(*entry);
                    }
                }
                else {
                    for (auto j = auth_topics.begin(); j != auth_topics.end();) {
                        if (is_subscribe_denied(i.topic_tokens, topic_filter)) {
                            j = auth_topics.erase(j);
                        }
                        else {
                            ++j;
                        }
                    }
                }
            }
        );
        return auth_topics;
    }

    /**
     * @brief Determine if user is allowed to subscribe to the specified topic filter
     * @param username The username to check
     * @param topic_filter Topic filter the user would like to subscribe to
     * @return true if the user is authorized
     */
    bool is_subscribe_authorized(std::string_view username, std::string_view topic_filter) const {
        return !get_auth_sub_topics(username, topic_filter).empty();
    }

    // Get the individual path elements of the topic filter
    static std::vector<std::string> get_topic_filter_tokens(std::string_view topic_filter) {
        std::vector<std::string> result;
        topic_filter_tokenizer(
            topic_filter,
            [&result](auto str) {
                result.push_back(std::string(str));
                return true;
            }
        );

        return result;
    }

    std::map<std::string, authentication> authentication_;
    std::map<std::string, group> groups_;

    std::vector<authorization> authorization_;

    std::optional<std::string> anonymous;
    std::optional<std::string> unauthenticated;

    using auth_map_type = multiple_subscription_map<std::string, std::pair<authorization::type, std::size_t>>;
    auth_map_type auth_pub_map;
    auth_map_type auth_sub_map;

private:
    void validate_entry(std::string const& context, std::string const& name) const {
        if (is_valid_group_name(name) && groups_.find(name) == groups_.end()) {
            throw std::runtime_error("An invalid group name was specified for " + context + ": " + name);
        }
        if (is_valid_user_name(name) && authentication_.find(name) == authentication_.end()) {
            throw std::runtime_error("An invalid username name was specified for " + context + ": " + name);
        }
    }

    void validate() {
        for (auto const& i : groups_) {
            for (auto const& j : i.second.members) {
                auto iter = authentication_.find(j);
                if(is_valid_user_name(j) && iter == authentication_.end())
                    throw std::runtime_error("An invalid username name was specified for group " + i.first + ": " + j);
            }
        }

        std::string unsalted;
        for (auto const& i : authentication_) {
            if (i.second.auth_method == authentication::method::sha256 && i.second.salt.empty()) {
                if (!unsalted.empty()) unsalted += ", ";
                unsalted += i.first;
            }
        }

        if (!unsalted.empty()) {
            ASYNC_MQTT_LOG("mqtt_broker", warning)
                << "The following users have no salt specified: "
                << unsalted;
        }

        for (auto const& i : authorization_) {
            for (auto const& j: i.sub) {
                validate_entry("topic " + i.topic, j);

                if (is_valid_user_name(j) || is_valid_group_name(j)) {
                    auth_sub_map.insert_or_assign(i.topic, j, std::make_pair(i.sub_type, i.rule_nr));
                }
            }
            for (auto const& j: i.pub) {
                validate_entry("topic " + i.topic, j);

                if(is_valid_user_name(j) || is_valid_group_name(j)) {
                    auth_pub_map.insert_or_assign(i.topic, j, std::make_pair(i.pub_type, i.rule_nr));
                }
            }
        }
    }

};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_SECURITY_HPP
