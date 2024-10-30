#pragma once

#include <map>

// class for code generation to take parse tree and convert to asm
// replaces tokens_to_asm
class Generator {
public:

    inline explicit Generator(NodeProg root)
        : m_prog(std::move(root)) {

    }

    // generate term
    void gen_term(const NodeTerm* term) {
        // define visitors with operators that allow them to take different types and generate different code
        // terms can be int literals or identifiers and different asm is generated via std::visit
        // std::visit determines type at runtime and calls the correct overload
        struct TermVisitor {
            Generator* gen;
            void operator()(const NodeTermIntLit* term_int_lit) const {
                gen->m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
                gen->push("rax");
            }
            void operator()(const NodeTermIdent* term_ident) const {
                // check if identifier (e.g. var) is declared
                // find if takes two iterators, the start and the end of the vector
                // the lambda function matches the name of the iterated var to the current term
                // if there is nothing found (iterator reaches end) then return an error (no match found)
                auto it = std::find_if(
                        gen->m_vars.cbegin(),
                        gen->m_vars.cend(),
                        [&](const Var& var){return var.name == term_ident->ident.value.value();}
                        );

                if (it == gen->m_vars.cend()) {
                    std::cerr << "Undeclared identifier: " << term_ident->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }

                // get offset from stack pointer to read value and push onto stack
                const auto &var = (*it); // de-reference iterator as that is the matched value
                std::stringstream offset;

                // calculate offset by subtracting variable stack location from current stack location
                // m_stack_loc - var.stack_loc is in bytes so need to multiply by 8 to convert to bits
                // subtract 1 because we don't need to move the stack pointer to get the current top of the stack
                offset << "QWORD [rsp + " << (gen->m_stack_loc - var.stack_loc - 1) * 8 << "]\n";
                gen->push(offset.str());
            }
            void operator()(const NodeTermParen* term_paren) const {
                gen->gen_expr(term_paren->expr);
            }
        };

        TermVisitor visitor({.gen = this});
        std::visit(visitor, term->var);
    }

    void gen_bin_expr(const NodeBinExpr* bin_expr) {
        // same principle - use std::visit to create visitors and call diff
        // functions based on type of bin expr

        struct BinExprVisitor {
            Generator* gen;
            void operator()(const NodeBinExprSub* sub) const {
                // push expressions for lhs and rhs to the top of the stack
                gen->gen_expr(sub->rhs);
                gen->gen_expr(sub->lhs);
                // pop values (commutative no order required) into rax and rbx
                gen->pop("rax");
                gen->pop("rbx");
                // use asm sub instruction and push result into rax
                gen->m_output << "    sub rax, rbx\n";
                gen->push("rax"); // variable to assign is at top of stack so push rax to the top
            }
            void operator()(const NodeBinExprDiv* div) const {
                // push expressions for lhs and rhs to the top of the stack
                gen->gen_expr(div->rhs);
                gen->gen_expr(div->lhs);
                // pop values (commutative no order required) into rax and rbx
                gen->pop("rax");
                gen->pop("rbx");
                // use asm div instruction and push result into rax
                gen->m_output << "    div rbx\n"; // mult multiplies specified register with rax and stores in rax
                gen->push("rax"); // variable to assign is at top of stack so push rax to the top
            }
            void operator()(const NodeBinExprAdd* add) const {
                // push expressions for lhs and rhs to the top of the stack
                gen->gen_expr(add->rhs);
                gen->gen_expr(add->lhs);
                // pop values (commutative no order required) into rax and rbx
                gen->pop("rax");
                gen->pop("rbx");
                // use asm add instruction and push result into rax
                gen->m_output << "    add rax, rbx\n";
                gen->push("rax"); // variable to assign is at top of stack so push rax to the top
            }
            void operator()(const NodeBinExprMult* mult) const {
                // push expressions for lhs and rhs to the top of the stack
                gen->gen_expr(mult->rhs);
                gen->gen_expr(mult->lhs);
                // pop values (commutative no order required) into rax and rbx
                gen->pop("rax");
                gen->pop("rbx");
                // use asm mult instruction and push result into rax
                gen->m_output << "    mul rbx\n"; // mult multiplies specified register with rax and stores in rax
                gen->push("rax"); // variable to assign is at top of stack so push rax to the top
            }
        };

        BinExprVisitor visitor{ .gen = this};
        std::visit(visitor, bin_expr->var);
    }

