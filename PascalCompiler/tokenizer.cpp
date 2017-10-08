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
	{ ">", My::Tokenizer::Token::SubTypes::Greater },          { "@", My::Tokenizer::Token::SubTypes::Pointer },          { "^", My::Tokenizer::Token::SubTypes::Pointer },            { "<>", My::Tokenizer::Token::SubTypes::NotEqual },
	{ "<=", My::Tokenizer::Token::SubTypes::LessEqual },       { ">=", My::Tokenizer::Token::SubTypes::GreaterEqual },    { ":=", My::Tokenizer::Token::SubTypes::Assign },            { "..", My::Tokenizer::Token::SubTypes::Range },
	{ "[", My::Tokenizer::Token::SubTypes::OpenBracket },      { "]", My::Tokenizer::Token::SubTypes::CloseBracket },     { ",", My::Tokenizer::Token::SubTypes::Comma },              { ":", My::Tokenizer::Token::SubTypes::Colon },
	{ ";", My::Tokenizer::Token::SubTypes::Semicolon },        { "(", My::Tokenizer::Token::SubTypes::OpenParenthesis },  { ")", My::Tokenizer::Token::SubTypes::CloseParenthesis },   { "(.", My::Tokenizer::Token::SubTypes::OpenBracket },
    { ".)", My::Tokenizer::Token::SubTypes::CloseBracket },    { "asm", My::Tokenizer::Token::SubTypes::Asm },            { "<<", My::Tokenizer::Token::SubTypes::ShiftLeft },         { ">>", My::Tokenizer::Token::SubTypes::ShiftRight },
    { "shl", My::Tokenizer::Token::SubTypes::ShiftLeft },      { "shr", My::Tokenizer::Token::SubTypes::ShiftRight },     { "**", My::Tokenizer::Token::SubTypes::Power },             { "><", My::Tokenizer::Token::SubTypes::SymmetricDiff },
    { "+=", My::Tokenizer::Token::SubTypes::PlusAssign },      { "-=", My::Tokenizer::Token::SubTypes::MinusAssign },     { "*=", My::Tokenizer::Token::SubTypes::MultAssign },        { "/=", My::Tokenizer::Token::SubTypes::DivideAssign },
    { "absolute", My::Tokenizer::Token::SubTypes::Absolute },  { "inline", My::Tokenizer::Token::SubTypes::Inline },      { "string", My::Tokenizer::Token::SubTypes::String },        { "unit", My::Tokenizer::Token::SubTypes::Unit },
    { "usus", My::Tokenizer::Token::SubTypes::Uses },          { "xor", My::Tokenizer::Token::SubTypes::Xor }
};

