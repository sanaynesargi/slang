#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>
#include <cassert>

#include "./tokenization.hpp"
#include "./parser.hpp"
#include "./generation.hpp"


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

    Parser parser(std::move(tokens));
    std::optional<NodeProg> prog = parser.parse_prog();

    if (!prog.has_value()) {
        std::cerr << "Invalid program" << std::endl;
        exit(EXIT_FAILURE);
    }

    Generator generator(prog.value());

    // create scope for automatic file closing
    {
        std::fstream file("out.asm", std::ios::out);
        file << generator.gen_prog(); // stream assembly into output file
    }

    // assemble and link the compiled code
    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}
