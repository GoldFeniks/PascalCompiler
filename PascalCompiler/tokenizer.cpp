#include "tokenizer.hpp"
#include <cstdio>
#include <iostream>
#include "boost/format.hpp"
#include <regex>

using namespace pascal_compiler;

const std::unordered_map<std::string, tokenizer::token::sub_types> tokenizer::token::token_sub_types = {
	{ "and",  sub_types::and },           { "array", sub_types::array },        { "begin", sub_types::begin },          { "case", sub_types::case_op },
	{ "const", sub_types::const_op },        { "div", sub_types::div },            { "do", sub_types::do_op },                { "downto", sub_types::downto },
	{ "else", sub_types::else_op },          { "end", sub_types::end },            { "file", sub_types::file },            { "for", sub_types::for_op },
	{ "function", sub_types::function },  { "goto", sub_types::goto_op },          { "if", sub_types::if_op },                { "in", sub_types::in },
	{ "label", sub_types::label },        { "mod", sub_types::mod },            { "nil", sub_types::nil },              { "not", sub_types::not },
	{ "of", sub_types::of },              { "packed", sub_types::packed },      { "procedure", sub_types::procedure },  { "program", sub_types::program },
	{ "record", sub_types::record },      { "repeat", sub_types::repeat },      { "set", sub_types::set },              { "then", sub_types::then },
	{ "to", sub_types::to },              { "type", sub_types::type },          { "until", sub_types::until },          { "var", sub_types::var },
	{ "while", sub_types::while_op },        { "with", sub_types::with },          { "+", sub_types::plus },               { "-", sub_types::minus },
	{ "*", sub_types::mult },             { "/", sub_types::divide },           { "=", sub_types::equal },              { "<", sub_types::less },
	{ ">", sub_types::greater },          { "@", sub_types::pointer },          { "^", sub_types::pointer },            { "<>", sub_types::not_equal },
	{ "<=", sub_types::less_equal },       { ">=", sub_types::greater_equal },    { ":=", sub_types::assign },            { "..", sub_types::range },
	{ "[", sub_types::open_bracket },      { "]", sub_types::close_bracket },     { ",", sub_types::comma },              { ":", sub_types::colon },
	{ ";", sub_types::semicolon },        { "(", sub_types::open_parenthesis },  { ")", sub_types::close_parenthesis },   { "(.", sub_types::open_bracket },
    { ".)", sub_types::close_bracket },    { "asm", sub_types::asm_op },            { "<<", sub_types::shift_left },         { ">>", sub_types::shift_right },
    { "shl", sub_types::shift_left },      { "shr", sub_types::shift_right },     { "**", sub_types::power },             { "><", sub_types::symmetric_diff },
    { "+=", sub_types::plus_assign },      { "-=", sub_types::minus_assign },     { "*=", sub_types::mult_assign },        { "/=", sub_types::divide_assign },
    { "absolute", sub_types::absolute },  { "inline", sub_types::inline_op },      { "string", sub_types::string },        { "unit", sub_types::unit },
    { "uses", sub_types::uses },          { "xor", sub_types::xor },            { "operator", sub_types::operator_op },    { ".", sub_types::dot },
    { "or", sub_types::or },              { "write", sub_types::write },        { "read", sub_types::read },            { "break", sub_types::break_op },
    { "continue", sub_types::continue_op }
};

const std::string tokenizer::token::types_strings[] = {
    "Identifier", "Integer", "Real", "Symbol", "String", "Operation", "Separator", "ReservedWord", "EndOfFile"
};

const std::string tokenizer::token::sub_types_strings[] = {

    "Identifier",        "IntegerConst",     "RealConst",          "StringConst",
    "Plus",              "Minus",             "Mult",               "Divide",
    "Equal",             "Less",              "Greater",            "OpenBracket",
    "CloseBracket",      "Dot",               "Comma",              "OpenParenthesis",
    "CloseParenthesis",  "Colon",             "Semicolon",          "Pointer",
    "ShiftLeft",         "ShiftRight",        "Power",              "NotEqual",
    "SymmetricDiff",     "LessEqual",         "GreaterEqual",       "Assign",
    "PlusAssign",        "MinusAssign",       "MultAssign",         "DivideAssign",
    "Absolute",          "And",               "Array",              "Asm",
    "Begin",             "Case",              "Const",              "Div",
    "Do",                "Downto",            "Else",               "End",
    "File",              "For",               "Function",           "Goto",
    "If",                "In",                "inline_op",             "Label",
    "Mod",               "Nil",               "Not",                "Of",
    "Or",                "Packed",            "Procedure",          "Program",
    "Record",            "Repeat",            "Set",                "String",
    "Then",              "To",                "Type",               "Unit",
    "Until",             "Uses",              "Var",                "While",
    "With",              "Xor",               "Range",              "operation",
    "CharConst",         "end_of_file",         "Write",              "Read",
    "Break",             "Continue"
};