const My::Tokenizer::Token My::Tokenizer::endToken = Token({ 0, 0 }, "", FiniteAutomata::States::End);

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
		return endToken;
	char c;
	std::string s = "";
    std::string charCode = "";
	FiniteAutomata::States cstate = state;
	while ((file >> c) && !file.eof()) {
		c = tolower(c);
		state = FiniteAutomata::FiniteAutomata[state][c - 1];
        switch (state) {
        case My::FiniteAutomata::States::Whitespace:
        case My::FiniteAutomata::States::Comment:
        case My::FiniteAutomata::States::CommentMultiline:
        case My::FiniteAutomata::States::Asterisk:
        case My::FiniteAutomata::States::StringEnd:
        case My::FiniteAutomata::States::OctStart:
        case My::FiniteAutomata::States::HexStart:
        case My::FiniteAutomata::States::BinStart:
        case My::FiniteAutomata::States::Char:
            continue;
        case My::FiniteAutomata::States::NewLine:
            ++row;
            column = 1;
            continue;
        case My::FiniteAutomata::States::ReturnInt:
            file.putback(c);
            file.putback(s.back());
            s.pop_back();
            goto tokenEnd;
        case My::FiniteAutomata::States::Return:
            file.putback(c);
            file.putback(s.back());
            s.pop_back();
            continue;
        case My::FiniteAutomata::States::DecimalCharCode:
        case My::FiniteAutomata::States::BinCharCode:
        case My::FiniteAutomata::States::HexCharCode:
        case My::FiniteAutomata::States::OctCharCode:
            charCode += c;
            continue;
        case My::FiniteAutomata::States::StringStart:
            if (charCode.length()) {
                switch (cstate) {
                case My::FiniteAutomata::States::DecimalCharCode:
                    s += std::strtol(charCode.c_str(), NULL, 10);
                    break;
                case My::FiniteAutomata::States::BinCharCode:
                    s += std::strtol(charCode.c_str(), NULL, 2);
                    break;
                case My::FiniteAutomata::States::HexCharCode:
                    s += std::strtol(charCode.c_str(), NULL, 16);
                    break;
                case My::FiniteAutomata::States::OctCharCode:
                    s += std::strtol(charCode.c_str(), NULL, 8);
                    break;
                default:
                    throw std::exception();
                }
                charCode = "";
            }
            continue;
        case My::FiniteAutomata::States::TokenEnd:
            file.putback(c);
        case My::FiniteAutomata::States::End:
            goto tokenEnd;
        case My::FiniteAutomata::UnknownSymbol:
            throw UnknownSymbolException();
        case My::FiniteAutomata::UnexpectedSymbol:
            throw UnexpectedSymbolException();
        case My::FiniteAutomata::EOLWhileParsingString:
            throw EOLWhileParsingStringException();
        case My::FiniteAutomata::ScaleFactorExpected:
            throw ScaleFactorExpectedException();
        case My::FiniteAutomata::UnexpectedEndOfFile:
            throw UnexpectedEndOfFileException();
        case My::FiniteAutomata::NumberExpected:
            throw NumberExpectedException();
        case My::FiniteAutomata::FractionalPartExpected:
            throw FractionalPartExpectedException();
        default:
            break;
        }
        cstate = state;
        ++column;
		s += c;
	}
    tokenEnd:
	tokens.push_back(Token(std::make_pair(row, column - s.length()), s, cstate));
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
    mySubType = other.mySubType;
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
    std::swap(mySubType, other.mySubType);
	std::swap(myString, other.myString);
	std::swap(myValue, other.myValue);
	std::swap(stringUsed, other.stringUsed);
	return *this;
}

bool My::Tokenizer::Token::operator==(const Token& other) {
	return myType == other.myType && mySubType == other.mySubType && myString == other.myString;
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
            if (string == "shl" || string == "shr")
                myType = Types::Operator;
            else
			    myType = Types::ReservedWord;
		}
		else {
			mySubType = SubTypes::Identifier;
			myType = Types::Identifier;
		}
		saveString(string);
		break;
	case My::FiniteAutomata::ReturnInt:
	case My::FiniteAutomata::Decimal:
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
		mySubType = SubTypes::StringConst;
		saveString(string);
		break;
    case My::FiniteAutomata::Separator:
    case My::FiniteAutomata::Parenthesis:
    case My::FiniteAutomata::Colon:
        mySubType = TokenSubTypes.find(string)->second;
        myType = Types::Separator;
        saveString(string);
        break;
	case My::FiniteAutomata::Slash:
	case My::FiniteAutomata::OperatorLess:
	case My::FiniteAutomata::OperatorGreater:
    case My::FiniteAutomata::OperatorPlus:
    case My::FiniteAutomata::OperatorMult:
	case My::FiniteAutomata::Operator:
		mySubType = TokenSubTypes.find(string)->second;
        myType = Types::Operator;
        saveString(string);
		break;
	case My::FiniteAutomata::End:
		myType = Types::EndOfFile;
        break;
	default:
		throw std::exception();
	}
}

My::Tokenizer::Token::Token(const Token& other) {
	*this = other;
}

My::Tokenizer::Token::Token(Token&& other) {
	*this = std::move(other);
}
