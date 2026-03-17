module;

#include <algorithm>
#include <boost/container/vector.hpp>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <stack>
#include <string>
#include <string_view>
#include <tuplet/tuple.hpp>

export module Parser;

namespace bstc = boost::container;
namespace cactus {
struct DocumentHeader {
    size_t id;
};

enum ValueType { BOOL, INT, FLOAT, CHAR, STRING };
using ValueVariantType = std::variant<bool, int, float, char>;
using ValuePtrVariantType = std::variant<bool *, int *, float *, char *>;
constexpr std::array value_type_size = {sizeof(bool), sizeof(int), sizeof(float), sizeof(char)};
constexpr std::array value_type_alignment = {alignof(bool), alignof(int), alignof(float),
                                             alignof(char)};

constexpr size_t max_val =
    *std::max_element(std::begin(value_type_alignment), std::end(value_type_alignment));

struct Token {
    enum { UNSPECIFIED, HEADER, NAME, VALUE, ARRAY_BEGIN, ARRAY_END } type;
    size_t string_size;
    size_t tag;
};

export struct Document {
    struct Node { // FIXME
        std::optional<std::string> key = {};
        size_t index;
        size_t data_offset;
    };

    bstc::vector<tuplet::tuple<size_t, ValueType>> data_offset_and_type;
    void *data;

    ~Document() {
        free(data);
    }

    [[nodiscard]] auto get(size_t index) -> std::optional<ValuePtrVariantType> {
        if (index >= data_offset_and_type.size()) return {};
        auto type = tuplet::get<1>(data_offset_and_type[index]);
        auto offset = tuplet::get<0>(data_offset_and_type[index]);
        switch (type) {
        case BOOL: return ValuePtrVariantType{(bool *)((std::byte *)data + offset)};
        case INT: return ValuePtrVariantType{(int *)((std::byte *)data + offset)};
        case FLOAT: return ValuePtrVariantType{(float *)((std::byte *)data + offset)};
        case CHAR: return ValuePtrVariantType{(char *)((std::byte *)data + offset)};
        default: return {};
        }
    }

    auto parse(const char *spines) {
        auto processed_spines = preprocess_spines(spines);

        bstc::vector<Token> tokens;
        bstc::vector<size_t> header_sizes;

        handle_verify_and_tokenization(processed_spines, &tokens, &header_sizes);

        size_t total_data_size = 0;
        for (auto &token : tokens) {
            if (token.type == Token::VALUE) {
                if (token.tag == ValueType::STRING) {
                    // total_data_size += token.string_size;
                } else {
                    auto size = value_type_size[token.tag];
                    auto alignment = value_type_alignment[token.tag];

                    total_data_size = (total_data_size + alignment - 1) & ~(alignment - 1);
                    total_data_size += size;
                }
            }
        }

        data = aligned_alloc(max_val, total_data_size);

        std::string log{};
        size_t cur_data_offset = 0;
        std::string_view spines_view{processed_spines};
        for (auto &token : tokens) {
            switch (token.type) {
            case Token::VALUE: {
                auto size = value_type_size[token.tag];
                auto alignment = value_type_alignment[token.tag];

                cur_data_offset = (cur_data_offset + alignment - 1) & ~(alignment - 1);

                switch (token.tag) {
                case ValueType::BOOL: {
                    *(bool *)((std::byte *)data + cur_data_offset) =
                        spines_view.front() == 't' ? 1 : 0;
                } break;
                case ValueType::INT: {
                    int val;
                    std::from_chars(spines_view.begin(), spines_view.begin() + token.string_size,
                                    val);
                    *(int *)((std::byte *)data + cur_data_offset) = val;
                } break;
                case ValueType::FLOAT: {
                    float val;
                    std::from_chars(spines_view.begin(), spines_view.begin() + token.string_size,
                                    val);
                    *(float *)((std::byte *)data + cur_data_offset) = val;
                } break;
                case ValueType::CHAR: break;
                case ValueType::STRING: break;
                }

                data_offset_and_type.emplace_back(
                    tuplet::make_tuple(cur_data_offset, (ValueType)token.tag));

                cur_data_offset += size;
            } break;
            default: break;
            }

            spines_view.remove_prefix(token.string_size);
        }
    }

