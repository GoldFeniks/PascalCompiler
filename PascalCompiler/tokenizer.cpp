#include "tokenizer.hpp"
#include <cstdio>
#include <iostream>

const My::Tokenizer::Token My::Tokenizer::endToken = Token({ 0, 0 }, "", FiniteAutomata::States::Identifier);

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
		if (state > FiniteAutomata::States::Count)
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
			state == FiniteAutomata::States::CommentMultiline) {
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
		return endToken;
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

const My::Tokenizer::Token::Types& My::Tokenizer::Token::GetType() {
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
	std::unordered_map<std::string, Types>::const_iterator it;
	switch (state) {
	case My::FiniteAutomata::Identifier:
		it = KeyWords.find(string);
		if (it != KeyWords.end())
			myType = it->second;
		else
			myType = Types::Identifier;
		saveString(string);
		break;
	case My::FiniteAutomata::ReturnInt:
	case My::FiniteAutomata::Integer:
		myType = Types::Integer;
		myValue.UnsignedLongLong = std::strtoull(string.c_str(), NULL, 0);
		break;
	case My::FiniteAutomata::FloatEnd:
	case My::FiniteAutomata::Float:
		myType = Types::Float;
		myValue.LongDouble = std::strtold(string.c_str(), NULL);
		break;
	case My::FiniteAutomata::String:
		myType = Types::String;
		saveString(string);
		break;
	case My::FiniteAutomata::Operator:
		myType = Operators.find(string)->second;
		saveString(string);
		break;
	case My::FiniteAutomata::OperatorGreater:
		myType = Operators.find(string)->second;
		saveString(string);
		break;
	case My::FiniteAutomata::OperatorLess:
		myType = Operators.find(string)->second;
		saveString(string);
		break;
	case My::FiniteAutomata::Colon:
		myType = Types::Colon;
		saveString(string);
		break;
	case My::FiniteAutomata::Parenthesis:
	case My::FiniteAutomata::Separator:
		myType = Separators.find(string)->second;
		saveString(string);
		break;
	case My::FiniteAutomata::Slash:
		myType = Operators.find(string)->second;
		saveString(string);
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
