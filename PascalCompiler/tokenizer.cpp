#include "tokenizer.hpp"
#include <cstdio>
#include <iostream>

const std::unordered_map<std::string, My::Tokenizer::Token::SubTypes> My::Tokenizer::Token::TokenSubTypes = {
	{ "and",  My::Tokenizer::Token::SubTypes::And },           { "array", My::Tokenizer::Token::SubTypes::Array },        { "begin", My::Tokenizer::Token::SubTypes::Begin },          { "case", My::Tokenizer::Token::SubTypes::Case },
	{ "const", My::Tokenizer::Token::SubTypes::Const },        { "div", My::Tokenizer::Token::SubTypes::Div },            { "do", My::Tokenizer::Token::SubTypes::Do },                { "downto", My::Tokenizer::Token::SubTypes::Downto },
	{ "else", My::Tokenizer::Token::SubTypes::Else },          { "end", My::Tokenizer::Token::SubTypes::End },            { "file", My::Tokenizer::Token::SubTypes::File },            { "for", My::Tokenizer::Token::SubTypes::For },
	{ "function", My::Tokenizer::Token::SubTypes::Function },  { "goto", My::Tokenizer::Token::SubTypes::Goto },          { "if", My::Tokenizer::Token::SubTypes::If },                { "in", My::Tokenizer::Token::SubTypes::In },
	{ "label", My::Tokenizer::Token::SubTypes::Label },        { "mod", My::Tokenizer::Token::SubTypes::Mod },            { "nil", My::Tokenizer::Token::SubTypes::Nil },              { "not", My::Tokenizer::Token::SubTypes::Not },
	{ "of", My::Tokenizer::Token::SubTypes::Of },              { "packed", My::Tokenizer::Token::SubTypes::Packed },      { "procedure", My::Tokenizer::Token::SubTypes::Procedure },  { "program", My::Tokenizer::Token::SubTypes::Program },
	{ "record", My::Tokenizer::Token::SubTypes::Record },      { "repeat", My::Tokenizer::Token::SubTypes::Repeat },      { "set", My::Tokenizer::Token::SubTypes::Set },              { "then", My::Tokenizer::Token::SubTypes::Then },
	{ "to", My::Tokenizer::Token::SubTypes::To },              { "type", My::Tokenizer::Token::SubTypes::Type },          { "until", My::Tokenizer::Token::SubTypes::Until },          { "var", My::Tokenizer::Token::SubTypes::Var },
	{ "while", My::Tokenizer::Token::SubTypes::While },        { "with", My::Tokenizer::Token::SubTypes::With },          { "+", My::Tokenizer::Token::SubTypes::Plus },               { "-", My::Tokenizer::Token::SubTypes::Minus },
	{ "*", My::Tokenizer::Token::SubTypes::Mult },             { "/", My::Tokenizer::Token::SubTypes::Divide },           { "=", My::Tokenizer::Token::SubTypes::Equal },              { "<", My::Tokenizer::Token::SubTypes::Less },
	{ ">", My::Tokenizer::Token::SubTypes::Greater },          { "@", My::Tokenizer::Token::SubTypes::Adress },           { "^", My::Tokenizer::Token::SubTypes::Adress },             { "<>", My::Tokenizer::Token::SubTypes::NotEqual },
	{ "<=", My::Tokenizer::Token::SubTypes::LessOrEqual },     { ">=", My::Tokenizer::Token::SubTypes::GreaterOrEqual },  { ":=", My::Tokenizer::Token::SubTypes::Assign },            { "..", My::Tokenizer::Token::SubTypes::Range },
	{ "[", My::Tokenizer::Token::SubTypes::OpenBracket },      { "]", My::Tokenizer::Token::SubTypes::CloseBracket },     { ",", My::Tokenizer::Token::SubTypes::Comma },              { ":", My::Tokenizer::Token::SubTypes::Colon },
	{ ";", My::Tokenizer::Token::SubTypes::Semicolon },        { "(", My::Tokenizer::Token::SubTypes::OpenParenthesis },  { ")", My::Tokenizer::Token::SubTypes::CloseParenthesis },   { ".", My::Tokenizer::Token::SubTypes::ProgramEnd }
};

