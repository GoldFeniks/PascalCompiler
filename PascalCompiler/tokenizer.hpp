#pragma once
#include <string>
#include <fstream>
#include <utility>
#include <vector>
#include "finite_automata.hpp"
#include <unordered_map>

namespace My {

	class Tokenizer {

	public:

		class UnexpectedSymbolException : std::exception {};
		class InvalidSyntaxException : std::exception {};
		class EOLWhileParsingStringException : std::exception {};

		class Token {

		public:

			enum Types { 
				Identifier, 
				Integer, 
				Float,
				String,
				Plus,
				Minus,
				Mult,
				Divide,
				Equal,
				Less,
				Greater,
				OpenBracket,
				CloseBracket,
				Comma,
				Colon,
				Semicolon,
				Adress,
				OpenParenthesis,
				CloseParenthesis,
				NotEqual,
				LessOrEqual,
				GreaterOrEqual,
				Assign,
				Range,
				And,
				Array,
				Begin,
				Case,
				Const,
				Div,
				Do,
				Downto,
				Else,
				End,
				File,
				For,
				Function,
				Goto,
				If,
				In,
				Label,
				Mod,
				Nil,
				Not,
				Of,
				Or,
				Packed,
				Procedure,
				Program,
				Record,
				Repeat,
				Set,
				Then,
				To,
				Type,
				Until,
				Var,
				While,
				With
			};

			const std::unordered_map<std::string, Types> KeyWords = {
				{ "and", And },
				{ "array", Array },
				{ "begin", Begin },
				{ "case", Case },
				{ "const", Const },
				{ "div", Div },
				{ "do", Do },
				{ "downto", Downto },
				{ "else", Else },
				{ "end", End },
				{ "file", File },
				{ "for", For },
				{ "function", Function },
				{ "goto", Goto },
				{ "if", If },
				{ "in", In },
				{ "label", Label },
				{ "mod", Mod },
				{ "nil", Nil },
				{ "not", Not },
				{ "of", Of },
				{ "packed", Packed },
				{ "procedure", Procedure },
				{ "program", Program },
				{ "record", Record },
				{ "repeat", Repeat },
				{ "set", Set },
				{ "then", Then },
				{ "to", To },
				{ "type", Type },
				{ "until", Until },
				{ "var", Var },
				{ "while", While },
				{ "with", With }
			};

			std::unordered_map<std::string, Types> Operators = {
				{ "+", Plus },
				{ "-", Minus },
				{ "*", Mult },
				{ "/", Divide },
				{ "=", Equal },
				{ "<", Less },
				{ ">", Greater },
				{ "@", Adress },
				{ "^", Adress },
				{ "<>", NotEqual },
				{ "<=", LessOrEqual },
				{ ">=", GreaterOrEqual },
				{ ":=", Assign },
				{ "..", Range }
			};

			std::unordered_map<std::string, Types> Separators = {
				{ "[", OpenBracket },
				{ "]", CloseBracket },
				{ ",", Comma },
				{ ":", Colon },
				{ ";", Semicolon },
				{ "(", OpenParenthesis },
				{ ")", CloseParenthesis }
			};
			
			Token() = delete;
			Token(std::pair<int, int> position, std::string string, FiniteAutomata::States state);
			Token(const Token& other);
			Token(Token&& other);

			Token& operator=(const Token& other);
			Token& operator=(Token&& other);

			bool operator==(const Token& other);
			bool operator!=(const Token& other);

			~Token();

			const std::pair<int, int>& GetPosition();
			const Types& GetType();
			const std::string& GetString();
			const char* GetStringValue();
			const unsigned long long GetLongLongValue();
			const long double GetLongDoubleValue();

			private:

				std::pair<int, int> myPosition;
				Types myType;
				std::string myString;

				union Value {
					char* String;
					unsigned long long UnsignedLongLong;
					long double LongDouble;
				};

				Value myValue;

				bool stringUsed = false;
				void saveString(std::string& string);
				void saveString(const char* string);

		};

		Tokenizer() = delete;
		Tokenizer(std::string file);
		Tokenizer(std::ifstream file);

		const Token& Next();
		const Token& Current();
		const Token& First();
		bool IsEnd();

	private:

		friend struct iterator;

		static const Token endToken;
		int currentIndex = -1;
		std::vector<Token> tokens;
		std::ifstream file;
		FiniteAutomata::States state = FiniteAutomata::States::Stop;
		int row = 1, column = 1;

	};

}//namespace My
