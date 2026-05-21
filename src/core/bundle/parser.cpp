module;

#include <cassert>

export module cactus.core.bundle:parser;

import cactus.common;
import :token;

namespace cactus {

export struct ParserLocation {
    size_t index = 0;
    size_t column = 1;
    size_t line = 1;

    auto advance_horizontal(size_t v) {
        index += v;
        column += v;
    }
};

export struct Parser {
    std::vector<std::pair<ParserLocation, Token>> tokens;

    auto parse(const stdf::path &dir) {
        std::string content = get_and_preprocess_file(dir).value(); // TODO error
        std::println("string:\n{}\n", content);

        handle_lexer(content);

        for (auto p : tokens) {
            std::println("index {} at {}:{} : token = {}; len = {} >> {}", p.first.index, p.first.line, p.first.column,
                         (size_t)p.second.type, p.second.len, std::string_view(content.data() + p.first.index, p.second.len));
        }
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
            AFTER_IDENTIFIER,
            EXPECT_VALUE,
            AFTER_VALUE,
            AFTER_SEPARATOR,
            AFTER_ARRAY,
        } state = ReadState::EXPECT_IDENTIFIER;

        ParserLocation cur_location;

        while (true) {
            if (content_view.empty()) break;
            if (content_view.front() == '\n' || content_view.front() == '\t' || content_view.front() == ' ') {
                handle_remove_spaces(&content_view, &cur_location);
                if (content_view.empty()) break;
            }

            if (state == ReadState::EXPECT_IDENTIFIER) {
                if (might_be_value(content_view.front())) {
                    assert(false);
                    return false; // TODO error
                }

                size_t len = get_identifier_len(content_view);
                tokens.push_back({cur_location, Token{.type = TokenType::IDENTIFIER, .len = len}});
                cur_location.advance_horizontal(len);
                content_view.remove_prefix(len);

                state = ReadState::AFTER_IDENTIFIER;
                continue;
            } else if (state == ReadState::AFTER_IDENTIFIER) {
                if (content_view.front() != '=' && content_view.front() != '{') {
                    assert(false);
                    return false; // TODO error
                }

                // the "{" will be handle later
                if (content_view.front() == '=') {
                    cur_location.advance_horizontal(1);
                    content_view.remove_prefix(1);
                }

                state = ReadState::EXPECT_VALUE;
                continue;
            } else if (state == ReadState::EXPECT_VALUE) {
                if (content_view.front() == '{') {
                    tokens.push_back({cur_location, Token{.type = (TokenType)content_view.front(), .len = 1}});
                    cur_location.advance_horizontal(1);
                    content_view.remove_prefix(1);
                } else if (content_view.front() == '\"') {
                    cur_location.advance_horizontal(1);
                    content_view.remove_prefix(1);

                    size_t end_string_i = content_view.find_first_of("\"");
                    if (end_string_i != 0)
                        tokens.push_back({cur_location, Token{.type = TokenType::STRING, .len = end_string_i}});
                    cur_location.advance_horizontal(end_string_i + 1);
                    content_view.remove_prefix(end_string_i + 1);
                } else if (might_be_value(content_view.front())) {
                    size_t num_len = content_view.find_first_of(" \t\n,");
                    tokens.push_back({cur_location, Token{.type = TokenType::NUMBER, .len = num_len}});
                    cur_location.advance_horizontal(num_len);
                    content_view.remove_prefix(num_len);
                } else {
                    assert(false);
                    return false; // TODO error
                }

                state = ReadState::AFTER_VALUE;
                continue;
            } else if (state == ReadState::AFTER_VALUE) {
                if (content_view.front() == ',') {
                    cur_location.advance_horizontal(1);
                    content_view.remove_prefix(1);

                    state = ReadState::AFTER_SEPARATOR;
                } else if (content_view.front() == '}') {
                    tokens.push_back({cur_location, Token{.type = (TokenType)content_view.front(), .len = 1}});
                    cur_location.advance_horizontal(1);
                    content_view.remove_prefix(1);

                    state = ReadState::AFTER_ARRAY;
                } else {
                    state = ReadState::EXPECT_IDENTIFIER;
                }
            } else if (state == ReadState::AFTER_SEPARATOR) {
                if (might_be_value(content_view.front())) {
                    state = ReadState::EXPECT_VALUE;
                } else {
                    state = ReadState::EXPECT_IDENTIFIER;
                }
            } else if (state == ReadState::AFTER_ARRAY) {
                if (content_view.front() == ',') {
                    cur_location.advance_horizontal(1);
                    content_view.remove_prefix(1);

                    state = ReadState::AFTER_SEPARATOR;
                } else {
                    state = ReadState::EXPECT_IDENTIFIER;
                }
            }
        }

        return true;
    }

    auto handle_remove_spaces(std::string_view *content_view, ParserLocation *location) -> void {
        constexpr size_t TAB_WIDTH = 4;

        size_t removed = 0;
        for (auto c : *content_view) {
            if (c == '\n') {
                ++location->line;
                location->column = 1;
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
