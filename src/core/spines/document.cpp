module;

export module cactus.core.spines:document;

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

export struct SpinesDocumentParsingError {
    enum struct Type {
        NONE,
        FILE_NOT_EXISTED,
        FILE_PERMISSTION_DENIED,
        FILE_IO_ERROR,
        FILE_UNKNOWN_ERROR,
        INVALID_IDENTIFIER_NAME,
        INVALID_VALUE,
        INVALID_SYNTAX,
    } type = Type::NONE;

    size_t index = 0;
    size_t column = 1;
    size_t line = 1;
};
using parsing_error_type_t = SpinesDocumentParsingError::Type;

export struct SpinesDocument {
    using data_t = std::variant<int, float, unsigned int>;
    struct IdentifierNameView {
        size_t offset_root;
        size_t len;
    };
    struct IdentifierOwnedData {
        size_t point_to;
        size_t data_len;
        size_t parent_len;
    };
    struct IdentifierData {
        IdentifierNameView name;
        IdentifierOwnedData owned_data;
    };

    std::string spines_source;

    DynamicArray<Token> tokens;

    DynamicArray<data_t> data;
    DynamicArray<char> string_data;
    DynamicArray<IdentifierData> identifier_data_list;

    size_t next_identifier_id = 0;
    size_t next_string_data_index = 0;

    [[nodiscard]] static auto make() noexcept -> SpinesDocument {
        return SpinesDocument{.spines_source{},
                              .tokens = DynamicArray<Token>::make(),
                              .data = DynamicArray<data_t>::make(),
                              .string_data = DynamicArray<char>::make(),
                              .identifier_data_list = DynamicArray<IdentifierData>::make(),
                              .next_identifier_id = 0,
                              .next_string_data_index = 0};
    }
    auto destroy() {
        tokens.destroy();
        data.destroy();
        string_data.destroy();
        identifier_data_list.destroy();
    }
    [[nodiscard]] auto clone() = delete; // FIXME

    auto parse(const stdf::path &dir) -> SpinesDocumentParsingError {
        auto spines_source_exp = get_and_preprocess_file(dir).transform_error(
            [](SpinesDocumentParsingError::Type t) { return SpinesDocumentParsingError{.type = t}; });
        if (!spines_source_exp.has_value()) return spines_source_exp.error();
        spines_source = std::move(spines_source_exp.value());

        handle_lexer(spines_source);

        std::stack<size_t> identifier_stack{};
        short just_after_identifier = 0;
        auto pop_identifier_stack = [&]() {
            if (identifier_stack.empty()) return;

            _assert(identifier_stack.top() < identifier_data_list.len,
                    "identifier_stack.top() should be store index to identifier_data_list which means is in bound.");
            IdentifierOwnedData old_identifier_data = identifier_data_list.get(identifier_stack.top()).value().owned_data;
            size_t old_data_len = old_identifier_data.data_len;
            size_t old_parent_len = old_identifier_data.parent_len;

            identifier_stack.pop();

            if (identifier_stack.empty()) return;

            _assert(identifier_stack.top() < identifier_data_list.len,
                    "identifier_stack.top() should be store index to identifier_data_list which means is in bound.");
            IdentifierOwnedData *identifier_data = &identifier_data_list.get_ptr(identifier_stack.top())->owned_data;
            identifier_data->data_len += old_data_len;
            identifier_data->parent_len += old_parent_len;
        };
        for (auto token : tokens) {
            switch ((int)token.type) {
            case TokenType::IDENTIFIER: {
                identifier_data_list.append(IdentifierData{
                    .name = {token.index, token.len}, .owned_data = {.point_to = data.len, .data_len = 0, .parent_len = 1}});

                identifier_stack.push(next_identifier_id);
                ++next_identifier_id;

                just_after_identifier = 2; // NOTE: cheat flag by set = 2 then decrease 2 times (because I dont want to somehow
                                           // exit this case without decrease this flag value)
            } break;
            case TokenType::NUMBER: {
                std::string_view strv(spines_source.data() + token.index, token.len);

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
                        return SpinesDocumentParsingError{.type = parsing_error_type_t::INVALID_VALUE,
                                                          .index = token.index,
                                                          .column = token.column,
                                                          .line = token.line};
                    }
                }

                _assert(!identifier_stack.empty(), "identifier should not be empty yet");
                _assert(identifier_stack.top() < identifier_data_list.len,
                        "identifier_stack.top() should be store index to identifier_data_list which means is in bound.");
                ++identifier_data_list.get_ptr(identifier_stack.top())->owned_data.data_len;
                if (just_after_identifier) pop_identifier_stack();
            } break;
            case TokenType::STRING: {
                std::string_view strv(spines_source.data() + token.index, token.len);

                data.append((unsigned int)next_string_data_index);

                for (char c : strv) string_data.append(c); // FIXME gotta do a range append to dynamic array
                string_data.append('\0');

                next_string_data_index += strv.size() + 1;

                _assert(!identifier_stack.empty(), "identifier should not be empty yet");
                _assert(identifier_stack.top() < identifier_data_list.len,
                        "identifier_stack.top() should be store index to identifier_data_list which means is in bound.");
                ++identifier_data_list.get_ptr(identifier_stack.top())->owned_data.data_len;
                if (just_after_identifier) pop_identifier_stack();
            } break;
            case '{': {
            } break;
            case '}': {
                pop_identifier_stack();
            } break;
            default: {
                return SpinesDocumentParsingError{.type = parsing_error_type_t::INVALID_SYNTAX,
                                                  .index = token.index,
                                                  .column = token.column,
                                                  .line = token.line};

            } break;
            }

