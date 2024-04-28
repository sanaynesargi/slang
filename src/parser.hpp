#pragma once

#include <optional>
#include "tokenization.hpp"

// Node definitions from grammar ../docs/grammar.md

// for simplicity- an expression is limited to an integer
struct NodeExpr {
    Token int_lit;
};

struct NodeExit {
    NodeExpr expr;
};


class Parser {
public:
    inline explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)) {

    }

    // each production will be a method that returns an optional node described in the grammar

    std::optional<NodeExpr> parse_expr() {

        if (peak().has_value() && peak().value().type == TokenType::int_lit) {
            return NodeExpr{.int_lit = consume()};
        }
        else {
            return {};
        }
    }

    std::optional<NodeExit> parse()  {
        std::optional<NodeExit> exit_node;

        while (peak().has_value()) {
            if (peak().value().type == TokenType::exit) {
                consume(); // consume exit
                // optional syntax allows for optional to have a value
                if (auto node_expr = parse_expr()) {
                    exit_node = NodeExit{.expr = node_expr.value()}; // set exit node
                } else {
                    std::cerr << "Invalid expression" << std::endl;
                    exit(EXIT_FAILURE);
                }

                // verify semicolon is after
                if (peak().has_value() && peak().value().type == TokenType::semi) {
                    consume(); // consume semicolon
                } else {
                    std::cerr << "Invalid expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
        }

        m_index = 0;
        return exit_node;
    }

private:
    // peek ahead optional for the case where char is eof
    [[nodiscard]] inline std::optional<Token> peak(int ahead = 1) const {
        // peak ahead to make sure there is no eof
        if (m_index + ahead > m_tokens.size()) {
            return {};
        } else {
            return m_tokens.at(m_index);
        }
    }

    inline Token consume () {
        return m_tokens.at(m_index++);
    }

    const std::vector<Token> m_tokens;
    size_t m_index = 0;
};