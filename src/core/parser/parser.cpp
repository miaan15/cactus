module;

#include <cassert>

export module cactus.core.parser;

import cactus.common;

namespace cactus {

export enum TokenType : int { IDENTIFIER = 256, NUMBER, STRING };

export struct Token {
    TokenType type;
    size_t len;
};

export struct ParserLocation {
    size_t index = 0;
    size_t column = 1;
    size_t line = 1;
};

export struct Parser {
    using data_t = std::variant<int, float, unsigned int>;
    struct IdentifierData {
        size_t point_to;
        size_t data_len;
        size_t parent_len;
    };

    std::vector<std::pair<ParserLocation, Token>> tokens;

    std::vector<data_t> data;
    std::vector<char> string_data;
    std::vector<IdentifierData> identifier_data_list;
    std::flat_map<std::string, size_t> identifier_to_id_map;

    size_t next_identifier_id = 0;
    size_t next_string_data_index = 0;

    explicit Parser() = default;
    ~Parser() = default;
    Parser(const Parser &other) = delete;
    Parser &operator=(const Parser &other) = delete;
    Parser(Parser &&other) noexcept = default;
    Parser &operator=(Parser &&other) noexcept = default;

    auto parse(const stdf::path &dir) {
        std::string content = get_and_preprocess_file(dir).value(); // TODO error
        std::println("string:\n{}\n", content);

        handle_lexer(content);

        for (auto p : tokens) {
            std::println("index {} at {}:{} : token = {}; len = {} >> {}", p.first.index, p.first.line, p.first.column,
                         (size_t)p.second.type, p.second.len, std::string_view(content.data() + p.first.index, p.second.len));
        }

        std::stack<size_t> identifier_stack{};
        short just_after_identifier = 0; // cheat flag by set = 2 then decrease 2 times
        auto pop_identifier_stack = [&]() {
            if (identifier_stack.empty()) return;

            const auto &old_identifier_data = identifier_data_list[identifier_stack.top()];
            auto old_data_len = old_identifier_data.data_len;
            auto old_parent_len = old_identifier_data.data_len;

            identifier_stack.pop();

            if (identifier_stack.empty()) return;

            auto &identifier_data = identifier_data_list[identifier_stack.top()];
            identifier_data.data_len += old_data_len;
            identifier_data.parent_len += old_parent_len;
        };
        for (auto &[location, token] : tokens) {
            switch ((int)token.type) {
            case TokenType::IDENTIFIER: {
                std::string str = content.substr(location.index, token.len);
                identifier_to_id_map[std::move(str)] = next_identifier_id;
                identifier_data_list.push_back({.point_to = data.size(), .data_len = 0, .parent_len = 1});

                identifier_stack.push(next_identifier_id);

                ++next_identifier_id;

                just_after_identifier = 2;
            } break;
            case TokenType::NUMBER: {
                std::string_view strv(content.data() + location.index, token.len);

                int i_val;
                auto res_i = std::from_chars(strv.data(), strv.data() + strv.size(), i_val);

                if (res_i.ptr == strv.data() + strv.size()) {
                    data.push_back(i_val);
                } else {
                    float f_val;
                    auto res_f = std::from_chars(strv.data(), strv.data() + strv.size(), f_val);

                    if (res_f.ptr == strv.data() + strv.size()) {
                        data.push_back(f_val);
                    } else {
                        std::println("{}:{}: parse number failed, got {}", location.line, location.column, strv);
                        assert(false);
                        return;
                        // TODO error
                    }
                }

                assert(!identifier_stack.empty());
                ++identifier_data_list[identifier_stack.top()].data_len;
                if (just_after_identifier) pop_identifier_stack();
            } break;
            case TokenType::STRING: {
                std::string_view strv(content.data() + location.index, token.len);

                data.push_back((unsigned int)next_string_data_index);

                string_data.insert(string_data.end(), strv.begin(), strv.end());
                string_data.push_back('\0');

                next_string_data_index += strv.size() + 1;

                assert(!identifier_stack.empty());
                ++identifier_data_list[identifier_stack.top()].data_len;
                if (just_after_identifier) pop_identifier_stack();
            } break;
            case '{': {
            } break;
            case '}': {
                pop_identifier_stack();
            } break;
            default: {
                std::println("{}:{}: not recorgnize symbol", location.line, location.column);
                assert(false);
                return;
                // TODO error
            } break;
            }

            if (just_after_identifier) --just_after_identifier;
        }

        std::println();
        std::print("data:\t");
        for (auto v : data) {
            std::visit([](auto &&arg) { std::print("{} ", arg); }, v);
        }
        std::println();

        std::print("str:\t");
        for (auto v : string_data) { std::print("{} ", v == '\0' ? '~' : v); }
        std::println();

        std::println("identifier:");
        for (auto [name, id] : identifier_to_id_map) {
            auto data = identifier_data_list[id];
            std::println("{}: point_to: {}; data_len: {}; parent_len: {}", name, data.point_to, data.data_len, data.parent_len);
        }
        std::println();
    }

private:
    auto get_and_preprocess_file(const stdf::path &dir) -> std::optional<std::string> {
        stdf::path full_dir = _root_dir / dir;

        std::error_code ec;
        auto size = stdf::file_size(full_dir, ec);
        if (ec || size == 0) return {}; // TODO error handle

        std::ifstream file(full_dir, std::ios::in | std::ios::binary);
        if (!file) return {}; // TODO error handle

        std::string content{};
        content.resize(size);

        file.read(content.data(), size);
        content.resize(file.gcount());

        content.erase(
            std::remove_if(content.begin(), content.end(), [](unsigned char c) { return c == '\r' || c == '\v' || c == '\f'; }),
            content.end());

        content.shrink_to_fit();

        return content;
    }

