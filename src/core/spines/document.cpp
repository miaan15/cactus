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
        FILE_NOT_EXISTED,
        FILE_PERMISSTION_DENIED,
        FILE_IO_ERROR,
        FILE_UNKNOWN_ERROR,
        INVALID_IDENTIFIER_NAME,
        INVALID_VALUE,
        INVALID_SYNTAX,
    } type;

    size_t index = 0;
    size_t column = 1;
    size_t line = 1;
};
using parsing_error_type_t = SpinesDocumentParsingError::Type;

export struct SpinesDocument {
    using data_t = std::variant<int, float, unsigned int>;
    struct IdentifierData {
        size_t name_offset_root;
        size_t name_len;

        size_t data_point_to_index;
        size_t data_len;
        size_t parent_len;
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

    auto parse(const stdf::path &dir) -> std::expected<void, SpinesDocumentParsingError> {
        auto spines_source_exp = get_and_preprocess_file(dir).transform_error(
            [](SpinesDocumentParsingError::Type t) { return SpinesDocumentParsingError{.type = t}; });
        if (!spines_source_exp.has_value()) return std::unexpected{spines_source_exp.error()};
        spines_source = std::move(spines_source_exp.value());

        auto lexer_err = handle_lexer(spines_source);
        if (!lexer_err.has_value()) return lexer_err;

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
                identifier_data_list.append(IdentifierData{
                    .name_offset_root = token.index, 
                    .name_len = token.len,
                    .data_point_to_index = data.len, 
                    .data_len = 0, 
                    .parent_len = 1});

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
                        return std::unexpected{SpinesDocumentParsingError{.type = parsing_error_type_t::INVALID_VALUE,
                                                                          .index = token.index,
                                                                          .column = token.column,
                                                                          .line = token.line}};
                    }
                }

                _assert(!identifier_stack.empty(), "identifier should not be empty yet");
                _assert(identifier_stack.top() < identifier_data_list.len,
                        "identifier_stack.top() should be store index to identifier_data_list which means is in bound.");
                ++identifier_data_list.get_ptr(identifier_stack.top())->data_len;
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
                ++identifier_data_list.get_ptr(identifier_stack.top())->data_len;
                if (just_after_identifier) pop_identifier_stack();
            } break;
            case '{': {
            } break;
            case '}': {
                pop_identifier_stack();
            } break;
            default: {
                return std::unexpected{SpinesDocumentParsingError{.type = parsing_error_type_t::INVALID_SYNTAX,
                                                                  .index = token.index,
                                                                  .column = token.column,
                                                                  .line = token.line}};

            } break;
            }

            if (just_after_identifier) --just_after_identifier;
        }

        return {};
    }

