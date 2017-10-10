#include <iostream>
#include "tokenizer.hpp"
#include <string>
#include "syntax_analyzer.hpp"

void tokenizerOutput(std::string inFile, std::string outFile) {
    std::ofstream out(outFile);
    My::Tokenizer tokenizer(inFile);
    My::Tokenizer::PToken token = tokenizer.First();
    try {
        while ((token = tokenizer.Next()) != tokenizer.endToken)
            out << token->ToString() << std::endl;
    }
    catch (My::Tokenizer::TokenizerException e) {
        out << e.what();
    }
}

void syntaxAnalyzerOutput(std::string inFile, std::string outFile) {
    std::ofstream out(outFile);
    My::SyntaxAnalyzer syntaxAnalyzer(inFile);
    try {
        syntaxAnalyzer.Parse();
        out << syntaxAnalyzer.ToString();
    }
    catch (My::SyntaxAnalyzer::SyntaxErrorException e) {
        out << e.what();
    }
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        std::cout << "Pascal compiler. Tyshchenko Andrey 2017";
        return 0;
    }
    std::string key(argv[1]);
    if (argc < 3) {
        std::cout << "File is not specified";
        return 0;
    }
    std::string inFile = argc < 4 ? "output.txt" : argv[3];
    if (key == "-l")
        tokenizerOutput(argv[2], inFile);
    else if (key == "-p")
        syntaxAnalyzerOutput(argv[2], inFile);
    else
        std::cout << "Unknown key " << key;
	return 0;
}