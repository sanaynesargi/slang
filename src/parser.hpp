#pragma once

#include <optional>
#include <variant>

#include "tokenization.hpp"
#include "arena.hpp"

// Node definitions from grammar ../docs/grammar.md
// var stands for "variant" as in variable of global type to more specific type
// type in this context represents production (grammar)

struct NodeTermIntLit {
    Token int_lit;
};

struct NodeTermIdent {
    Token ident;
};

struct NodeExpr;

struct NodeBinExprAdd {
    NodeExpr* lhs{};
    NodeExpr* rhs{};
};

//struct NodeBinExprMult {
//    NodeExpr* lhs;
//    NodeExpr* rhs;
//};

struct NodeBinExpr {
//    std::variant<NodeBinExprAdd*, NodeBinExprMult*> var;
    NodeBinExprAdd* add;
};

// term to hold either in int lit or an identifier
// to allow for handing of binary expressions
struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*> var;
};

struct NodeExpr {
    std::variant<NodeTerm*, NodeBinExpr*> var;
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

    // parse each term (e.g. int literal or identifier)
    std::optional<NodeTerm*> parse_term() {
        if (auto int_lit = try_consume(TokenType::int_lit)) {
            auto term_int_lit = m_allocator.alloc<NodeTermIntLit>(); // allocate int lit
            term_int_lit->int_lit = int_lit.value(); // set int lit to consumed token
            auto term = m_allocator.alloc<NodeTerm>(); // allocate expression
            term->var = term_int_lit; // set expression var to int lit
            return term;
        } else if (auto ident = try_consume(TokenType::ident)) {
            // same process as above for all token identifiers
            auto term_ident = m_allocator.alloc<NodeTermIdent>();
            term_ident->ident = ident.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_ident;
            return term;
        } else {
            return {};
        }
    }

    // each production will be a method that returns an optional node described in the grammar
    std::optional<NodeExpr*> parse_expr() {
        if (auto term = parse_term()) {
            // check if next token is a binary operator
            if (try_consume(TokenType::plus).has_value()) {
                // parse left hand side
                auto bin_expr = m_allocator.alloc<NodeBinExpr>();

                // determine type of binary expression (e.g. addition vs multiplication)
                auto bin_expr_add = m_allocator.alloc<NodeBinExprAdd>();

                // create left hand side expression and add term to it
                auto lhs_expr = m_allocator.alloc<NodeExpr>();
                lhs_expr->var = term.value();
                bin_expr_add->lhs = lhs_expr; // set left hand side expression to previous (convert from NodeBinExpr to NodeExpr)


                // same for the right side
                if (auto rhs = parse_expr()) {
                    bin_expr_add->rhs = rhs.value();
                    bin_expr->add = bin_expr_add;
                    auto expr = m_allocator.alloc<NodeExpr>();
                    expr->var = bin_expr;
                    return expr; // add binary expression to full expression with left side
                } else {
                    std::cerr << "Expected Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
            } else {
                // if the expression is not a binary expression, just treat it as an ident or int lit
                auto expr = m_allocator.alloc<NodeExpr>();
                expr->var = term.value();
                return expr;
            }
        } else {
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

                // try to consume )
                try_consume(TokenType::close_paren, "Expected ')'");
                // verify semi is after
                try_consume(TokenType::semi, "Expected ';'");

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
                try_consume(TokenType::semi, "Expected ';'");

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

    // refactor the consume token error into its own function
    inline Token try_consume(TokenType type, const std::string& err_msg) {
        if (peak().has_value() && peak().value().type == type) {
            return consume();
        } else {
            std::cerr << err_msg << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // overload function to return an optional type where no error is needed
    inline std::optional<Token> try_consume(TokenType type) {
        if (peak().has_value() && peak().value().type == type) {
            return consume();
        } else {
            return {};
        }
    }

        const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllocator m_allocator;
};