    auto handle_lexer(std::string_view content_view) -> bool { // TODO error handle
        enum struct ReadState {
            EXPECT_IDENTIFIER,
            EXPECT_VALUE_OR_ARR,
            AFTER_IDENTIFIER,
            AFTER_VALUE,
            AFTER_SEPARATOR,
            AFTER_ARRAY_BEGIN,
            AFTER_ARRAY_END,
        } state = ReadState::EXPECT_IDENTIFIER;

        ParserLocation cur_location;

        auto advance_location_horizontal = [&](size_t v) {
            cur_location.index += v;
            cur_location.column += v;
            content_view.remove_prefix(v);
        };
        while (true) {
            bool line_breaked = false;
            if (content_view.empty()) break;
            if (content_view.front() == '\n' || content_view.front() == '\t' || content_view.front() == ' ') {
                handle_remove_spaces(&content_view, &cur_location, &line_breaked);
                if (content_view.empty()) break;
            }

            switch (state) {
            case ReadState::EXPECT_IDENTIFIER: {
                if (might_be_value(content_view.front())) {
                    std::println("{}:{} expect identifier got {}", cur_location.line, cur_location.column,
                                 content_view.substr(0, content_view.find_first_not_of(" \n\t")));
                    assert(false);
                    return false; // TODO error
                }

                size_t len = get_identifier_len(content_view);
                tokens.push_back({cur_location, Token{.type = TokenType::IDENTIFIER, .len = len}});
                advance_location_horizontal(len);

                state = ReadState::AFTER_IDENTIFIER;
            } break;

            case ReadState::EXPECT_VALUE_OR_ARR: {
                switch (content_view.front()) {
                case '{': {
                    tokens.push_back({cur_location, Token{.type = (TokenType)content_view.front(), .len = 1}});
                    advance_location_horizontal(1);

                    state = ReadState::AFTER_ARRAY_BEGIN;
                } break;
                case '\"': {
                    advance_location_horizontal(1);

                    size_t end_string_i = content_view.find_first_of("\"");
                    if (end_string_i != 0)
                        tokens.push_back({cur_location, Token{.type = TokenType::STRING, .len = end_string_i}});

                    advance_location_horizontal(end_string_i + 1);

                    state = ReadState::AFTER_VALUE;
                } break;
                default: {
                    if (might_be_value(content_view.front())) {
                        size_t num_len = content_view.find_first_of(" \t\n,}");
                        tokens.push_back({cur_location, Token{.type = TokenType::NUMBER, .len = num_len}});
                        advance_location_horizontal(num_len);
                    } else {
                        std::println("{}:{} expect value got {}", cur_location.line, cur_location.column,
                                     content_view.substr(0, content_view.find_first_not_of(" \n\t")));

                        assert(false);
                        return false; // TODO error
                    }

                    state = ReadState::AFTER_VALUE;
                } break;
                }
            } break;

            case ReadState::AFTER_IDENTIFIER: {
                if (content_view.front() != '=' && content_view.front() != '{') {
                    std::println("{}:{} expect {{ or = got {}", cur_location.line, cur_location.column,
                                 content_view.substr(0, content_view.find_first_not_of(" \n\t")));

                    assert(false);
                    return false; // TODO error
                }

                // the "{" will be handle later
                if (content_view.front() == '=') {
                    advance_location_horizontal(1);
                }

                state = ReadState::EXPECT_VALUE_OR_ARR;
            } break;

            case ReadState::AFTER_VALUE: {
                switch (content_view.front()) {
                case ',':
                    advance_location_horizontal(1);

                    state = ReadState::AFTER_SEPARATOR;
                    break;
                case '}':
                    tokens.push_back({cur_location, Token{.type = (TokenType)content_view.front(), .len = 1}});
                    advance_location_horizontal(1);

                    state = ReadState::AFTER_ARRAY_END;
                    break;
                default:
                    state = ReadState::EXPECT_IDENTIFIER;
                    break;
                }
            } break;

            case ReadState::AFTER_SEPARATOR: {
                if (might_be_value(content_view.front())) {
                    state = ReadState::EXPECT_VALUE_OR_ARR;
                } else if (content_view.front() == '}') {
                    advance_location_horizontal(1);

                    state = ReadState::AFTER_ARRAY_END;
                } else {
                    state = ReadState::EXPECT_IDENTIFIER;
                }
            } break;

            case ReadState::AFTER_ARRAY_BEGIN: {
                if (might_be_value(content_view.front())) {
                    state = ReadState::EXPECT_VALUE_OR_ARR;
                } else {
                    state = ReadState::EXPECT_IDENTIFIER;
                }
            } break;

            case ReadState::AFTER_ARRAY_END: {
                if (content_view.front() == ',') {
                    advance_location_horizontal(1);

                    state = ReadState::AFTER_SEPARATOR;
                } else {
                    state = ReadState::EXPECT_IDENTIFIER;
                }
            } break;
            }
        }

        return true;
    }

    auto handle_remove_spaces(std::string_view *content_view, ParserLocation *location, bool *line_breaked) -> void {
        constexpr size_t TAB_WIDTH = 4;

        size_t removed = 0;
        for (auto c : *content_view) {
            if (c == '\n') {
                ++location->line;
                location->column = 1;
                *line_breaked = true;
            } else if (c == '\t') {
                location->column += TAB_WIDTH - ((location->column - 1) % TAB_WIDTH);
            } else if (c == ' ') {
                ++location->column;
            } else {
                break;
            }

            ++location->index;

            ++removed;
        }

        content_view->remove_prefix(removed);
    }

    auto get_identifier_len(const std::string_view &trimmed_content_view) -> size_t {
        size_t len = 0;
        for (auto c : trimmed_content_view) {
            if (std::isalnum((unsigned char)c) || c == '_') {
                ++len;
            } else {
                break;
            }
        }

        return len;
    }

    auto might_be_value(char c) -> bool { return std::isdigit(c) || c == '-' || c == '\"'; }
};

} // namespace cactus