const std::unordered_map<std::string, My::Tokenizer::Token::Types> My::Tokenizer::Token::TokenTypes = {
	{ "+", My::Tokenizer::Token::Types::Operator },         { "-", My::Tokenizer::Token::Types::Operator },         { "(", My::Tokenizer::Token::Types::Separator },          { ")", My::Tokenizer::Token::Types::Separator },
	{ "*", My::Tokenizer::Token::Types::Operator },         { "/", My::Tokenizer::Token::Types::Operator },         { "=", My::Tokenizer::Token::Types::Operator },           { "<", My::Tokenizer::Token::Types::Operator },
	{ ">", My::Tokenizer::Token::Types::Operator },         { "@", My::Tokenizer::Token::Types::Operator },         { "^", My::Tokenizer::Token::Types::Operator },           { "<>", My::Tokenizer::Token::Types::Operator },
	{ "<=", My::Tokenizer::Token::Types::Operator },        { ">=", My::Tokenizer::Token::Types::Operator },        { ":=", My::Tokenizer::Token::Types::Operator },          { "..", My::Tokenizer::Token::Types::Operator },
	{ "[", My::Tokenizer::Token::Types::Separator },        { "]", My::Tokenizer::Token::Types::Separator },        { ",", My::Tokenizer::Token::Types::Separator },          { ":", My::Tokenizer::Token::Types::Separator },
	{ ";", My::Tokenizer::Token::Types::Separator },        { "(.", My::Tokenizer::Token::Types::Separator },       { ".)", My::Tokenizer::Token::Types::Separator },         { ".", My::Tokenizer::Token::Types::Separator }
};

const My::Tokenizer::Token My::Tokenizer::emptyToken = Token({ 0, 0 }, "", FiniteAutomata::States::Identifier);

My::Tokenizer::Tokenizer(std::string file) {
	this->file = std::ifstream(file);
	this->file >> std::noskipws;
}

My::Tokenizer::Tokenizer(std::ifstream file) {
	this->file = std::move(file);
	this->file >> std::noskipws;
}

const My::Tokenizer::Token& My::Tokenizer::Next() {
	if (currentIndex < tokens.size() - 1)
		return tokens[++currentIndex];
	if (IsEnd())
		return Current();
	char c;
	std::string s = "";
	FiniteAutomata::States cstate = state;
	while ((file >> c) && !file.eof()) {
		c = tolower(c);
		state = FiniteAutomata::FiniteAutomata[state][c];
		if (state == FiniteAutomata::States::Stop) {
			file.putback(c);
			break;
		}
		if (state == FiniteAutomata::States::ReturnInt) {
			file.putback(c);
			file.putback(s.back());
			s.pop_back();
			break;
		}
		if (state == FiniteAutomata::States::Return) {
			file.putback(c);
			file.putback(s.back());
			s.pop_back();
			continue;
		}
		if (state > FiniteAutomata::States::End)
			switch (state) {
			case FiniteAutomata::States::UnexpectedSymbol:
				throw new UnexpectedSymbolException();
			case FiniteAutomata::States::InvalidSyntax:
				throw new InvalidSyntaxException();
			case FiniteAutomata::States::EOLWhileParsingString:
				throw new EOLWhileParsingStringException();
			default:
				throw new std::exception();
			}
		cstate = state;
		if (state == FiniteAutomata::States::Whitespace ||
			state == FiniteAutomata::States::Comment ||
			state == FiniteAutomata::States::CommentMultiline ||
			state == FiniteAutomata::States::Asterisk ||
            state == FiniteAutomata::States::StringEnd) {
			++column;
			continue;
		}
		if (state == FiniteAutomata::States::NewLine) {
			++row;
			column = 1;
			continue;
		}
		s += c;
	}
	if (file.eof())
		tokens.push_back(Token(std::make_pair(row, column), s, FiniteAutomata::States::End));
	else
		tokens.push_back(Token(std::make_pair(row, column), s, cstate));
	column += s.length();
	++currentIndex;
	return tokens.back();
}

bool My::Tokenizer::IsEnd() {
	return file.eof();
}

const My::Tokenizer::Token& My::Tokenizer::Current() {
	return tokens[currentIndex];
}

const My::Tokenizer::Token& My::Tokenizer::First() {
	if (currentIndex >= tokens.size())
		return emptyToken;
	currentIndex = 0;
	return Current();
}

