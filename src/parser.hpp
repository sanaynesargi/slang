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

struct NodeTermParen {
    NodeExpr* expr;
};


struct NodeBinExprAdd {
    NodeExpr* lhs{};
    NodeExpr* rhs{};
};

struct NodeBinExprMult {
    NodeExpr* lhs;
    NodeExpr* rhs;
};


struct NodeBinExprSub {
    NodeExpr* lhs{};
    NodeExpr* rhs{};
};

struct NodeBinExprDiv {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExpr {
    std::variant<NodeBinExprAdd*, NodeBinExprMult*, NodeBinExprSub*, NodeBinExprDiv*> var;
};

// term to hold either in int lit or an identifier
// to allow for handing of binary expressions
struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*> var;
};

struct NodeExpr {
    std::variant<NodeTerm*, NodeBinExpr*> var;
};

struct NodeStmt;

struct NodeScope {
    std::vector<NodeStmt*> stmts;
};


struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtDef {
    Token ident;
    NodeExpr* expr{};
};

struct NodeIfPred;

struct NodeIfPredElif {
    NodeExpr* expr{};
    NodeScope* scope{};
    std::optional<NodeIfPred*> pred;
};

struct NodeIfPredElse {
    NodeScope* scope;
};

struct NodeIfPred {
    std::variant<NodeIfPredElif*, NodeIfPredElse*> var;
};

struct NodeStmtIf {
    NodeExpr* expr;
    NodeScope* scope;
    std::optional<NodeIfPred*> pred;
};

