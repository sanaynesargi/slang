#pragma once

#include <optional>
#include <variant>

#include "tokenization.hpp"
#include "arena.hpp"

// Node definitions from grammar ../docs/grammar.md
// var stands for "variant" as in variable of global type to more specific type
// type in this context represents production (grammar)

struct NodeExprIntLit {
    Token int_lit;
};

struct NodeExprIdent {
    Token ident;
};

struct NodeExpr;

struct NodeBinExprAdd {
    NodeExpr* lhs{};
    NodeExpr* rhs{};
};

struct NodeBinExprMult {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExpr {
    std::variant<NodeBinExprAdd*, NodeBinExprMult*> var;
};

struct NodeExpr {
    std::variant<NodeExprIntLit*, NodeExprIdent*, NodeBinExpr*> var;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtDef {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtDef*> var;
};

struct NodeProg {
    std::vector<NodeStmt*> stmts;
};


class Parser {
public:
    inline explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)),
        m_allocator(1024 * 1024 * 4) // 4mb
    {}

    std::optional<NodeBinExpr*> parse_bin_expr() {
        if (auto lhs = parse_expr()) {
            // parse left hand side
            auto bin_expr = m_allocator.alloc<NodeBinExpr>();

            // determine type of binary expression (e.g. addition vs multiplication)
            if (peak().has_value() && peak().value().type == TokenType::plus) {
                auto bin_expr_add = m_allocator.alloc<NodeBinExprAdd>();
                bin_expr_add->lhs = lhs.value(); // set left hand side
                consume(); // consume plus token

                // same for the right side
                if (auto rhs = parse_expr()) {
                    bin_expr_add->rhs = rhs.value();
                    bin_expr->var = bin_expr_add;
                    return bin_expr;
                } else {
                    std::cerr << "Expected Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
            } else {
                // temporary error message to represent multiplication
                std::cerr << "Unsupported Binary Operator" << std::endl;
                exit(EXIT_FAILURE);
            }
        } else {
            return {};
        }
    }

    // each production will be a method that returns an optional node described in the grammar
    std::optional<NodeExpr*> parse_expr() {

        if (peak().has_value() && peak().value().type == TokenType::int_lit) {
            auto expr_int_lit = m_allocator.alloc<NodeExprIntLit>(); // allocate int lit
            expr_int_lit->int_lit = consume(); // set int lit to consumed token
            auto expr = m_allocator.alloc<NodeExpr>(); // allocate expression
            expr->var = expr_int_lit; // set expression var to int lit
            return expr;
        } else if (peak().has_value() && peak().value().type == TokenType::ident) {
            // same process as above for all token identifiers
            auto expr_ident = m_allocator.alloc<NodeExprIdent>();
            expr_ident->ident = consume();
            auto expr = m_allocator.alloc<NodeExpr>();
            expr->var = expr_ident;
            return expr;
        } else if (auto bin_expr = parse_bin_expr()) {
            // check if we can parse binary expression
            auto expr = m_allocator.alloc<NodeExpr>();
            expr->var = bin_expr.value();
            return expr;
        }
        else {
            return {};
        }
    }

    std::optional<NodeStmt*> parse_stmt()  {
        while (peak().has_value()) {
            if (peak().value().type == TokenType::exit && peak(1).has_value()
                && peak(1).value().type == TokenType::open_paren) {
                consume(); // consume exit
                consume(); // consume (

                auto stmt_exit = m_allocator.alloc<NodeStmtExit>();

                // optional syntax allows for optional to have a value
                if (auto node_expr = parse_expr()) {
                    stmt_exit->expr = node_expr.value();
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

                // allocate Node Statement and set it to exit type
                auto stmt = m_allocator.alloc<NodeStmt>();
                stmt->var = stmt_exit;
                return stmt;
            } else if (peak().has_value() && peak().value().type == TokenType::def
                       && peak(1).has_value() && peak(1).value().type == TokenType::ident
                       && peak(2).has_value() && peak(2).value().type == TokenType::eq) {
                consume(); // def
                auto stmt_def = m_allocator.alloc<NodeStmtDef>();
                stmt_def->ident = consume();
                consume(); // eq

                if (auto expr = parse_expr()) {
                    stmt_def->expr = expr.value();
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

                // same process as above
                auto stmt = m_allocator.alloc<NodeStmt>();
                stmt->var = stmt_def;
                return stmt;
            } else {
                return {};
            }
        }
        return {};
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
    ArenaAllocator m_allocator;
};