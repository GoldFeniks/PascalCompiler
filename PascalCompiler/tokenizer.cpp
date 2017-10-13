#include "tokenizer.hpp"
#include <cstdio>
#include <iostream>
#include "boost\format.hpp"
#include <regex>

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
    { "uses", My::Tokenizer::Token::SubTypes::Uses },          { "xor", My::Tokenizer::Token::SubTypes::Xor },            { "operator", My::Tokenizer::Token::SubTypes::Operator },    { ".", My::Tokenizer::Token::SubTypes::Dot },
    { "or", My::Tokenizer::Token::SubTypes::Or }
};

const std::string My::Tokenizer::Token::TypesStrings[] = {
    "Identifier", "Integer", "Float", "Char", "String", "Operator", "Separator", "ReservedWord", "EndOfFile"
};

const std::string My::Tokenizer::Token::SubTypesStrings[] = {

    "Identifier",        "IntegerConst",      "FloatConst",         "StringConst",
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
    "If",                "In",                "Inline",             "Label",
    "Mod",               "Nil",               "Not",                "Of",
    "Or",                "Packed",            "Procedure",          "Program",
    "Record",            "Repeat",            "Set",                "String",
    "Then",              "To",                "Type",               "Unit",
    "Until",             "Uses",              "Var",                "While",
    "With",              "Xor",               "Range",              "Operator",
    "CharConst",         "EndOfFile",

};

const std::unordered_set<std::string> My::Tokenizer::Token::CharOperators = { "shl", "shr", "xor", "mod", "div", "not", "or", "and" };

My::Tokenizer::Tokenizer(const std::string file) {
	this->file = std::ifstream(file);
	this->file >> std::noskipws;
}

My::Tokenizer::Tokenizer(std::ifstream&& file) {
	this->file = std::move(file);
	this->file >> std::noskipws;
}

My::Tokenizer::Tokenizer(Tokenizer&& other) {
    *this = std::move(other);
}

My::Tokenizer& My::Tokenizer::operator=(Tokenizer&& other) {
    std::swap(currentIndex, other.currentIndex);
    std::swap(tokens, other.tokens);
    std::swap(file, other.file);
    std::swap(state, other.state);
    std::swap(row, other.row);
    std::swap(column, other.column);
    return *this;
}

const My::Tokenizer::PToken My::Tokenizer::Next() {
	if (currentIndex + 1 < tokens.size())
		return tokens[++currentIndex];
    if (IsEnd()) {
        currentIndex = tokens.size();
        endToken->myPosition = std::make_pair(row, column);
        return endToken;
    }
	char c;
	std::string s = "";
    std::string rawString = "";
    std::string charCode = "";
	FiniteAutomata::States cstate = state;
	while (!file.eof() && (file >> c)) {
        state = My::FiniteAutomata::FiniteAutomata[static_cast<unsigned int>(state)][c < 0 ? 129 : tolower(c)];
        switch (state) {
        case My::FiniteAutomata::States::StringEnd:
            rawString += c;
        case My::FiniteAutomata::States::Whitespace:
        case My::FiniteAutomata::States::Comment:
        case My::FiniteAutomata::States::CommentMultiline:
        case My::FiniteAutomata::States::Asterisk:
            ++column;
            continue;
        case My::FiniteAutomata::States::CommentNewLine:
        case My::FiniteAutomata::States::NewLine:
            ++row;
            column = 1;
            continue;
        case My::FiniteAutomata::States::ReturnInt:
            file.putback(c);
            file.putback(s.back());
            s.pop_back();
            rawString.pop_back();
            cstate = My::FiniteAutomata::States::Decimal;
            goto tokenEnd;
        case My::FiniteAutomata::States::BeginMultilineComment:
        case My::FiniteAutomata::States::BeginComment:
            s.pop_back();
            rawString.pop_back();
            cstate = My::FiniteAutomata::States::TokenEnd;
            continue;
        case My::FiniteAutomata::States::DecimalCharCode:
        case My::FiniteAutomata::States::BinCharCode:
        case My::FiniteAutomata::States::HexCharCode:
        case My::FiniteAutomata::States::OctCharCode:
            cstate = state;
            rawString += c;
            charCode += c;
            ++column;
            continue;
        case My::FiniteAutomata::States::Char:
        case My::FiniteAutomata::States::StringStart:
            if (charCode.length()) {
                s += codeToChar(cstate, charCode.c_str());
                charCode = "";
            }
            ++column;
            rawString += c;
            continue;
        case My::FiniteAutomata::States::TokenEnd:
            if (charCode.length()) {
                s += codeToChar(cstate, charCode.c_str());
                charCode = "";
            }
            file.putback(c);
        case My::FiniteAutomata::States::End:
            goto tokenEnd;
        default:
            tryThrowException(state, c);
            break;
        }
        cstate = state;
        ++column;
		s += c;
        rawString += c;
	}
    if (file.eof()) {
        tryThrowException(My::FiniteAutomata::FiniteAutomata[static_cast<unsigned int>(state)][128], c);
        if (cstate == FiniteAutomata::States::End || cstate == FiniteAutomata::States::TokenEnd) {
            currentIndex = tokens.size();
            endToken->myPosition = std::make_pair(row, column);
            return endToken;
        }
    }
tokenEnd:
	tokens.push_back(PToken(new Token(std::make_pair(row, column - rawString.length()), s, cstate, rawString)));
	++currentIndex;
	return tokens.back();
}