    auto preprocess_spines(const char *spines) -> std::string {
        std::string res;
        auto *c = spines;
        bool is_comment = false;
        for (; *c != '\0'; ++c) {
            if (is_comment) {
                if (*c == '\n') is_comment = false;
                continue;
            }
            if (std::string_view{" \t\n\r"}.contains(*c)) continue;
            if (*c == '#') {
                is_comment = true;
                continue;
            }
            res.push_back(*c);
        }
        return res;
    }

    auto handle_verify_and_tokenization(std::string_view strv, bstc::vector<Token> *tokens,
                                        bstc::vector<size_t> *header_sizes) -> void {
        // FIXME: add error check on dup name; inconsist array var type
        enum struct ReadState { HEADER, NAME, ASSIGN, VALUE, ARRAY } state = ReadState::HEADER;
        std::stack<tuplet::tuple<size_t, size_t>> pre_levels;
        while (true) {
            if (strv.empty()) break;

            switch (state) {
            case ReadState::HEADER: {
                if (strv.front() != ':') {
                    assert(false); // TODO
                    return;
                }
                size_t header_level = strv.find_first_not_of(":");
                if (header_level == strv.npos) {
                    assert(false); // TODO
                    return;
                }

                if (pre_levels.empty()) {
                    if (header_level > 1) {
                        assert(false); // TODO
                        return;
                    }
                } else if (header_level > tuplet::get<1>(pre_levels.top()) + 1) {
                    assert(false); // TODO
                    return;
                }

                header_sizes->emplace_back(0);

                while (!pre_levels.empty() && tuplet::get<1>(pre_levels.top()) >= header_level)
                    pre_levels.pop();

                if (!pre_levels.empty()) {
                    assert(tuplet::get<0>(pre_levels.top()) < header_sizes->size() - 1);
                    ++(*header_sizes)[tuplet::get<0>(pre_levels.top())];
                }

                pre_levels.push({header_sizes->size() - 1, header_level});

                state = ReadState::NAME;

                strv.remove_prefix(header_level);
                tokens->emplace_back(Token::HEADER, header_level, header_sizes->size() - 1);
            } break;

            case ReadState::NAME: {
                size_t i = strv.find_first_of("!:");
                if (i == strv.npos) {
                    return; // no need error
                }
                if (!is_valid_name(strv.substr(0, i))) {
                    assert(false); // TODO
                    return;
                }

                strv.remove_prefix(i);
                tokens->emplace_back(Token::NAME, i);

                if (strv.front() == '!') {
                    state = ReadState::ASSIGN;
                    strv.remove_prefix(1);
                    tokens->emplace_back(Token::UNSPECIFIED, 1);
                }
                if (strv.front() == ':') {
                    state = ReadState::HEADER;
                }
            } break;

            case ReadState::ASSIGN: {
                if (strv.front() == ':') {
                    state = ReadState::HEADER;
                } else if (strv.front() == ',') {
                    strv.remove_prefix(1);
                    tokens->emplace_back(Token::UNSPECIFIED, 1);
                } else if (strv.front() == '[' || strv.front() == ']') {
                    state = ReadState::ARRAY;
                } else if (std::isdigit(strv.front())
                           || std::string_view{"tf.+-"}.contains(strv.front())) {
                    state = ReadState::VALUE;
                } else {
                    assert(false); // TODO
                    return;
                }
            } break;

            case ReadState::VALUE: {
                switch (strv.front()) {
                case '\'': {
                    auto end = strv.find_first_of("\'");
                    if (end > 2 || end == strv.npos) {
                        assert(false); // TODO
                        return;
                    }
                    auto inner = strv.substr(1, end);
                    if (!(inner.size() == 1 && inner.front() != '\\')
                        && !(inner.size() == 2 && inner.front() == '\\')) {
                        assert(false); // TODO
                        return;
                    }

                    strv.remove_prefix(end + 1);
                    tokens->emplace_back(Token::UNSPECIFIED, 1);
                    tokens->emplace_back(Token::VALUE, end - 1, ValueType::CHAR);
                    tokens->emplace_back(Token::UNSPECIFIED, 1);
                } break;
                case '"': {
                    auto end = strv.find_first_of("\"");
                    if (end == strv.npos) {
                        assert(false); // TODO
                        return;
                    }

                    // string

                    strv.remove_prefix(end + 1);
                    tokens->emplace_back(Token::UNSPECIFIED, 1);
                    tokens->emplace_back(Token::VALUE, end - 1, ValueType::STRING);
                    tokens->emplace_back(Token::UNSPECIFIED, 1);
                } break;
                default: {
                    if (strv.size() >= 4 && strv[0] == 't' && strv[1] == 'r' && strv[2] == 'u'
                        && strv[3] == 'e') {
                        // true
                        strv.remove_prefix(4);
                        tokens->emplace_back(Token::VALUE, 4, ValueType::BOOL);

                        break;
                    }
                    if (strv.size() >= 5 && strv[0] == 'f' && strv[1] == 'a' && strv[2] == 'l'
                        && strv[3] == 's' && strv[4] == 'e') {
                        // false
                        strv.remove_prefix(5);
                        tokens->emplace_back(Token::VALUE, 5, ValueType::BOOL);

                        break;
                    }

                    bool has_sign = false;
                    if (strv.front() == '+' || strv.front() == '-') {
                        strv.remove_prefix(1);
                        has_sign = true;
                    }
                    if (strv.empty()) {
                        assert(false); // TODO
                        return;
                    }
                    std::string_view float_suffix{"fF"};
                    bool has_digit = false, has_dot = false;
                    size_t end = 0;
                    for (auto c : strv) {
                        if (std::isdigit(c)) {
                            has_digit = true;
                        } else if (c == '.') {
                            if (has_dot) {
                                assert(false); // TODO
                                return;
                            }
                            has_dot = true;
                        } else {
                            break;
                        }

                        ++end;
                    }
                    if (end == 0) {
                        assert(false); // TODO
                        return;
                    }

                    if (end < strv.size() - 1 && float_suffix.contains(strv[end])) {
                        strv.remove_prefix(end);
                        tokens->emplace_back(Token::VALUE, end + has_sign, ValueType::FLOAT);
                        break;
                    }
                    if (has_dot) {
                        strv.remove_prefix(end);
                        tokens->emplace_back(Token::VALUE, end + has_sign, ValueType::FLOAT);
                        break;
                    }

                    strv.remove_prefix(end);
                    tokens->emplace_back(Token::VALUE, end + has_sign, ValueType::INT);
                } break;
                }

                state = ReadState::ASSIGN;
            } break;

            case ReadState::ARRAY: {
                if (strv.front() == '[') {
                    strv.remove_prefix(1);
                    tokens->emplace_back(Token::ARRAY_BEGIN, 1);
                    state = ReadState::ASSIGN;
                } else if (strv.front() == ']') {
                    strv.remove_prefix(1);
                    tokens->emplace_back(Token::ARRAY_END, 1);
                    state = ReadState::ASSIGN;
                } else {
                    assert(false); // TODO
                    return;
                }
            } break;
            }
        }
    }

    auto is_valid_name(std::string_view strv) -> bool {
        if (strv.empty()) return false;
        return std::ranges::all_of(strv,
                                   [](unsigned char c) { return std::isalnum(c) || c == '_'; });
    }
};

} // namespace cactus
