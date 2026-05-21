module;

export module cactus.core.bundle:token;

import cactus.common;

namespace cactus {

export enum struct TokenType { IDENTIFIER = 256, NUMBER, STRING };

export struct Token {
    TokenType type;
    size_t len;
};

} // namespace cactus