            if (just_after_identifier) --just_after_identifier;
        }

        return SpinesDocumentParsingError{};
    }

private:
    auto get_and_preprocess_file(const stdf::path &dir) noexcept
        -> std::expected<std::string, SpinesDocumentParsingError::Type> {
        stdf::path full_dir = _root_dir / dir;

        std::error_code ec;
        auto size = stdf::file_size(full_dir, ec);
        switch (ec.value()) {
        case (int)std::errc::is_a_directory:
        case (int)std::errc::no_such_file_or_directory:
            return std::unexpected{parsing_error_type_t::FILE_NOT_EXISTED};
            break;
        case (int)std::errc::permission_denied:
            return std::unexpected{parsing_error_type_t::FILE_PERMISSTION_DENIED};
            break;
        case (int)std::errc::io_error:
            return std::unexpected{parsing_error_type_t::FILE_IO_ERROR};
            break;
        default:
            return std::unexpected{parsing_error_type_t::FILE_UNKNOWN_ERROR};
            break;
        }

        std::ifstream file(full_dir, std::ios::in | std::ios::binary);
        if (!file) return std::unexpected{parsing_error_type_t::FILE_IO_ERROR};

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

    auto handle_lexer(std::string_view content_view) noexcept -> SpinesDocumentParsingError {
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
                    return SpinesDocumentParsingError{.type = parsing_error_type_t::INVALID_IDENTIFIER_NAME,
                                                      .index = cur_token.index,
                                                      .column = cur_token.column,
                                                      .line = cur_token.line};
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
                        return SpinesDocumentParsingError{.type = parsing_error_type_t::INVALID_VALUE,
                                                          .index = cur_token.index,
                                                          .column = cur_token.column,
                                                          .line = cur_token.line};
                    }

                    state = ReadState::AFTER_VALUE;
                } break;
                }
            } break;

            case ReadState::AFTER_IDENTIFIER: {
                if (content_view.front() != '=' && content_view.front() != '{') {
                    return SpinesDocumentParsingError{.type = parsing_error_type_t::INVALID_SYNTAX,
                                                      .index = cur_token.index,
                                                      .column = cur_token.column,
                                                      .line = cur_token.line};
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

        return SpinesDocumentParsingError{};
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

public:
    struct Accessor {
        SpinesDocument const *doc_ref = nullptr;
        size_t current_id = 0;
        std::optional<size_t> target_data_index = {}; // has value -> in a identifier, no value -> in a access value

        [[nodiscard]] auto pick(std::string_view search_name) const noexcept -> Accessor {
            _assert(doc_ref != nullptr, "Document reference is null.");
            _assert(!target_data_index.has_value(), "Cannot call .pick(name) on a data value.");
            _assert(current_id < doc_ref->identifier_data_list.len, "current_id is out of bounds.");

            auto current_data = doc_ref->identifier_data_list.get(current_id).value();

            size_t search_start = current_id + 1;
            size_t search_end = current_id + current_data.owned_data.parent_len;

            _assert(search_end <= doc_ref->identifier_data_list.len, "Search range exceeds identifier_data_list bounds.");

            for (size_t i = search_start; i < search_end; ++i) {
                auto child_data = doc_ref->identifier_data_list.get(i).value();

                _assert(child_data.name.offset_root + child_data.name.len <= doc_ref->spines_source.size(),
                        "String view out of bounds of spines_source.");

                std::string_view child_name{doc_ref->spines_source.data() + child_data.name.offset_root, child_data.name.len};

                if (child_name == search_name) { return Accessor{doc_ref, i, {}}; }
            }

            // std::println("Error: Child identifier '{}' not found in this scope.", search_name);
            _assert(false, "Child identifier not found.");
            return *this;
        }

        [[nodiscard]] auto pick(size_t index) const noexcept -> Accessor {
            _assert(doc_ref != nullptr, "Document reference is null.");
            _assert(!target_data_index.has_value(), "Cannot chain .pick(index) on a data value.");
            _assert(current_id < doc_ref->identifier_data_list.len, "current_id is out of bounds.");

            auto current_data = doc_ref->identifier_data_list.get(current_id).value();
            _assert(index < current_data.owned_data.data_len, "Data index out of bounds for this identifier.");

            _assert(current_data.owned_data.point_to + index < doc_ref->data.len,
                    "Calculated data index out of bounds of data container.");

            return Accessor{doc_ref, current_id, current_data.owned_data.point_to + index};
        }

        template <typename T> [[nodiscard]] auto as() const noexcept -> std::optional<T> {
            _assert(doc_ref != nullptr, "Document reference is null.");
            _assert(target_data_index.has_value(), "Accessor points to an identifier, not a value. Call .pick(index) first.");
            if (!target_data_index.has_value()) return {}; // TODO error handle

            _assert(target_data_index.value() < doc_ref->data.len, "Target data index should be in bound");
            auto val = doc_ref->data.get(target_data_index.value()).value();

            if constexpr (std::is_same_v<T, std::string_view> || std::is_same_v<T, std::string>) {
                _assert(std::holds_alternative<unsigned int>(val), "Requested string, but data is not a string index.");
                if (!std::holds_alternative<unsigned int>(val)) return {}; // TODO error handle

                unsigned int str_idx = std::get<unsigned int>(val);
                _assert(str_idx < doc_ref->string_data.len, "String index out of bounds of string_data container.");

                return std::string_view(doc_ref->string_data.get_ptr(str_idx));

            } else if constexpr (std::is_same_v<T, int>) {
                _assert(std::holds_alternative<int>(val), "Requested int, but data is not an int.");
                if (!std::holds_alternative<int>(val)) return {}; // TODO error handle

                return std::get<int>(val);

            } else if constexpr (std::is_same_v<T, float>) {
                _assert(std::holds_alternative<float>(val), "Requested float, but data is not a float.");
                if (!std::holds_alternative<float>(val)) return {}; // TODO error handle

                return std::get<float>(val);

            } else {
                static_assert(sizeof(T) == 0, "Unsupported type cast for .as<T>()");
            }
        }
    };

    [[nodiscard]] auto pick(std::string_view root_name) const noexcept -> Accessor {
        for (size_t i = 0; i < identifier_data_list.len; i += identifier_data_list.get(i).value().owned_data.parent_len) {
            auto data = identifier_data_list.get(i).value();
            std::string_view name{spines_source.data() + data.name.offset_root, data.name.len};
            if (name == root_name) return Accessor{this, i, {}};
        }
        // std::println("Error: Root identifier '{}' not found.", root_name);
        _assert(false, "Root identifier not found.");
        return Accessor{this, 0, {}};
    }
};

} // namespace cactus
