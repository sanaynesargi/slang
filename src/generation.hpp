#pragma once

// class for code generation to take parse tree and convert to asm
// replaces tokens_to_asm
class Generator {
public:

    inline Generator(NodeExit root)
        : m_root(std::move(root)) {

    }

    // eventually there will be a method to generate code for each node

    [[nodiscard]] std::string generate() const {
        std::stringstream output;
        output << "global _start\n_start:\n"; // add starting template assembly

        output << "    mov rax, 60\n";
        output << "    mov rdi, " << m_root.expr.int_lit.value.value() << "\n"; // int literal;
        output << "    syscall";

        return output.str();
    }

private:
    const NodeExit m_root;
};