My::Tokenizer::Token& My::Tokenizer::Token::operator=(const Token& other) {
	myPosition = other.myPosition;
	myType = other.myType;
	myString = other.myString;
	if (other.stringUsed)
		saveString(other.myValue.String);
	else
		myValue = other.myValue;
	stringUsed = other.stringUsed;
	return *this;
}

My::Tokenizer::Token& My::Tokenizer::Token::operator=(Token&& other) {
	std::swap(myPosition, other.myPosition);
	std::swap(myType, other.myType);
	std::swap(myString, other.myString);
	std::swap(myValue, other.myValue);
	std::swap(stringUsed, other.stringUsed);
	return *this;
}

bool My::Tokenizer::Token::operator==(const Token& other) {
	return myString == other.myString;
}

bool My::Tokenizer::Token::operator!=(const Token& other) {
	return !(*this == other);
}

My::Tokenizer::Token::~Token() {
	if (stringUsed)
		delete myValue.String;
}

const std::pair<int, int>& My::Tokenizer::Token::GetPosition() {
	return myPosition;
}

const My::Tokenizer::Token::SubTypes My::Tokenizer::Token::GetSubType() {
	return mySubType;
}

const My::Tokenizer::Token::Types My::Tokenizer::Token::GetType() {
	return myType;
}

const std::string& My::Tokenizer::Token::GetString() {
	return myString;
}

const char* My::Tokenizer::Token::GetStringValue() {
	return myValue.String;
}

const unsigned long long My::Tokenizer::Token::GetLongLongValue() {
	return myValue.UnsignedLongLong;
}

const long double My::Tokenizer::Token::GetLongDoubleValue() {
	return myValue.LongDouble;
}

void My::Tokenizer::Token::saveString(std::string& string) {
	if (stringUsed)
		delete myValue.String;
	myValue.String = new char[string.length() + 1];
	std::memcpy(myValue.String, string.c_str(), string.length() + 1);
	stringUsed = true;
}

void My::Tokenizer::Token::saveString(const char* string) {
	if (stringUsed)
		delete myValue.String;
	int size = std::strlen(string);
	myValue.String = new char[size + 1];
	std::memcpy(myValue.String, string, size + 1);
	stringUsed = true;
}

My::Tokenizer::Token::Token(std::pair<int, int> position, std::string string, FiniteAutomata::States state) {
	myPosition = position;
	myString = string;
	std::unordered_map<std::string, SubTypes>::const_iterator it;
	switch (state) {
	case My::FiniteAutomata::Identifier:
		it = TokenSubTypes.find(string);
		if (it != TokenSubTypes.end()) {
			mySubType = it->second;
			myType = Types::ReservedWord;
		}
		else {
			mySubType = SubTypes::Identifier;
			myType = Types::Identifier;
		}
		saveString(string);
		break;
	case My::FiniteAutomata::ReturnInt:
	case My::FiniteAutomata::Integer:
		myType = Types::Integer;
		mySubType = SubTypes::Integer;
		myValue.UnsignedLongLong = std::strtoull(string.c_str(), NULL, 0);
		break;
	case My::FiniteAutomata::FloatEnd:
	case My::FiniteAutomata::Float:
		myType = Types::Float;
		mySubType = SubTypes::Float;
		myValue.LongDouble = std::strtold(string.c_str(), NULL);
		break;
    case My::FiniteAutomata::StringEnd:
	case My::FiniteAutomata::String:
		myType = Types::String;
		mySubType = SubTypes::String;
		saveString(string);
		break;
	case My::FiniteAutomata::Slash:
	case My::FiniteAutomata::Separator:
	case My::FiniteAutomata::Parenthesis:
	case My::FiniteAutomata::Colon:
	case My::FiniteAutomata::OperatorLess:
	case My::FiniteAutomata::OperatorGreater:
	case My::FiniteAutomata::Operator:
		mySubType = TokenSubTypes.find(string)->second;
		myType = TokenTypes.find(string)->second;
		saveString(string);
		break;
	case My::FiniteAutomata::End:
		myType = Types::EndOfFile;
        break;
	default:
		throw std::exception();
		break;
	}
}

My::Tokenizer::Token::Token(const Token& other) {
	*this = other;
}

My::Tokenizer::Token::Token(Token&& other) {
	*this = std::move(other);
}
