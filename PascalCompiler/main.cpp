#include <iostream>
#include "tokenizer.hpp"
#include <string>
#include <iostream>

void tokenizerOutput(std::string inFile, std::string outFile) {
    std::ofstream out(outFile);
    My::Tokenizer tokenizer(inFile);
    My::Tokenizer::Token token = tokenizer.First();
    try {
        while ((token = tokenizer.Next()) != tokenizer.endToken)
            out << token.ToString() << std::endl;
    }
    catch (My::Tokenizer::TokenizerException e) {
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
    if (key == "-l") {
        if (argc < 4)
            tokenizerOutput(argv[2], "output.txt");
        else
            tokenizerOutput(argv[2], argv[3]);
    }
    else
        std::cout << "Unknown key " << key;
	return 0;
}