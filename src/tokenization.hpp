#pragma once

#include <string>
#include <vector>

// create tokens as enum
enum class TokenType {
    exit,
    int_lit,
    semi
};

// create token definition
struct Token {
    TokenType type;
    std::optional<std::string> value {}; // optional value (e.g. 5 for int_lit)
};


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

                // while alnum we keep looking
                while (peak().has_value() && std::isalnum(peak().value())) {
                    buf.push_back(consume());
                }

                if (buf == "exit") {
                    tokens.push_back({.type = TokenType::exit});
                    buf.clear(); // clear current token buffer
                } else {
                    std::cerr << "Mess Up!" << std::endl;
                }
            } else if (std::isdigit(peak().value())) {
                buf.push_back(consume());

                while (peak().has_value() && std::isdigit(peak().value())) {
                    buf.push_back(consume());
                }

                tokens.push_back({.type = TokenType::int_lit, .value = buf});
                buf.clear();
            } else if (peak().value() == ';') {
                consume(); // consume even if value is insignificant
                tokens.push_back({.type = TokenType::semi});
            } else if (std::isspace(peak().value())) {
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
    [[nodiscard]] std::optional<char> peak(int ahead = 1) const {
        // peak ahead to make sure there is no eof
        if (m_index + ahead > m_src.length()) {
            return {};
        } else {
            return m_src.at(m_index);
        }
    }

    char consume () {
        return m_src.at(m_index++);
    }

    const std::string m_src;
    int m_index{};
};