const std::unordered_set<std::string> tokenizer::token::char_operators = { "shl", "shr", "xor", "mod", "div", "not", "or", "and" };

tokenizer::tokenizer(const std::string file) {
	this->file_ = std::ifstream(file);
	this->file_ >> std::noskipws;
}

tokenizer::tokenizer(std::ifstream&& file) {
	this->file_ = move(file);
	this->file_ >> std::noskipws;
}

tokenizer::tokenizer(tokenizer&& other) noexcept {
    *this = std::move(other);
}

tokenizer& tokenizer::operator=(tokenizer&& other) noexcept {
    std::swap(current_index_, other.current_index_);
    swap(tokens_, other.tokens_);
    swap(file_, other.file_);
    std::swap(state_, other.state_);
    std::swap(row_, other.row_);
    std::swap(column_, other.column_);
    return *this;
}

tokenizer::token_p tokenizer::next() {
	if (current_index_ + 1 < tokens_.size())
		return tokens_[++current_index_];
    if (is_end()) {
        current_index_ = tokens_.size();
        end_token_->position_ = std::make_pair(row_, column_);
        return end_token_;
    }
	char c;
	std::string s = "";
    std::string raw_string = "";
    std::string char_code = "";
    auto cstate = state_;
	while (!file_.eof() && (file_ >> c)) {
        state_ = finite_automata::finite_automata[static_cast<unsigned int>(state_)][c < 0 ? 129 : tolower(c)];
        switch (state_) {
        case finite_automata::states::string_end:
            raw_string += c;
        case finite_automata::states::whitespace:
        case finite_automata::states::comment:
        case finite_automata::states::comment_multiline:
        case finite_automata::states::asterisk:
            ++column_;
            continue;
        case finite_automata::states::comment_new_line:
        case finite_automata::states::new_line:
            ++row_;
            column_ = 1;
            continue;
        case finite_automata::states::return_int:
            file_.putback(c);
            file_.putback(s.back());
            s.pop_back();
            raw_string.pop_back();
            cstate = finite_automata::states::decimal;
            goto tokenEnd;
        case finite_automata::states::begin_multiline_comment:
        case finite_automata::states::begin_comment:
            s.pop_back();
            raw_string.pop_back();
            cstate = finite_automata::states::token_end;
            continue;
        case finite_automata::states::decimal_char_code:
        case finite_automata::states::bin_char_code:
        case finite_automata::states::hex_char_code:
        case finite_automata::states::oct_char_code:
            cstate = state_;
            raw_string += c;
            char_code += c;
            ++column_;
            continue;
        case finite_automata::states::char_s:
        case finite_automata::states::string_start:
            if (char_code.length()) {
                s += code_to_char(cstate, char_code.c_str());
                char_code = "";
            }
            ++column_;
            raw_string += c;
            continue;
        case finite_automata::states::token_end:
            if (char_code.length()) {
                s += code_to_char(cstate, char_code.c_str());
                char_code = "";
            }
            file_.putback(c);
        case finite_automata::states::end:
            goto tokenEnd;
        default:
            try_throw_exception(state_, c);
            break;
        }
        cstate = state_;
        ++column_;
		s += tolower(c);
        raw_string += c;
	}
    if (file_.eof()) {
        try_throw_exception(finite_automata::finite_automata[static_cast<unsigned int>(state_)][128], c);
        if (cstate == finite_automata::states::end || cstate == finite_automata::states::token_end) {
            current_index_ = tokens_.size();
            end_token_->position_ = std::make_pair(row_, column_);
            return end_token_;
        }
    }
tokenEnd:
    if (char_code.length())
        s += code_to_char(cstate, char_code.c_str());
	tokens_.push_back(std::make_shared<token>(std::make_pair(row_, column_ - raw_string.length()), s, cstate, raw_string));
	++current_index_;
	return tokens_.back();
}

bool tokenizer::is_end() const {
	return file_.eof();
}

tokenizer::token_p tokenizer::get_end_token() const {
    return end_token_;
}

void tokenizer::back(const int i = 1) {
    current_index_ -= i;
}

