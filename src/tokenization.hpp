#pragma once

#include <string>
#include <vector>

// create tokens as enum
enum class TokenType {
    exit,
    int_lit,
    semi,
    open_paren,
    close_paren,
    ident,
    def,
    eq,
    plus,
    star,
    minus,
    fslash,
    open_curly,
    close_curly,
    if_,
};

// create token definition
struct Token {
    TokenType type;
    std::optional<std::string> value {}; // optional value (e.g. 5 for int_lit)
};


// helper func to determine operator precedence
std::optional<int> bin_prec(TokenType type) {
    switch (type) {
        case TokenType::minus:
        case TokenType::plus:
            return 0;
        case TokenType::fslash:
        case TokenType::star:
            return 1;
        default:
            return {}; // if type not an operator
    }
}


class Tokenizer {
public:

    // move src code to member variable
    inline explicit Tokenizer(std::string src)
        : m_src(std::move(src))
    {
    }

    inline std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        std::string buf;

        while (peak().has_value()) {
            if (std::isalpha(peak().value())) {
                // consume first alpha
                buf.push_back(consume());

                // while ,alphanumeric we keep looking
                while (peak().has_value() && std::isalnum(peak().value())) {
                    buf.push_back(consume());
                }

                // keyword check
                if (buf == "exit") {
                    tokens.push_back({.type = TokenType::exit});
                    buf.clear(); // clear current token buffer
                } else if (buf == "def") {
                    tokens.push_back({.type = TokenType::def});
                    buf.clear();
                } else if (buf == "if") {
                    tokens.push_back({.type = TokenType::if_});
                    buf.clear();
                } else {
                    // instead of err, if not keyword -> create identifier
                   tokens.push_back({.type = TokenType::ident, .value = buf});
                   buf.clear();
                }
            } else if (std::isdigit(peak().value())) {
                buf.push_back(consume());

                while (peak().has_value() && std::isdigit(peak().value())) {
                    buf.push_back(consume());
                }

                tokens.push_back({.type = TokenType::int_lit, .value = buf});
                buf.clear();
            } else if (peak().value() == '(') {
                consume();
                tokens.push_back({.type = TokenType::open_paren});
            } else if (peak().value() == ')') {
                consume();
                tokens.push_back({.type = TokenType::close_paren});
            } else if (peak().value() == ';') {
                consume(); // consume even if value is insignificant
                tokens.push_back({.type = TokenType::semi});
            } else if (peak().value() == '=') {
                consume();
                tokens.push_back({.type = TokenType::eq});
            } else if (peak().value() == '+') {
                consume();
                tokens.push_back({.type = TokenType::plus});
            } else if (peak().value() == '*') {
                consume();
                tokens.push_back({.type = TokenType::star});
            } else if (peak().value() == '-') {
                consume();
                tokens.push_back({.type = TokenType::minus});
            } else if (peak().value() == '/') {
                consume();
                tokens.push_back({.type = TokenType::fslash});
            } else if (peak().value() == '{') {
                consume();
                tokens.push_back({.type = TokenType::open_curly});
            } else if (peak().value() == '}') {
                consume();
                tokens.push_back({.type = TokenType::close_curly});
            }  else if (std::isspace(peak().value())) {
                consume();
                continue;
            } else {
                std::cerr << "Messed Up" << std::endl;
            }
        }

        m_index = 0; // reset index in case we want to tokenize again

        return tokens;
    }


private:

    // peek ahead optional for the case where char is eof
    [[nodiscard]] inline std::optional<char> peak(int offset = 0) const {
        // peak ahead to make sure there is no eof
        if (m_index + offset >= m_src.length()) {
            return {};
        } else {
            return m_src.at(m_index + offset);
        }
    }

    inline char consume () {
        return m_src.at(m_index++);
    }

    const std::string m_src;
    size_t m_index{};
};