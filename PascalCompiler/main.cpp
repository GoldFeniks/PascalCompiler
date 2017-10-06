#include <iostream>
#include "tokenizer.hpp"

int main(int argc, char* argv[]) {
	if (argc <= 1)
		std::cout << "Pascal compiler. Tyshchenko Andrey 2017";
	My::Tokenizer tokenizer("1.pas");
	My::Tokenizer::Token token = tokenizer.First();
	while (!tokenizer.IsEnd()) {
		token = tokenizer.Next();
		int a = 1;
	}
	return 0;
}