int tokenizer::code_to_char(const finite_automata::states state, const char* char_code) {
    try {
        switch (state) {
        case finite_automata::states::decimal_char_code:
            return std::stoi(char_code, nullptr, 10);
        case finite_automata::states::bin_char_code:
            return std::stoi(char_code + 1, nullptr, 2);
        case finite_automata::states::hex_char_code:
            return std::stoi(char_code + 1, nullptr, 16);
        case finite_automata::states::oct_char_code:
            return std::stoi(char_code + 1, nullptr, 8);
        default:
            throw std::exception();
        }
    }
    catch (std::out_of_range) {
        throw overflow_exception(char_code, std::make_pair(row_, column_), "Integer");
    }
}

void tokenizer::try_throw_exception(const finite_automata::states state, const char c) const {
    switch (state) {
    case finite_automata::states::unknown_symbol:
        throw unknown_symbol_exception(c, std::make_pair(row_, column_));
    case finite_automata::states::unexpected_symbol:
        throw unexpected_symbol_exception(c, std::make_pair(row_, column_));
    case finite_automata::states::eol_while_parsing_string:
        throw eol_while_parsing_string_exception(c, std::make_pair(row_, column_));
    case finite_automata::states::scale_factor_expected:
        throw scale_factor_expected_exception(c, std::make_pair(row_, column_));
    case finite_automata::states::unexpected_end_of_file:
        throw unexpected_end_of_file_exception(c, std::make_pair(row_, column_));
    case finite_automata::states::number_expected:
        throw number_expected_exception(c, std::make_pair(row_, column_));
    case finite_automata::states::fractional_part_expected:
        throw fractional_part_expected_exception(c, std::make_pair(row_, column_));
    default:
        return;
    }
}

std::string tokenizer::token::escape(std::string string) {
    string = regex_replace(string, std::regex("(\\n)"), "\\n");
    string = regex_replace(string, std::regex("(\\r)"), "\\r");
    string = regex_replace(string, std::regex("(\\t)"), "\\t");
    return string;
}

tokenizer::token_p tokenizer::current() const {
    if (current_index_ >= tokens_.size())
        return end_token_;
	return tokens_[current_index_];
}

tokenizer::token_p tokenizer::first() {
	if (current_index_ >= tokens_.size())
		return end_token_;
	current_index_ = 0;
	return current();
}

tokenizer::token& tokenizer::token::operator=(const token& other) {
	position_ = other.position_;
	type_ = other.type_;
    sub_type_ = other.sub_type_;
	string_ = other.string_;
	if (other.string_used_)
		save_string(other.value_.string);
	else
		value_ = other.value_;
	string_used_ = other.string_used_;
	return *this;
}

tokenizer::token& tokenizer::token::operator=(token&& other) noexcept {
	swap(position_, other.position_);
	std::swap(type_, other.type_);
    std::swap(sub_type_, other.sub_type_);
	swap(string_, other.string_);
	std::swap(value_, other.value_);
	std::swap(string_used_, other.string_used_);
	return *this;
}

bool tokenizer::token::operator==(const token& other) const {
	return type_ == other.type_ && sub_type_ == other.sub_type_ && string_ == other.string_ && position_ == other.position_;
}

bool tokenizer::token::operator!=(const token& other) const {
	return !(*this == other);
}

tokenizer::token::~token() {
	if (string_used_)
		delete value_.string;
}

const std::pair<int, int>& tokenizer::token::get_position() const {
	return position_;
}

tokenizer::token::sub_types tokenizer::token::get_sub_type() const {
	return sub_type_;
}

tokenizer::token::types tokenizer::token::get_type() const {
	return type_;
}

const std::string& tokenizer::token::get_string() const {
	return string_;
}

const char* tokenizer::token::get_string_value() const {
	return value_.string;
}

long long tokenizer::token::get_long_long_value() const {
	return value_.long_long;
}

long double tokenizer::token::get_long_double_value() const {
	return value_.real;
}

std::string tokenizer::token::get_value_string() const {
    switch (type_) {
    case types::integer:
        return std::string(std::to_string(value_.long_long));
    case types::real:
        return std::string(std::to_string(value_.real));
    default:
        return std::string(string_used_ ? escape(value_.string) : "");
    }
}

std::string tokenizer::token::to_string() const {
    return str(boost::format("(%1%,%2%)%|10t|%|3$-20|%|4$-30|%|5$-30|%|6$-30|") %
        position_.first % position_.second % types_strings[static_cast<unsigned int>(type_)] %
        sub_types_strings[static_cast<unsigned int>(sub_type_)] % get_value_string() % string_
    );
}

void tokenizer::token::save_string(const std::string& string) {
	if (string_used_)
		delete value_.string;
	value_.string = new char[string.length() + 1];
	memcpy(value_.string, string.c_str(), string.length() + 1);
	string_used_ = true;
}