// TODO: use using instead of struct
struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtDef*, NodeScope*, NodeStmtIf*> var;
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

    // parse each term (e.g. int literal or identifier or paren expr)
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
        } else if (auto open_paren = try_consume(TokenType::open_paren)) {
            // handle the case of paren by setting resetting prec level inside paren
            auto expr = parse_expr(); // -> this parses the expr (giving the above effect)
            if (!expr.has_value()) {
                std::cerr << "Expected expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            // consume close paren
            try_consume(TokenType::close_paren, "Expected `)`");

            // allocate for paren expr
            auto term_paren = m_allocator.alloc<NodeTermParen>();
            term_paren->expr = expr.value();

            // allocate term to store paren expr
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_paren;

            return term; // return the term
        } else {
            return {};
        }
    }

    // similar to parse_stmt, this utility function pareses the current scope
    std::optional<NodeScope*> parse_scope() {
        if (!try_consume(TokenType::open_curly).has_value()) {
            // needs open paren to start parsing the scope
            return {};
        }


        auto scope = m_allocator.alloc<NodeScope>();
        // while we can parse statements, add them to the scope
        while (auto stmt = parse_stmt()) {
            scope->stmts.push_back(stmt.value());
        }

        // scope should end with a }
        try_consume(TokenType::close_curly, "Expected `}`");

        // according to grammar scope is a statement so treat it as such
        auto stmt = m_allocator.alloc<NodeStmt>();
        stmt->var = scope;

        return scope;
    }

    // parse the optional if predicate (elif or else)
    std::optional<NodeIfPred*> parse_if_pred() {
        if (try_consume(TokenType::elif)) {
            try_consume(TokenType::open_paren, "Expected `(`"); // expect an open paren

            auto elif = m_allocator.alloc<NodeIfPredElif>();

            if (auto expr = parse_expr()) {
                elif->expr = expr.value(); // consume expression elif (expr here)
            } else {
                std::cerr << "Expected Expression" << std::endl;
                exit(EXIT_FAILURE);
            }

            try_consume(TokenType::close_paren, "Expected `)`"); // consume close paren

            // parse the scope
            if (auto scope = parse_scope()) {
                elif->scope = scope.value();
            } else {
                std::cerr << "Expected Scope" << std::endl;
                exit(EXIT_FAILURE);
            }

            // optionally continue with other elifs
            elif->pred = parse_if_pred(); // becomes recursive according to grammar

            auto pred = m_allocator.alloc<NodeIfPred>();
            pred->var = elif;

            return pred;
        }

        // handle else in similar way (no open or close parens)
        if (try_consume(TokenType::else_)) {
            auto else_ = m_allocator.alloc<NodeIfPredElse>();

            // look for the scope
            if (auto scope = parse_scope()) {
                else_->scope = scope.value();
            } else {
                std::cerr << "Expected Scope" << std::endl;
                exit(EXIT_FAILURE);
            }

            auto pred = m_allocator.alloc<NodeIfPred>();
            pred->var = else_;
            return pred;
        }

        return {};
    }

    // each production will be a method that returns an optional node described in the grammar
    std::optional<NodeExpr*> parse_expr(int min_prec = 0) {
        std::optional<NodeTerm*> term_lhs = parse_term(); // obtain left hand side term

        if (!term_lhs.has_value()) {
            return {}; // cannot parse expr
        }

        // create lhs expression
        auto expr_lhs = m_allocator.alloc<NodeExpr>();
        expr_lhs->var = term_lhs.value(); // add lhs (term) to it

        // implement precedence climbing
        // https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
        while (true) {
            std::optional<Token> curr_token = peak();
            std::optional<int> prec;

            if (curr_token.has_value()) {
                prec = bin_prec(curr_token->type);

                // break if no precedence level OR precedence less than minimum
                if (!prec.has_value() || prec < min_prec) {
                    break;
                }
            } else {
                // break if no current token
                break;
            }

            // assume all operations to be left-associative
            // (2 + 3) + 4 = 2 + 3 + 4
            Token op = consume(); // consume operator
            int next_min_prec = prec.value() + 1;
            auto expr_rhs = parse_expr(next_min_prec); // compute rhs recursively

            if (!expr_rhs.has_value()) {
                std::cerr << "Unable to parse expression" << std::endl;
                exit(EXIT_FAILURE);
            }

            auto expr = m_allocator.alloc<NodeBinExpr>(); // create full expression (lhs and rhs)

            // copy of lhs to prevent a pointer loop
            auto expr_lhs2 = m_allocator.alloc<NodeExpr>();

            if (op.type == TokenType::plus) {
                auto add = m_allocator.alloc<NodeBinExprAdd>();
                expr_lhs2->var = expr_lhs->var;
                // then use the created expression in the main expr
                add->lhs = expr_lhs2;
                add->rhs = expr_rhs.value();
                expr->var = add;
            } else if (op.type == TokenType::star) {
                // same process as above
                auto mult = m_allocator.alloc<NodeBinExprMult>();
                expr_lhs2->var = expr_lhs->var;
                mult->lhs = expr_lhs2;
                mult->rhs = expr_rhs.value();
                expr->var = mult;
            } else if (op.type == TokenType::minus) {
                auto sub = m_allocator.alloc<NodeBinExprSub>();
                expr_lhs2->var = expr_lhs->var;
                // then use the created expression in the main expr
                sub->lhs = expr_lhs2;
                sub->rhs = expr_rhs.value();
                expr->var = sub;
            } else if (op.type == TokenType::fslash) {
                // same process as above
                auto div = m_allocator.alloc<NodeBinExprDiv>();
                expr_lhs2->var = expr_lhs->var;
                div->lhs = expr_lhs2;
                div->rhs = expr_rhs.value();
                expr->var = div;
            } else {
                assert(false);
            }


            // previously lhs was a term, but now it can become an expression
            // 1 + 2
            // (1) -> lhs (+) (2) -> rhs
            // (1 + 2) -> lhs _____ potential other expr
            expr_lhs->var = expr;
        }

        return expr_lhs;
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
            } else if (peak().has_value() && peak().value().type == TokenType::open_curly) {
                if (auto scope = parse_scope()) {
                    // according to grammar scope is a statement so treat it as such
                    auto stmt = m_allocator.alloc<NodeStmt>();
                    stmt->var = scope.value();
                    return stmt;
                } else {
                    std::cerr << "Invalid Scope" << std::endl;
                    exit(EXIT_FAILURE);
                }

            } else if (auto if_ = try_consume(TokenType::if_)) {
                try_consume(TokenType::open_paren, "Expected `(`"); // get the open paren
                // get the expression in the if statement
                auto stmt_if = m_allocator.alloc<NodeStmtIf>();

                if (auto expr = parse_expr()) {
                    stmt_if->expr = expr.value();
                } else {
                    std::cerr << "Invalid Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }

                try_consume(TokenType::close_paren, "Expected `)`"); // get the close paren (completes the statement)

                // parse the scope (described in the grammar)
                if (auto scope = parse_scope()) {
                    stmt_if->scope = scope.value();
                } else {
                    std::cerr << "Invalid (Expected) Scope" << std::endl;
                    exit(EXIT_FAILURE);
                }

                stmt_if->pred = parse_if_pred();

                // allocate a node statement for the If to be stored in (up the production tree)
                auto stmt = m_allocator.alloc<NodeStmt>();
                stmt->var = stmt_if;
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