    // generate expression
    void gen_expr(const NodeExpr* expr) {
        // push value of expression to the top of the stack for later use
        struct ExprVisitor {
            Generator* gen;
            void operator()(const NodeTerm* term) const {
                gen->gen_term(term);
            }
            void operator()(const NodeBinExpr* bin_expr) const {
                gen->gen_bin_expr(bin_expr);
            }
        };

        // get current expression by checking each type of operator
        ExprVisitor visitor{.gen = this};
        std::visit(visitor, expr->var);
    }

    // eventually there will be a method to generate code for each node
    void gen_stmt(const NodeStmt* stmt) {
        // match operator for type -> depending on the type the corresponding function will be called
        struct StmtVisitor {
            Generator* gen;
            void operator()(const NodeStmtExit* stmt_exit) const {
                gen->gen_expr(stmt_exit->expr);
                gen->m_output << "    mov rax, 60\n";
                gen->pop("rdi");
                gen->m_output << "    syscall\n";
            }
            void operator()(const NodeStmtDef* stmt_def) const {
                // variable assembly generation
                auto it = std::find_if(
                        gen->m_vars.cbegin(),
                        gen->m_vars.cend(),
                        [&](const Var& var){return var.name == stmt_def->ident.value.value();}
                );

                // check if variable has been declared (does not equal the end)
                if (it != gen->m_vars.cend()) {
                    std::cerr << "Identifier already used: " << stmt_def->ident.value.value() << std::endl;
                exit(EXIT_FAILURE);
                }

                // insert declaration into map, stack position is current stack position
                gen->m_vars.push_back({.name = stmt_def->ident.value.value(), .stack_loc = gen -> m_stack_loc});
                gen->gen_expr(stmt_def->expr); // push expression value to the top of the stack
            }
            void operator()(const NodeScope* scope) const {
                gen->begin_scope();

                for (const NodeStmt* stmt : scope->stmts) {
                    gen->gen_stmt(stmt);
                }

                gen->end_scope();
            }
            void operator()(const NodeStmtIf* stmt_if) const {
                // handle if statement
                assert(false && "Not Implemented");
            }
        };

        StmtVisitor visitor{.gen = this};
        std::visit(visitor, stmt->var);
    }

    // generate program
    [[nodiscard]] std::string gen_prog() {
        m_output << "global _start\n_start:\n"; // add starting template assembly

        for (NodeStmt* stmt : m_prog.stmts) {
            gen_stmt(stmt);
        }

        m_output << "    mov rax, 60\n";
        m_output << "    mov rdi, 0\n"; // int literal;
        m_output << "    syscall";

        return m_output.str();
    }


private:
    // abstractions for push and pop stack methods
    void push(const std::string& reg) {
        m_output << "    push " << reg << "\n";
        m_stack_loc++;
    }

    void pop(const std::string& reg) {
        m_output << "    pop " << reg << "\n";
        m_stack_loc--;
    }

    void begin_scope() {
        // we mark the start of a scope by the current length of m_vars
        // everything above this on the stack will be part of this scope
        m_scopes.push_back(m_vars.size());
    }

    void end_scope() {
        // pop all variables until we reach the previous begin_scope (in m_scopes)
        size_t pop_count = m_vars.size() - m_scopes.back();
        m_output << "    add rsp, " << pop_count * 8 << "\n"; // add to stack pointer (move above to pop off)
        m_stack_loc -= pop_count;

        // remove variables in m_vars;
        for (int i = 0; i < pop_count; i++) {
            m_vars.pop_back();
        }

        m_scopes.pop_back();
    }

    struct Var {
        std::string name;
        size_t stack_loc;
    };

    const NodeProg m_prog;
    std::stringstream m_output;
    size_t m_stack_loc = 0;
    std::vector<Var> m_vars {};
    std::vector<size_t> m_scopes;
};
