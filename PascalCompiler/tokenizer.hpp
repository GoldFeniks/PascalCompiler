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

        class UnknownSymbolException : public std::exception {};
        class UnexpectedSymbolException : public std::exception {};
        class EOLWhileParsingStringException : public std::exception {};
        class ScaleFactorExpectedException : public std::exception {};
        class UnexpectedEndOfFileException : public std::exception {};
        class NumberExpectedException : public std::exception {};
        class FractionalPartExpectedException : public std::exception {};

		class Token {

		public:

			enum class Types {
				Identifier, Integer, Float, String, Operator, Separator, ReservedWord, EndOfFile
			};

			enum class SubTypes { 

				Identifier,        Integer,      Float,         StringConst,
                Plus,              Minus,        Mult,          Divide,
                Equal,             Less,         Greater,       OpenBracket,
                CloseBracket,      Dot,          Comma,         OpenParenthesis,
                CloseParenthesis,  Colon,        Semicolon,     Pointer,
                ShiftLeft,         ShiftRight,   Power,         NotEqual,
                SymmetricDiff,     LessEqual,    GreaterEqual,  Assign,
                PlusAssign,        MinusAssign,  MultAssign,    DivideAssign,
                Absolute,          And,          Array,         Asm,
                Begin,             Case,         Const,         Div,
                Do,                Downto,       Else,          End,        
                File,              For,          Function,      Goto,
                If,                In,           Inline,        Label,
                Mod,               Nil,          Not,           Of,
                Or,                Packed,       Procedure,     Program,
                Record,            Repeat,       Set,           String,
                Then,              To,           Type,          Unit,         
                Until,             Uses,         Var,           While,
                With,              Xor,          Range,         Operator

			};

			static const std::unordered_map<std::string, SubTypes> TokenSubTypes;
			
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
			const SubTypes GetSubType();
			const Types GetType();
			const std::string& GetString();
			const char* GetStringValue();
			const unsigned long long GetLongLongValue();
			const long double GetLongDoubleValue();

			private:

				std::pair<int, int> myPosition;
				SubTypes mySubType;
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

        static const Token endToken;

	private:

		friend struct iterator;

		int currentIndex = -1;
		std::vector<Token> tokens;
		std::ifstream file;
		FiniteAutomata::States state = FiniteAutomata::States::TokenEnd;
		int row = 1, column = 1;

	};

}//namespace My
