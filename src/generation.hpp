#pragma once

#include <unordered_map>

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
                if (!gen->m_vars.contains(term_ident->ident.value.value())) {
                    std::cerr << "Undeclared identifier: " << term_ident->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }

                // get offset from stack pointer to read value and push onto stack
                const auto &var = gen->m_vars.at(term_ident->ident.value.value());
                std::stringstream offset;

                // calculate offset by subtracting variable stack location from current stack location
                // m_stack_loc - var.stack_loc is in bytes so need to multiply by 8 to convert to bits
                // subtract 1 because we don't need to move the stack pointer to get the current top of the stack
                offset << "QWORD [rsp + " << (gen->m_stack_loc - var.stack_loc - 1) * 8 << "]\n";
                gen->push(offset.str());
            }
        };

        TermVisitor visitor({.gen = this});
        std::visit(visitor, term->var);
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
                assert(false);  // not implemented
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

                // check if variable has been declared
                if (gen->m_vars.contains(stmt_def->ident.value.value())) {
                    std::cerr << "Identifier already used: " << stmt_def->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }

                // insert declaration into map, stack position is current stack position
                gen->m_vars.insert({stmt_def->ident.value.value(), Var{.stack_loc = gen -> m_stack_loc}});
                gen->gen_expr(stmt_def->expr); // push expression value to the top of the stack
            }
            void operator()(const NodeStmt* stmt_def) const {
                assert(false);  // not implemented
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

    struct Var {
        size_t stack_loc;
    };

    const NodeProg m_prog;
    std::stringstream m_output;
    size_t m_stack_loc = 0;
    std::unordered_map<std::string, Var> m_vars {};
};
