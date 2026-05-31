module;

export module cactus.core.parser;

import cactus.common;
import cactus.core.strat;

namespace cactus {

export enum TokenType : int { IDENTIFIER = 256, NUMBER, STRING };

export struct Token {
    TokenType type;
    size_t len;

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

    DynamicArray<Token> tokens;

    DynamicArray<data_t> data;
    DynamicArray<char> string_data;
    DynamicArray<IdentifierData> identifier_data_list;
    HashMap<std::string, size_t> identifier_to_id_map;
    // TODO: futher optimize by make a buffer to store all identifier's name since it should be all mostly static

    size_t next_identifier_id = 0;
    size_t next_string_data_index = 0;

    [[nodiscard]] static auto make() noexcept -> Parser {
        return Parser{.tokens = DynamicArray<Token>::make(),
                      .data = DynamicArray<data_t>::make(),
                      .string_data = DynamicArray<char>::make(),
                      .identifier_data_list = DynamicArray<IdentifierData>::make(),
                      .identifier_to_id_map = HashMap<std::string, size_t>::make(),
                      .next_identifier_id = 0,
                      .next_string_data_index = 0};
    }
    auto destroy() {
        tokens.destroy();
        data.destroy();
        string_data.destroy();
        identifier_data_list.destroy();
        identifier_to_id_map.destroy();
    }
    [[nodiscard]] auto clone() = delete; // FIXME

