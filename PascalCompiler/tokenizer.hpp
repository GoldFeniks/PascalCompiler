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

        class TokenizerException : public std::exception {

        public:

            virtual const char* what() const override;

        protected:

            TokenizerException(const char symbol, const std::pair<int, int> position, const char* format) :
                symbol(symbol), position(position) { initMessage(format); };

            const char* initMessage(const char* format);

            const char symbol;
            const std::pair<int, int> position;
            char* message = nullptr;

        };

        class UnknownSymbolException : public TokenizerException {

        public:

            UnknownSymbolException(const char symbol, const std::pair<int, int> position) : 
                TokenizerException(symbol, position, "Unknown symbol \"%1%\" at (%2%, %3%)") {};

        };

        class UnexpectedSymbolException : public TokenizerException {

        public:

            UnexpectedSymbolException(const char symbol, const std::pair<int, int> position) :
                TokenizerException(symbol, position, "Unexpected symbol \"%1%\" at (%2%, %3%)") {};
        
        };

        class EOLWhileParsingStringException : public TokenizerException {

        public:

            EOLWhileParsingStringException(const char symbol, const std::pair<int, int> position) :
                TokenizerException(symbol, position, "End of line reached at (%2%, %3%) while pasing string") {};
                    
        };

        class ScaleFactorExpectedException : public TokenizerException {

        public:

            ScaleFactorExpectedException(const char symbol, const std::pair<int, int> position) :
                TokenizerException(symbol, position, "Scale factor expected at (%2%, %3%)") {};
        
        };

        class UnexpectedEndOfFileException : public TokenizerException {
        
        public:

            UnexpectedEndOfFileException(const char symbol, const std::pair<int, int> position) :
                TokenizerException(symbol, position, "Unexpected end of file at (%2%, %3%)") {};

        };

        class NumberExpectedException : public TokenizerException {

        public:

            NumberExpectedException(const char symbol, const std::pair<int, int> position) :
                TokenizerException(symbol, position, "Number expected at (%2%, %3%)") {};
            
        };

        class FractionalPartExpectedException : public TokenizerException {

        public:

            FractionalPartExpectedException(const char symbol, const std::pair<int, int> position) :
                TokenizerException(symbol, position, "fractional part expected at (%2%, %3%)") {};

        };

		class Token {

		public:

			enum class Types : unsigned int {
				Identifier, Integer, Float, Char, String, Operator, Separator, ReservedWord, EndOfFile
			};

            static const std::string TypesStrings[];

			enum class SubTypes : unsigned int {

				Identifier,        IntegerConst,      FloatConst,         StringConst,
                Plus,              Minus,             Mult,               Divide,
                Equal,             Less,              Greater,            OpenBracket,
                CloseBracket,      Dot,               Comma,              OpenParenthesis,
                CloseParenthesis,  Colon,             Semicolon,          Pointer,
                ShiftLeft,         ShiftRight,        Power,              NotEqual,
                SymmetricDiff,     LessEqual,         GreaterEqual,       Assign,
                PlusAssign,        MinusAssign,       MultAssign,         DivideAssign,
                Absolute,          And,               Array,              Asm,
                Begin,             Case,              Const,              Div,
                Do,                Downto,            Else,               End,        
                File,              For,               Function,           Goto,
                If,                In,                Inline,             Label,
                Mod,               Nil,               Not,                Of,
                Or,                Packed,            Procedure,          Program,
                Record,            Repeat,            Set,                String,
                Then,              To,                Type,               Unit,         
                Until,             Uses,              Var,                While,
                With,              Xor,               Range,              Operator,
                CharConst,

			};

            static const std::string SubTypesStrings[];

			static const std::unordered_map<std::string, SubTypes> TokenSubTypes;
			
			Token() = delete;
			Token(std::pair<int, int> position, std::string string, FiniteAutomata::States state, std::string rawString);
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
            const std::string ToString();

			private:

				std::pair<int, int> myPosition;
				SubTypes mySubType;
				Types myType;
				std::string myString;

				union Value {
					char* String;
					unsigned long long UnsignedLongLong;
					double Double;
				};

				Value myValue;

				bool stringUsed = false;
				void saveString(std::string& string);
				void saveString(const char* string);
                std::string escape(std::string string);

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

        static long int codeToChar(My::FiniteAutomata::States state, const char* charCode);
        void tryThrowException(My::FiniteAutomata::States state, char c);

	};

}//namespace My
