#include <iostream>
#include "tokenizer.hpp"
#include <string>
#include <iostream>

void tokenizerOutput(std::string inFile, std::string outFile) {
    std::ofstream out(outFile);
    My::Tokenizer tokenizer(inFile);
    My::Tokenizer::Token token = tokenizer.First();
    while ((token = tokenizer.Next()) != tokenizer.endToken)
        out << token.ToString() << std::endl;
}

int main(int argc, char* argv[]) {
	if (argc <= 1)
		std::cout << "Pascal compiler. Tyshchenko Andrey 2017";
    tokenizerOutput("Tests/Tokenizer/1.pas", "output.txt");
	return 0;
}