private:
    auto get_and_preprocess_file(const stdf::path &dir) noexcept
        -> std::expected<std::string, SpinesDocumentParsingError::Type> {
        stdf::path full_dir = _root_dir / dir;

        std::error_code ec;
        auto size = stdf::file_size(full_dir, ec);
        if (ec) {
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

    auto handle_lexer(std::string_view content_view) noexcept -> std::expected<void, SpinesDocumentParsingError> {
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
                    return std::unexpected{SpinesDocumentParsingError{.type = parsing_error_type_t::INVALID_IDENTIFIER_NAME,
                                                                      .index = cur_token.index,
                                                                      .column = cur_token.column,
                                                                      .line = cur_token.line}};
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
                        return std::unexpected{SpinesDocumentParsingError{.type = parsing_error_type_t::INVALID_VALUE,
                                                                          .index = cur_token.index,
                                                                          .column = cur_token.column,
                                                                          .line = cur_token.line}};
                    }

                    state = ReadState::AFTER_VALUE;
                } break;
                }
            } break;

            case ReadState::AFTER_IDENTIFIER: {
                if (content_view.front() != '=' && content_view.front() != '{') {
                    return std::unexpected{SpinesDocumentParsingError{.type = parsing_error_type_t::INVALID_SYNTAX,
                                                                      .index = cur_token.index,
                                                                      .column = cur_token.column,
                                                                      .line = cur_token.line}};
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

        return {};
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
        enum struct Error { INVALID_NAME, INDEX_OUT_OF_BOUND, INVALID_OPERATION, NOT_A_VALUE, WRONG_VALUE_TYPE, NOT_SUPPORTED_TYPE };
        std::optional<Error> error{};

        SpinesDocument const *doc_ref = nullptr;

        size_t chain_depth = 0;

        size_t cur_identifier_index = (size_t)-1;
        std::optional<size_t> data_offset{};

        [[nodiscard]] auto pick(std::string_view name) const noexcept -> Accessor {
            if (error.has_value()) return Accessor{error, doc_ref, chain_depth};

            _assert(doc_ref != nullptr, "Document reference is null.");
            _assert(cur_identifier_index < doc_ref->identifier_data_list.len, "cur_identifier_index is out of bounds.");

            IdentifierData cur_identifier_data = doc_ref->identifier_data_list.get(cur_identifier_index).value();

            if (data_offset.has_value()) 
                return Accessor{Error::INVALID_OPERATION, doc_ref, chain_depth + 1};

            size_t search_start = cur_identifier_index + 1;
            size_t search_end = cur_identifier_index + cur_identifier_data.parent_len;
            _assert(search_end <= doc_ref->identifier_data_list.len, "Search range exceeds identifier_data_list bounds.");

            for (size_t i = search_start; i < search_end; ++i) {
                auto child_data = doc_ref->identifier_data_list.get(i).value();

                _assert(child_data.name_offset_root + child_data.name_len <= doc_ref->spines_source.size(),
                        "String view out of bounds of spines_source.");

                std::string_view child_name{doc_ref->spines_source.data() + child_data.name_offset_root, child_data.name_len};
                if (child_name == name)
                    return Accessor{error, doc_ref, chain_depth + 1, i, {}};
            }

            return Accessor{Error::INVALID_NAME, doc_ref, chain_depth + 1};
        }

        [[nodiscard]] auto pick(size_t index) const noexcept -> Accessor {
            if (error.has_value()) return Accessor{error, doc_ref, chain_depth};

            _assert(doc_ref != nullptr, "Document reference is null.");
            _assert(cur_identifier_index < doc_ref->identifier_data_list.len, "cur_identifier_index is out of bounds.");

            IdentifierData cur_identifier_data = doc_ref->identifier_data_list.get(cur_identifier_index).value();

            if (data_offset.has_value()) 
                return Accessor{Error::INVALID_OPERATION, doc_ref, chain_depth + 1};

            if (index >= cur_identifier_data.data_len) {
                return Accessor{Error::INDEX_OUT_OF_BOUND, doc_ref, chain_depth + 1};
            }

            return Accessor{error, doc_ref, chain_depth + 1, cur_identifier_index, index};
        }

        template <typename T> [[nodiscard]] auto as() const noexcept -> std::expected<T, std::pair<Error, size_t>> {
            if (error.has_value()) return std::unexpected{std::make_pair(error.value(), chain_depth)};

            _assert(doc_ref != nullptr, "Document reference is null.");
            _assert(cur_identifier_index < doc_ref->identifier_data_list.len, "cur_identifier_index is out of bounds.");

            IdentifierData cur_identifier_data = doc_ref->identifier_data_list.get(cur_identifier_index).value();

            size_t offset = data_offset.value_or(0);
            if (!data_offset.has_value() && cur_identifier_data.data_len != 0) {
                return std::unexpected{std::make_pair(Error::NOT_A_VALUE, chain_depth + 1)};
            }

            size_t data_index = cur_identifier_data.data_point_to_index + offset;
            _assert(data_index < doc_ref->data.len, "data_index is out of bounds.");

            auto val = doc_ref->data.get(data_index).value();

            if constexpr (std::is_same_v<T, std::string_view> || std::is_same_v<T, std::string>) {
                if (!std::holds_alternative<unsigned int>(val))
                    return std::unexpected{std::make_pair(Error::WRONG_VALUE_TYPE, chain_depth + 1)};

                unsigned int str_idx = std::get<unsigned int>(val);
                _assert(str_idx < doc_ref->string_data.len, "String index out of bounds of string_data container.");

                return std::string_view(doc_ref->string_data.get_ptr(str_idx));

            } else if constexpr (std::is_arithmetic_v<T>) {
                if (!std::holds_alternative<int>(val) && !std::holds_alternative<float>(val))
                    return std::unexpected{std::make_pair(Error::WRONG_VALUE_TYPE, chain_depth + 1)};

                if (std::holds_alternative<int>(val)) return (T)std::get<int>(val);
                if (std::holds_alternative<float>(val)) return (T)std::get<float>(val);

            } else {
                static_assert(false, "Unsupported type cast for .as<T>()");
            }

            return std::unexpected{std::make_pair(Error::WRONG_VALUE_TYPE, chain_depth + 1)};
        }

        [[nodiscard]] auto is_error() const noexcept -> bool {
            return error.has_value();
        }

        [[nodiscard]] auto get_error() const noexcept -> std::optional<std::pair<Error, size_t>> {
            if (!error.has_value()) return {};
            return std::make_pair(error.value(), chain_depth);
        }
    };

    [[nodiscard]] auto pick(std::string_view name) const noexcept -> Accessor {
        for (size_t i = 0; i < identifier_data_list.len;) {
            auto data = identifier_data_list.get(i).value();
            std::string_view cur_name{spines_source.data() + data.name_offset_root, data.name_len};
            if (cur_name == name) return Accessor{{}, this, 0, i, {}};

            i += data.parent_len;
        }

        return Accessor{Accessor::Error::INVALID_NAME, this, 0};
    }
};

} // namespace cactus
