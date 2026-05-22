module;

export module cactus.core.bundle:token;

import cactus.common;

namespace cactus {

export enum TokenType : int { IDENTIFIER = 256, NUMBER, STRING };

export struct Token {
    TokenType type;
    size_t len;
};

} // namespace cactus
