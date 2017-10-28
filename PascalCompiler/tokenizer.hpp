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

namespace pascal_compiler {

    class tokenizer {

    public:

        class tokenizer_exception : public std::exception {

        public:

            const char* what() const override;

        protected:

            explicit tokenizer_exception(const std::pair<int, int>& position) : position_(position) {};

            const std::pair<int, int> position_;
            char* message_ = nullptr;

        };

        class tokenizer_symbol_exception : public tokenizer_exception {

        protected:

            tokenizer_symbol_exception(const char symbol, const std::pair<int, int>& position, const char* format) :
                tokenizer_exception(position), symbol_(symbol) { init_message(format); };

            const char* init_message(const char* format);

            const char symbol_;

        };//class TokenizerException

        class unknown_symbol_exception : public tokenizer_symbol_exception {

        public:

            unknown_symbol_exception(const char symbol, const std::pair<int, int> position) : 
                tokenizer_symbol_exception(symbol, position, "Unknown symbol \"%1%\" at (%2%, %3%)") {};

        };//class UnknownSymbolException

        class unexpected_symbol_exception : public tokenizer_symbol_exception {

        public:

            unexpected_symbol_exception(const char symbol, const std::pair<int, int> position) :
                tokenizer_symbol_exception(symbol, position, "Unexpected symbol \"%1%\" at (%2%, %3%)") {};
        
        };//class UnexpectedSymbolException

        class eol_while_parsing_string_exception : public tokenizer_symbol_exception {

        public:

            eol_while_parsing_string_exception(const char symbol, const std::pair<int, int> position) :
                tokenizer_symbol_exception(symbol, position, "End of line reached at (%2%, %3%) while pasing string") {};
                    
        };//class EOLWhileParsingStringException

        class scale_factor_expected_exception : public tokenizer_symbol_exception {

        public:

            scale_factor_expected_exception(const char symbol, const std::pair<int, int> position) :
                tokenizer_symbol_exception(symbol, position, "Scale factor expected at (%2%, %3%)") {};
        
        };//class ScaleFactorExpectedException

        class unexpected_end_of_file_exception : public tokenizer_symbol_exception {
        
        public:

            unexpected_end_of_file_exception(const char symbol, const std::pair<int, int> position) :
                tokenizer_symbol_exception(symbol, position, "Unexpected end of file at (%2%, %3%)") {};

        };//class UnexpectedEndOfFileException

        class number_expected_exception : public tokenizer_symbol_exception {

        public:

            number_expected_exception(const char symbol, const std::pair<int, int> position) :
                tokenizer_symbol_exception(symbol, position, "Number expected at (%2%, %3%)") {};
            
        };//class NumberExpectedException

        class fractional_part_expected_exception : public tokenizer_symbol_exception {

        public:

            fractional_part_expected_exception(const char symbol, const std::pair<int, int> position) :
                tokenizer_symbol_exception(symbol, position, "fractional part expected at (%2%, %3%)") {};

        };//class FractionalPartExpectedException

        class overflow_exception : public tokenizer_exception {

        public:

            overflow_exception(const std::string& string, const std::pair<int, int>& position, const char* type);

        };

		class token {

		public:

			enum class types : unsigned int {
				identifier, integer, real, symbol, string, operation, separator, reserved_word, end_of_file
			};

            static const std::string types_strings[];

			enum class sub_types : unsigned int {

				identifier,        integer_const,     real_const,         string_const,
                plus,              minus,             mult,               divide,
                equal,             less,              greater,            open_bracket,
                close_bracket,     dot,               comma,              open_parenthesis,
                close_parenthesis, colon,             semicolon,          pointer,
                shift_left,        shift_right,       power,              not_equal,
                symmetric_diff,    less_equal,        greater_equal,      assign,
                plus_assign,       minus_assign,      mult_assign,        divide_assign,
                absolute,          and,               array,              asm_op,
                begin,             case_op,           const_op,           div,
                do_op,             downto,            else_op,            end,        
                file,              for_op,            function,           goto_op,
                if_op,             in,                inline_op,          label,
                mod,               nil,               not,                of,
                or,                packed,            procedure,          program,
                record,            repeat,            set,                string,
                then,              to,                type,               unit,         
                until,             uses,              var,                while_op,
                with,              xor,               range,              operator_op,
                char_const,        end_of_file,       write,              read,
                break_op,          continue_op
			};

            static const std::string sub_types_strings[];
			static const std::unordered_map<std::string, sub_types> token_sub_types;
            static const std::unordered_set<std::string> char_operators;
			
			token() = delete;
			token(std::pair<int, int> position, const std::string& string, finite_automata::states state, const std::string& raw_string);
			token(const token& other);
			token(token&& other) noexcept;

			token& operator=(const token& other);
			token& operator=(token&& other) noexcept;

			bool operator==(const token& other) const;
			bool operator!=(const token& other) const;

			~token();

			const std::pair<int, int>& get_position() const;
		    sub_types get_sub_type() const;
		    types get_type() const;
			const std::string& get_string() const;
			const char* get_string_value() const;
		    long long get_long_long_value() const;
		    long double get_long_double_value() const;
            std::string get_value_string() const;
		    std::string to_string() const;

            template<typename T>
            T get_value() const;

            union value {

                char* string;
                long long long_long;
                double real;

                explicit operator char*() const { return string; }
                explicit operator long long() const { return long_long; }
                explicit operator double() const { return real; }
                explicit operator char() const { return string[0]; }

                explicit value(const long long long_long) : long_long(long_long) {};
                explicit value(const double real) : real(real) {};
                explicit value(const bool b) : long_long(b ? 0 : 1) {};
                value() {};

            };

            value get_value() const {
                return value_;
            };

			private:

				std::pair<int, int> position_;
				sub_types sub_type_;
				types type_;
                std::string string_;

                friend class CnstantNode;

				value value_;

				bool string_used_ = false;
				void save_string(const std::string& string);
				void save_string(const char* string);
                static std::string escape(std::string string);

                friend class tokenizer;

		};//class Token

        typedef std::shared_ptr<token> token_p;

		tokenizer() = delete;
        explicit tokenizer(const std::string file);
        explicit tokenizer(std::ifstream&& file);
        tokenizer(const tokenizer&) = delete;
        tokenizer(tokenizer&& other) noexcept;

        tokenizer& operator=(const tokenizer&) = delete;
        tokenizer& operator=(tokenizer&& other) noexcept;

        token_p next();
        token_p current() const;
        token_p first();
		bool is_end() const;
        token_p get_end_token() const;
        void back(const int i);

	private:

		friend struct iterator;

        token_p end_token_ = std::make_shared<token>(std::make_pair(1, 1), "", finite_automata::states::end, "");
		int current_index_ = -1;
		std::vector<token_p> tokens_;
		std::ifstream file_;
		finite_automata::states state_ = finite_automata::states::token_end;
		int row_ = 1, column_ = 1;

        int code_to_char(pascal_compiler::finite_automata::states state, const char* char_code);
        void try_throw_exception(pascal_compiler::finite_automata::states state, char c) const;

	};//class Tokenizer

    template<typename T>
    T tokenizer::token::get_value() const {
        return T(value_);
    }

}//namespace pascal_compiler
