#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

#include "./tokenization.hpp"

// temporary function to covert tokens to assembly
std::string tokens_to_asm(const std::vector<Token>& tokens) {
    std::stringstream output;

    output << "global _start\n_start:\n"; // add starting template assembly

    for (int i = 0; i < tokens.size(); i++) {
        const Token& token = tokens.at(i);

        // check if after a exit value is an int_lit followed by a semi
        // this is the test syntax we have in the file currently
        if (token.type == TokenType::exit) {
            if (i + 1 < tokens.size() && tokens.at(i + 1).type == TokenType::int_lit) {
                if (i + 2 < tokens.size() && tokens.at(i + 2).type == TokenType::semi) {
                    // x86 assembly for the syscall exit (to exit from a program)
                    output << "    mov rax, 60\n";
                    output << "    mov rdi, " << tokens.at(i + 1).value.value() << "\n"; // int literal;
                    output << "    syscall";
                }
            }
        }
    }

    return output.str(); // return string from stream
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Incorrect usage. Correct usage is..." << std::endl;
        std::cerr << "sl <input.sl>" << std::endl;

        return EXIT_FAILURE;
    }

    std::string contents;
    {
        std::stringstream contents_stream; // create file content buffer (stream)
        std::fstream input(argv[1], std::ios::in); // create file input
        contents_stream << input.rdbuf(); // load file contents
        contents = contents_stream.str(); // final file as string from buf
    } // close file through destructor (closed scope)


    Tokenizer tokenizer(std::move(contents)); // move contents into tokenizer
    std::vector<Token> tokens = tokenizer.tokenize();

    // create scope for automatic file closing
    {
        std::fstream file("out.asm", std::ios::out);
        file << tokens_to_asm(tokens); // stream assembly into output file
    }

    // assemble and link the compiled code
    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}