    auto parse(const stdf::path &dir) {
        std::string content = get_and_preprocess_file(dir).value(); // TODO error
        std::println("string:\n{}\n", content);

        handle_lexer(content);

        for (auto p : tokens) {
            std::println("index {} at {}:{} : token = {}; len = {} >> {}", p.index, p.line, p.column, (size_t)p.type, p.len,
                         std::string_view(content.data() + p.index, p.len));
        }

        std::stack<size_t> identifier_stack{};
        short just_after_identifier = 0;
        auto pop_identifier_stack = [&]() {
            if (identifier_stack.empty()) return;

            _assert(identifier_stack.top() < identifier_data_list.len,
                    "identifier_stack.top() should be store index to identifier_data_list which means is in bound.");
            IdentifierData old_identifier_data = identifier_data_list.get(identifier_stack.top()).value();
            size_t old_data_len = old_identifier_data.data_len;
            size_t old_parent_len = old_identifier_data.parent_len;

            identifier_stack.pop();

            if (identifier_stack.empty()) return;

            _assert(identifier_stack.top() < identifier_data_list.len,
                    "identifier_stack.top() should be store index to identifier_data_list which means is in bound.");
            IdentifierData *identifier_data = identifier_data_list.get_ptr(identifier_stack.top());
            identifier_data->data_len += old_data_len;
            identifier_data->parent_len += old_parent_len;
        };
        for (auto token : tokens) {
            switch ((int)token.type) {
            case TokenType::IDENTIFIER: {
                std::string str = content.substr(token.index, token.len);
                identifier_to_id_map.add(std::move(str), next_identifier_id);
                identifier_data_list.append({.point_to = data.len, .data_len = 0, .parent_len = 1});

                identifier_stack.push(next_identifier_id);

                ++next_identifier_id;

                just_after_identifier = 2; // cheat flag by set = 2 then decrease 2 times (because I dont want to somehow exit
                                           // this case without decrease this flag value)
            } break;
            case TokenType::NUMBER: {
                std::string_view strv(content.data() + token.index, token.len);

                int i_val;
                auto res_i = std::from_chars(strv.data(), strv.data() + strv.size(), i_val);

                if (res_i.ptr == strv.data() + strv.size()) {
                    data.append(i_val);
                } else {
                    float f_val;
                    auto res_f = std::from_chars(strv.data(), strv.data() + strv.size(), f_val);

                    if (res_f.ptr == strv.data() + strv.size()) {
                        data.append(f_val);
                    } else {
                        std::println("{}:{}: parse number failed, got {}", token.line, token.column, strv);
                        _assert(false, "failed on parsing number");
                        return;
                        // TODO error
                    }
                }

                _assert(!identifier_stack.empty(), "identifier should not be empty yet");
                _assert(identifier_stack.top() < identifier_data_list.len,
                        "identifier_stack.top() should be store index to identifier_data_list which means is in bound.");
                ++identifier_data_list.get_ptr(identifier_stack.top())->data_len;
                if (just_after_identifier) pop_identifier_stack();
            } break;
            case TokenType::STRING: {
                std::string_view strv(content.data() + token.index, token.len);

                data.append((unsigned int)next_string_data_index);

                for (char c : strv) string_data.append(c); // FIXME gotta do a range append to dynamic array
                string_data.append('\0');

                next_string_data_index += strv.size() + 1;

                _assert(!identifier_stack.empty(), "identifier should not be empty yet");
                _assert(identifier_stack.top() < identifier_data_list.len,
                        "identifier_stack.top() should be store index to identifier_data_list which means is in bound.");
                ++identifier_data_list.get_ptr(identifier_stack.top())->data_len;
                if (just_after_identifier) pop_identifier_stack();
            } break;
            case '{': {
            } break;
            case '}': {
                pop_identifier_stack();
            } break;
            default: {
                std::println("{}:{}: not recorgnize symbol", token.line, token.column);
                _assert(false, "Not recorgnize symbol");
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
            _assert(id < identifier_data_list.len, "id should be store index to identifier_data_list which means is in bound.");
            IdentifierData data = identifier_data_list.get(id).value();
            std::println("{}: point_to: {}; data_len: {}; parent_len: {}", name, data.point_to, data.data_len, data.parent_len);
        }
        std::println();
    }

private:
    auto get_and_preprocess_file(const stdf::path &dir) noexcept -> std::optional<std::string> {
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

    auto handle_lexer(std::string_view content_view) noexcept -> bool { // TODO error handle
        enum struct ReadState {
            EXPECT_IDENTIFIER,
            EXPECT_VALUE_OR_ARR,
            AFTER_IDENTIFIER,
            AFTER_VALUE,
            AFTER_SEPARATOR,
            AFTER_ARRAY_BEGIN,
            AFTER_ARRAY_END,
        } state = ReadState::EXPECT_IDENTIFIER;

        Token cur_token;

        auto advance_location_horizontal = [&](size_t v) noexcept {
            cur_token.index += v;
            cur_token.column += v;
            content_view.remove_prefix(v);
        };
        auto append_token = [&](TokenType type, size_t len) noexcept {
            cur_token.type = type;
            cur_token.len = len;
            tokens.append(cur_token);
        };

        auto get_identifier_len = [&](const std::string_view &trimmed_content_view) noexcept -> size_t {
            size_t len = 0;
            for (auto c : trimmed_content_view) {
                if (std::isalnum((unsigned char)c) || c == '_')
                    ++len;
                else
                    break;
            }

            return len;
        };

        auto might_be_value = [&](char c) noexcept -> bool { return std::isdigit(c) || c == '-' || c == '\"'; };

        while (true) {
            bool line_breaked = false;
            if (content_view.empty()) break;
            if (content_view.front() == '\n' || content_view.front() == '\t' || content_view.front() == ' ') {
                handle_remove_spaces(&content_view, &cur_token, &line_breaked);
                if (content_view.empty()) break;
            }

            switch (state) {
            case ReadState::EXPECT_IDENTIFIER: {
                if (might_be_value(content_view.front())) {
                    std::println("{}:{} expect identifier got {}", cur_token.line, cur_token.column,
                                 content_view.substr(0, content_view.find_first_not_of(" \n\t")));
                    _assert(false, "Excepting identifier but not");
                    return false; // TODO error
                }

                size_t len = get_identifier_len(content_view);
                append_token(TokenType::IDENTIFIER, len);
                advance_location_horizontal(len);

                state = ReadState::AFTER_IDENTIFIER;
            } break;

            case ReadState::EXPECT_VALUE_OR_ARR: {
                switch (content_view.front()) {
                case '{': {
                    append_token((TokenType)content_view.front(), 1);
                    advance_location_horizontal(1);

                    state = ReadState::AFTER_ARRAY_BEGIN;
                } break;
                case '\"': {
                    advance_location_horizontal(1);

                    size_t end_string_i = content_view.find_first_of("\"");
                    if (end_string_i != 0) append_token(TokenType::STRING, end_string_i);

                    advance_location_horizontal(end_string_i + 1);

                    state = ReadState::AFTER_VALUE;
                } break;
                default: {
                    if (might_be_value(content_view.front())) {
                        size_t num_len = content_view.find_first_of(" \t\n,}");
                        append_token(TokenType::NUMBER, num_len);
                        advance_location_horizontal(num_len);
                    } else {
                        std::println("{}:{} expect value got {}", cur_token.line, cur_token.column,
                                     content_view.substr(0, content_view.find_first_not_of(" \n\t")));

                        _assert(false, "Expecting value but not");
                        return false; // TODO error
                    }

                    state = ReadState::AFTER_VALUE;
                } break;
                }
            } break;

            case ReadState::AFTER_IDENTIFIER: {
                if (content_view.front() != '=' && content_view.front() != '{') {
                    std::println("{}:{} expect {{ or = got {}", cur_token.line, cur_token.column,
                                 content_view.substr(0, content_view.find_first_not_of(" \n\t")));

                    _assert(false, "Expecting { but not");
                    return false; // TODO error
                }

                // the "{" will be handle later
                if (content_view.front() == '=') { advance_location_horizontal(1); }

                state = ReadState::EXPECT_VALUE_OR_ARR;
            } break;

            case ReadState::AFTER_VALUE: {
                switch (content_view.front()) {
                case ',':
                    advance_location_horizontal(1);

                    state = ReadState::AFTER_SEPARATOR;
                    break;
                case '}':
                    append_token((TokenType)content_view.front(), 1);
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

    auto handle_remove_spaces(std::string_view *content_view, Token *token, bool *line_breaked) noexcept -> void {
        constexpr size_t TAB_WIDTH = 4;

        size_t removed = 0;
        for (auto c : *content_view) {
            if (c == '\n') {
                ++token->line;
                token->column = 1;
                *line_breaked = true;
            } else if (c == '\t') {
                token->column += TAB_WIDTH - ((token->column - 1) % TAB_WIDTH);
            } else if (c == ' ') {
                ++token->column;
            } else {
                break;
            }

            ++token->index;

            ++removed;
        }

        content_view->remove_prefix(removed);
    }
};

} // namespace cactus