bool My::Tokenizer::IsEnd() const {
	return file.eof();
}

const My::Tokenizer::PToken My::Tokenizer::GetEndToken() const {
    return endToken;
}

long int My::Tokenizer::codeToChar(My::FiniteAutomata::States state, const char* charCode) {
    switch (state) {
    case My::FiniteAutomata::States::DecimalCharCode:
        return std::strtol(charCode, NULL, 10);
        break;
    case My::FiniteAutomata::States::BinCharCode:
        return std::strtol(charCode + 1, NULL, 2);
        break;
    case My::FiniteAutomata::States::HexCharCode:
        return std::strtol(charCode + 1, NULL, 16);
        break;
    case My::FiniteAutomata::States::OctCharCode:
        return std::strtol(charCode + 1, NULL, 8);
        break;
    default:
        throw std::exception();
    }
}

void My::Tokenizer::tryThrowException(My::FiniteAutomata::States state, char c) const {
    switch (state) {
    case My::FiniteAutomata::States::UnknownSymbol:
        throw UnknownSymbolException(c, std::make_pair(row, column));
    case My::FiniteAutomata::States::UnexpectedSymbol:
        throw UnexpectedSymbolException(c, std::make_pair(row, column));
    case My::FiniteAutomata::States::EOLWhileParsingString:
        throw EOLWhileParsingStringException(c, std::make_pair(row, column));
    case My::FiniteAutomata::States::ScaleFactorExpected:
        throw ScaleFactorExpectedException(c, std::make_pair(row, column));
    case My::FiniteAutomata::States::UnexpectedEndOfFile:
        throw UnexpectedEndOfFileException(c, std::make_pair(row, column));
    case My::FiniteAutomata::States::NumberExpected:
        throw NumberExpectedException(c, std::make_pair(row, column));
    case My::FiniteAutomata::States::FractionalPartExpected:
        throw FractionalPartExpectedException(c, std::make_pair(row, column));
    default:
        return;
    }
}

std::string My::Tokenizer::Token::escape(std::string string) {
    string = std::regex_replace(string, std::regex("(\\n)"), "\\n");
    string = std::regex_replace(string, std::regex("(\\r)"), "\\r");
    string = std::regex_replace(string, std::regex("(\\t)"), "\\t");
    return string;
}

const My::Tokenizer::PToken My::Tokenizer::Current() const {
    if (currentIndex >= tokens.size())
        return endToken;
	return tokens[currentIndex];
}

const My::Tokenizer::PToken My::Tokenizer::First() {
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
	return myType == other.myType && mySubType == other.mySubType && myString == other.myString && myPosition == other.myPosition;
}

bool My::Tokenizer::Token::operator!=(const Token& other) {
	return !(*this == other);
}

My::Tokenizer::Token::~Token() {
	if (stringUsed)
		delete myValue.String;
}

const std::pair<int, int>& My::Tokenizer::Token::GetPosition() const {
	return myPosition;
}

const My::Tokenizer::Token::SubTypes My::Tokenizer::Token::GetSubType() const {
	return mySubType;
}

const My::Tokenizer::Token::Types My::Tokenizer::Token::GetType() const {
	return myType;
}

const std::string& My::Tokenizer::Token::GetString() const {
	return myString;
}

const char* My::Tokenizer::Token::GetStringValue() const {
	return myValue.String;
}

const unsigned long long My::Tokenizer::Token::GetLongLongValue() const {
	return myValue.UnsignedLongLong;
}

const long double My::Tokenizer::Token::GetLongDoubleValue() const {
	return myValue.Double;
}

