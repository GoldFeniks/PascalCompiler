#pragma once
#include <string>
#include <fstream>
#include <utility>
#include <vector>
#include "finite_automata.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "boost/format.hpp"

namespace My {

    class Tokenizer {

    public:

        class TokenizerException : public std::exception {

        public:

            virtual const char* what() const override;

        protected:

            TokenizerException(const std::pair<int, int>& position) : position(position) {};

            const std::pair<int, int> position;
            char* message = nullptr;

        };

        class TokenizerSymbolException : public TokenizerException {

        protected:

            TokenizerSymbolException(const char symbol, const std::pair<int, int>& position, const char* format) :
                TokenizerException(position), symbol(symbol) { initMessage(format); };

            const char* initMessage(const char* format);

            const char symbol;

        };//class TokenizerException

        class UnknownSymbolException : public TokenizerSymbolException {

        public:

            UnknownSymbolException(const char symbol, const std::pair<int, int> position) : 
                TokenizerSymbolException(symbol, position, "Unknown symbol \"%1%\" at (%2%, %3%)") {};

        };//class UnknownSymbolException

        class UnexpectedSymbolException : public TokenizerSymbolException {

        public:

            UnexpectedSymbolException(const char symbol, const std::pair<int, int> position) :
                TokenizerSymbolException(symbol, position, "Unexpected symbol \"%1%\" at (%2%, %3%)") {};
        
        };//class UnexpectedSymbolException

        class EOLWhileParsingStringException : public TokenizerSymbolException {

        public:

            EOLWhileParsingStringException(const char symbol, const std::pair<int, int> position) :
                TokenizerSymbolException(symbol, position, "End of line reached at (%2%, %3%) while pasing string") {};
                    
        };//class EOLWhileParsingStringException

        class ScaleFactorExpectedException : public TokenizerSymbolException {

        public:

            ScaleFactorExpectedException(const char symbol, const std::pair<int, int> position) :
                TokenizerSymbolException(symbol, position, "Scale factor expected at (%2%, %3%)") {};
        
        };//class ScaleFactorExpectedException

        class UnexpectedEndOfFileException : public TokenizerSymbolException {
        
        public:

            UnexpectedEndOfFileException(const char symbol, const std::pair<int, int> position) :
                TokenizerSymbolException(symbol, position, "Unexpected end of file at (%2%, %3%)") {};

        };//class UnexpectedEndOfFileException

        class NumberExpectedException : public TokenizerSymbolException {

        public:

            NumberExpectedException(const char symbol, const std::pair<int, int> position) :
                TokenizerSymbolException(symbol, position, "Number expected at (%2%, %3%)") {};
            
        };//class NumberExpectedException

        class FractionalPartExpectedException : public TokenizerSymbolException {

        public:

            FractionalPartExpectedException(const char symbol, const std::pair<int, int> position) :
                TokenizerSymbolException(symbol, position, "fractional part expected at (%2%, %3%)") {};

        };//class FractionalPartExpectedException

        class OverflowException : public TokenizerException {

        public:

            OverflowException(const std::string& string, const std::pair<int, int>& position, const char* type);

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
                CharConst,         EndOfFile,         Write,              Read    
			};

            static const std::string SubTypesStrings[];
			static const std::unordered_map<std::string, SubTypes> TokenSubTypes;
            static const std::unordered_set<std::string> CharOperators;
			
			Token() = delete;
			Token(std::pair<int, int> position, std::string string, FiniteAutomata::States state, std::string rawString);
			Token(const Token& other);
			Token(Token&& other);

			Token& operator=(const Token& other);
			Token& operator=(Token&& other);

			bool operator==(const Token& other);
			bool operator!=(const Token& other);

			~Token();

			const std::pair<int, int>& GetPosition() const;
			const SubTypes GetSubType() const;
			const Types GetType() const;
			const std::string& GetString() const;
			const char* GetStringValue() const;
			const long long GetLongLongValue() const;
			const long double GetLongDoubleValue() const;
            std::string GetValueString() const;
            const std::string ToString() const;

            template<typename T>
            T GetValue() const;

            union Value {

                char* String;
                long long LongLong;
                double Double;

                operator char*() const { return String; }
                operator long long() const { return LongLong; }
                operator double() const { return Double; }
                operator char() const { return String[0]; }

                Value(char* String) : String(String) {};
                Value(long long LongLong) : LongLong(LongLong) {};
                Value(double Double) : Double(Double) {};
                Value(bool b) : LongLong(b ? 0 : 1) {};
                Value() {};

            };

            Value GetValue() const {
                return myValue;
            };

			private:

				std::pair<int, int> myPosition;
				SubTypes mySubType;
				Types myType;
                std::string myString;

                friend class CnstantNode;

				Value myValue;

				bool stringUsed = false;
				void saveString(std::string& string);
				void saveString(const char* string);
                static std::string escape(std::string string);

                friend class Tokenizer;

		};//class Token

        typedef std::shared_ptr<Token> PToken;

		Tokenizer() = delete;
		Tokenizer(const std::string file);
		Tokenizer(std::ifstream&& file);
        Tokenizer(const Tokenizer&) = delete;
        Tokenizer(Tokenizer&& other);

        Tokenizer& operator=(const Tokenizer&) = delete;
        Tokenizer& operator=(Tokenizer&& other);

		const PToken Next();
		const PToken Current() const; 
		const PToken First();
		bool IsEnd() const;
        const PToken GetEndToken() const;
        void Back(const int i);

	private:

		friend struct iterator;

        PToken endToken = PToken(new Token({ 1, 1 }, "", FiniteAutomata::States::End, ""));
		int currentIndex = -1;
		std::vector<PToken> tokens;
		std::ifstream file;
		FiniteAutomata::States state = FiniteAutomata::States::TokenEnd;
		int row = 1, column = 1;

        int codeToChar(My::FiniteAutomata::States state, const char* charCode);
        void tryThrowException(My::FiniteAutomata::States state, char c) const;

	};//class Tokenizer

    template<typename T>
    T Tokenizer::Token::GetValue() const {
        return T(myValue);
    }

}//namespace My
