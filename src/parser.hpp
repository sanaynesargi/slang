#pragma once

#include <optional>
#include <variant>

#include "tokenization.hpp"

// Node definitions from grammar ../docs/grammar.md

struct NodeExprIntLit {
    Token int_lit;
};

struct NodeExprIdent {
    Token ident;
};

struct NodeExpr {
    std::variant<NodeExprIntLit, NodeExprIdent> var;
};

struct NodeStmtExit {
    NodeExpr expr;
};

struct NodeStmtDef {
    Token ident;
    NodeExpr expr;
};

struct NodeStmt {
    std::variant<NodeStmtExit, NodeStmtDef> var;
};

struct NodeProg {
    std::vector<NodeStmt> stmts;
};


class Parser {
public:
    inline explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)) {

    }

    // each production will be a method that returns an optional node described in the grammar

    std::optional<NodeExpr> parse_expr() {

        if (peak().has_value() && peak().value().type == TokenType::int_lit) {
            return NodeExpr{.var = NodeExprIntLit{.int_lit = consume()}};
        } else if (peak().has_value() && peak().value().type == TokenType::ident) {
            return NodeExpr{.var = NodeExprIdent{.ident = consume()}};
        }
        else {
            return {};
        }
    }

    std::optional<NodeStmt> parse_stmt()  {
        while (peak().has_value()) {
            if (peak().value().type == TokenType::exit && peak(1).has_value()
                && peak(1).value().type == TokenType::open_paren) {
                consume(); // consume exit
                consume(); // consume (

                NodeStmtExit stmt_exit;

                // optional syntax allows for optional to have a value
                if (auto node_expr = parse_expr()) {
                    stmt_exit = NodeStmtExit{.expr = node_expr.value()};
                } else {
                    std::cerr << "Invalid expression" << std::endl;
                    exit(EXIT_FAILURE);
                }

                // check for (
                if (peak().has_value() && peak().value().type == TokenType::close_paren) {
                    consume();
                } else {
                    std::cerr << "Expected )" << std::endl;
                    exit(EXIT_FAILURE);
                }
                // verify semicolon is after
                if (peak().has_value() && peak().value().type == TokenType::semi) {
                    consume(); // consume semicolon
                } else {
                    std::cerr << "Expected ;" << std::endl;
                    exit(EXIT_FAILURE);
                }

                return NodeStmt{.var = stmt_exit};
            } else if (peak().has_value() && peak().value().type == TokenType::def
                       && peak(1).has_value() && peak(1).value().type == TokenType::ident
                       && peak(2).has_value() && peak(2).value().type == TokenType::eq) {
                consume(); // def
                auto stmt_def = NodeStmtDef{.ident = consume()};
                consume(); // eq

                if (auto expr = parse_expr()) {
                    stmt_def.expr = expr.value();
                } else {
                    std::cerr << "Invalid Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }

                // check for semicolon
                if (peak().has_value() && peak().value().type == TokenType::semi) {
                    consume();
                } else {
                    std::cerr << "Expected ;" << std::endl;
                    exit(EXIT_FAILURE);
                }

                return NodeStmt{.var = stmt_def};
            } else {
                return {};
            }
        }
    }

    std::optional<NodeProg> parse_prog() {
        NodeProg prog; // create root node (program)

        while (peak().has_value()) {
            if (auto stmt = parse_stmt()) {
                prog.stmts.push_back(stmt.value());
            } else {
                std::cerr << "Invalid Statement" << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        return prog;
    }

private:
    // peek ahead optional for the case where char is eof
    [[nodiscard]] inline std::optional<Token> peak(int offset = 0) const {
        // peak ahead to make sure there is no eof
        if (m_index + offset >= m_tokens.size()) {
            return {};
        } else {
            return m_tokens.at(m_index + offset);
        }
    }

    inline Token consume () {
        return m_tokens.at(m_index++);
    }

    const std::vector<Token> m_tokens;
    size_t m_index = 0;
};