std::string My::Tokenizer::Token::GetValueString() const {
    switch (myType) {
    case Types::Integer:
        return std::string(std::to_string(myValue.UnsignedLongLong));
    case Types::Float:
        return std::string(std::to_string(myValue.Double));
    default:
        return std::string(stringUsed ? escape(myValue.String) : "");
    }
}

const std::string My::Tokenizer::Token::ToString() const {
    return boost::str(boost::format("(%1%,%2%)%|10t|%|3$-20|%|4$-30|%|5$-30|%|6$-30|") %
        myPosition.first % myPosition.second % TypesStrings[static_cast<unsigned int>(myType)] %
        SubTypesStrings[static_cast<unsigned int>(mySubType)] % GetValueString() % myString
    );
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

My::Tokenizer::Token::Token(std::pair<int, int> position, std::string string, FiniteAutomata::States state, std::string rawString) {
	myPosition = position;
	myString = rawString;
	std::unordered_map<std::string, SubTypes>::const_iterator it;
    std::unordered_set<std::string>::const_iterator sit;
	switch (state) {
	case My::FiniteAutomata::States::Identifier:
		it = TokenSubTypes.find(string);
		if (it != TokenSubTypes.end()) {
			mySubType = it->second;
            sit = CharOperators.find(string);
            if (sit != CharOperators.end())
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
	case My::FiniteAutomata::States::ReturnInt:
	case My::FiniteAutomata::States::Decimal:
		myValue.UnsignedLongLong = std::strtoull(string.c_str(), NULL, 10);
        myType = Types::Integer;
        mySubType = SubTypes::IntegerConst;
        break;
    case My::FiniteAutomata::States::Hex:
        myValue.UnsignedLongLong = std::strtoull(string.c_str() + 1, NULL, 16);
        myType = Types::Integer;
        mySubType = SubTypes::IntegerConst;
        break;
    case My::FiniteAutomata::States::Oct:
        myValue.UnsignedLongLong = std::strtoull(string.c_str() + 1, NULL, 8);
        myType = Types::Integer;
        mySubType = SubTypes::IntegerConst;
        break;
    case My::FiniteAutomata::States::Bin:
        myValue.UnsignedLongLong = std::strtoull(string.c_str() + 1, NULL, 2);
        myType = Types::Integer;
        mySubType = SubTypes::IntegerConst;
        break;
	case My::FiniteAutomata::States::FloatEnd:
	case My::FiniteAutomata::States::Float:
		myType = Types::Float;
		mySubType = SubTypes::FloatConst;
		myValue.Double = std::strtold(string.c_str(), NULL);
		break;
    case My::FiniteAutomata::States::StringEnd:
	case My::FiniteAutomata::States::String:
    case My::FiniteAutomata::States::DecimalCharCode:
    case My::FiniteAutomata::States::HexCharCode:
    case My::FiniteAutomata::States::OctCharCode:
    case My::FiniteAutomata::States::BinCharCode:
        if (string.length() > 1) {
            myType = Types::String;
            mySubType = SubTypes::StringConst;
        }
        else {
            myType = Types::Char;
            mySubType = SubTypes::CharConst;
        }
        saveString(string);
        break;
    case My::FiniteAutomata::States::Separator:
    case My::FiniteAutomata::States::Parenthesis:
    case My::FiniteAutomata::States::Colon:
        mySubType = TokenSubTypes.find(string)->second;
        myType = Types::Separator;
        saveString(string);
        break;
	case My::FiniteAutomata::States::Slash:
	case My::FiniteAutomata::States::OperatorLess:
	case My::FiniteAutomata::States::OperatorGreater:
    case My::FiniteAutomata::States::OperatorPlus:
    case My::FiniteAutomata::States::OperatorMult:
    case My::FiniteAutomata::States::OperatorDot:
	case My::FiniteAutomata::States::Operator:
		mySubType = TokenSubTypes.find(string)->second;
        myType = Types::Operator;
        saveString(string);
		break;
    case My::FiniteAutomata::States::End:
    case My::FiniteAutomata::States::TokenEnd:
		myType = Types::EndOfFile;
        mySubType = SubTypes::EndOfFile;
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


const char* My::Tokenizer::TokenizerException::what() const {
    return message;
}

const char* My::Tokenizer::TokenizerException::initMessage(const char* format) {
    if (message)
        return message;
    auto bMessage = boost::str(boost::format(format) % symbol % position.first % position.second);
    const int size = std::strlen(bMessage.c_str()) + 1;
    message = new char[size];
    std::memcpy(message, bMessage.c_str(), size);
    return message;
}
