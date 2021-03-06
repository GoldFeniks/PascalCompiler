#include <iostream>
#include "tokenizer.hpp"
#include <string>
#include "syntax_analyzer.hpp"
#include "exceptions.hpp"
#include "asm_code.hpp"
#include "operations.hpp"

void tokenizer_output(const std::string in_file, const std::string out_file) {
    std::ofstream out(out_file);
    pascal_compiler::tokenizer tokenizer(in_file);
    auto token = tokenizer.first();
    try {
        while ((token = tokenizer.next()) != tokenizer.get_end_token())
            out << token->to_string() << std::endl;
    }
    catch (pascal_compiler::tokenizer::tokenizer_exception e) {
        out << e.what();
    }
}

void syntax_analyzer_output(const std::string in_file, const std::string out_file) {
    std::ofstream out(out_file);
    pascal_compiler::syntax_analyzer::syntax_analyzer syntax_analyzer(in_file);
    try {
        syntax_analyzer.parse();
        out << syntax_analyzer.tables().back().to_string();
    }
    catch (const pascal_compiler::exception e) {
        out << e.what();
    }
    catch (const pascal_compiler::syntax_analyzer::tree::convertion_error e) {
        out << e.what();
    }
}

void generator_output(const std::string in_file, const std::string out_file, bool optimize = false) {
    std::ofstream out(out_file);
    pascal_compiler::syntax_analyzer::syntax_analyzer syntax_analyzer(in_file);
    if (optimize)
        syntax_analyzer.set_optimizer(std::make_shared<pascal_compiler::optimizer::unreachable_code_optimizer>());
    try {
        syntax_analyzer.parse();
        pascal_compiler::code::asm_code code;
        syntax_analyzer.to_asm_code(code);
        out << code.to_string();
    }
    catch (const pascal_compiler::exception e) {
        out << e.what();
    }
    catch (const pascal_compiler::syntax_analyzer::tree::convertion_error e) {
        out << e.what();
    }

}

int main(const int argc, char* argv[]) {
    if (argc <= 1) {
        std::cout << "Pascal compiler. Tyshchenko Andrey 2017";
        return 0;
    }
    const std::string key(argv[1]);
    if (argc < 3) {
        std::cout << "File is not specified";
        return 0;
    }
    const std::string in_file = argc < 4 ? "output.txt" : argv[3];
    if (key == "-l")
        tokenizer_output(argv[2], in_file);
    else if (key == "-p")
        syntax_analyzer_output(argv[2], in_file);
    else if (key == "-g")
        generator_output(argv[2], in_file);
    else if (key == "-o") {
        generator_output(argv[2], in_file, true);
    }
    else
        std::cout << "Unknown key " << key;
	return 0;
}