void tokenizer::token::save_string(const char* string) {
	if (string_used_)
		delete value_.string;
    const int size = strlen(string);
	value_.string = new char[size + 1];
	memcpy(value_.string, string, size + 1);
	string_used_ = true;
}

tokenizer::token::token(const std::pair<int, int> position, const std::string& string, const finite_automata::states state, const std::string& raw_string) {
	position_ = position;
	string_ = raw_string;
    try {
        switch (state) {
        case finite_automata::states::identifier:
        {
            const auto it = token_sub_types.find(string);
            if (it != token_sub_types.end()) {
                sub_type_ = it->second;
                const auto sit = char_operators.find(string);
                if (sit != char_operators.end())
                    type_ = types::operation;
                else
                    type_ = types::reserved_word;
            }
            else {
                sub_type_ = sub_types::identifier;
                type_ = types::identifier;
            }
            save_string(string);
            break;
        }
        case finite_automata::states::return_int:
        case finite_automata::states::decimal:
            value_.long_long = std::stoull(string.c_str(), nullptr, 10);
            type_ = types::integer;
            sub_type_ = sub_types::integer_const;
            break;
        case finite_automata::states::hex:
            value_.long_long = std::stoull(string.c_str() + 1, nullptr, 16);
            type_ = types::integer;
            sub_type_ = sub_types::integer_const;
            break;
        case finite_automata::states::oct:
            value_.long_long = std::stoull(string.c_str() + 1, nullptr, 8);
            type_ = types::integer;
            sub_type_ = sub_types::integer_const;
            break;
        case finite_automata::states::bin:
            value_.long_long = std::stoull(string.c_str() + 1, nullptr, 2);
            type_ = types::integer;
            sub_type_ = sub_types::integer_const;
            break;
        case finite_automata::states::float_end:
        case finite_automata::states::real:
            type_ = types::real;
            sub_type_ = sub_types::real_const;
            value_.real = std::stold(string.c_str(), nullptr);
            break;
        case finite_automata::states::string_end:
        case finite_automata::states::string:
        case finite_automata::states::decimal_char_code:
        case finite_automata::states::hex_char_code:
        case finite_automata::states::oct_char_code:
        case finite_automata::states::bin_char_code:
            if (string.length() > 1) {
                type_ = types::string;
                sub_type_ = sub_types::string_const;
            }
            else {
                type_ = types::symbol;
                sub_type_ = sub_types::char_const;
            }
            save_string(string);
            break;
        case finite_automata::states::separator:
        case finite_automata::states::parenthesis:
        case finite_automata::states::colon:
            sub_type_ = token_sub_types.find(string)->second;
            type_ = types::separator;
            save_string(string);
            break;
        case finite_automata::states::slash:
        case finite_automata::states::operator_less:
        case finite_automata::states::operator_greater:
        case finite_automata::states::operator_plus:
        case finite_automata::states::operator_mult:
        case finite_automata::states::operator_dot:
        case finite_automata::states::operation:
            sub_type_ = token_sub_types.find(string)->second;
            type_ = types::operation;
            save_string(string);
            break;
        case finite_automata::states::end:
        case finite_automata::states::token_end:
            type_ = types::end_of_file;
            sub_type_ = sub_types::end_of_file;
            break;
        default:
            throw std::exception();
        }
    }
    catch (std::out_of_range) {
        throw overflow_exception(raw_string, position, 
            state == finite_automata::states::real || state == finite_automata::states::float_end ? "Real" : "Integer");
    }
}

tokenizer::token::token(const token& other) {
	*this = other;
}

tokenizer::token::token(token&& other) noexcept {
	*this = std::move(other);
}


const char* tokenizer::tokenizer_exception::what() const {
    return message_;
}

const char* tokenizer::tokenizer_symbol_exception::init_message(const char* format) {
    if (message_)
        return message_;
    auto b_message = str(boost::format(format) % symbol_ % position_.first % position_.second);
    const int size = strlen(b_message.c_str()) + 1;
    message_ = new char[size];
    memcpy(message_, b_message.c_str(), size);
    return message_;
}

tokenizer::overflow_exception::overflow_exception(const std::string& string, const std::pair<int, int>& position, const char* type) : tokenizer_exception(position) {
    auto bm = str(boost::format("%1% overflow \"%2%\" at (%3%, %4%)") % type % string % position.first  % position.second);
    const int size = strlen(bm.c_str()) + 1;
    message_ = new char[size];
    memcpy(message_, bm.c_str(), size);
}
