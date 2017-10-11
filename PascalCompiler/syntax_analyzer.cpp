#include "syntax_analyzer.hpp"
#include "boost/format.hpp"
#include <iomanip>

My::SyntaxAnalyzer::SyntaxErrorException::SyntaxErrorException(const My::Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type) {
    message = std::string(boost::str(boost::format("(%4%, %5%) Syntax Error: %1% expected but %2% \"%3%\" found") %
        My::Tokenizer::Token::SubTypesStrings[static_cast<unsigned int>(type)] %
        My::Tokenizer::Token::TypesStrings[static_cast<unsigned int>(token->GetType())] %
        token->GetValueString() % token->GetPosition().first % token->GetPosition().second
    ));
}

const char* My::SyntaxAnalyzer::SyntaxErrorException::what() const {
    return message.c_str();
}

void My::SyntaxAnalyzer::require(const Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type) {
    if (token->GetSubType() != type)
        throw SyntaxErrorException(token, type);
}

std::string My::SyntaxAnalyzer::walk(Node::PNode node, std::string prefix, bool last) {
    if (node == nullptr)
        return std::string();
    std::string result = boost::str(boost::format("%1%%2%%3%\n") % prefix % 
        (last ? "\xE2\x94\x94\xE2\x94\x80" : "\xE2\x94\x9C\xE2\x94\x80") % node->Token->GetString());
    std::string lPrefix, rPrefix;
    std::string spaces = std::string(node->Token->GetString().length() - 1, ' ');
    prefix += (last ? "  " : "\xE2\x94\x82 ");
    prefix == spaces;
    result += walk(node->Left, prefix, false);
    result += walk(node->Right, prefix, true);
    return result;
}

My::SyntaxAnalyzer& My::SyntaxAnalyzer::operator=(SyntaxAnalyzer&& other) {
    std::swap(this->tokenizer, other.tokenizer);
    std::swap(this->root, other.root);
    return *this;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseExpression() {
    Node::PNode e = ParseTerm();
    auto token = tokenizer.Current();
    while (token->GetSubType() == Tokenizer::Token::SubTypes::Plus ||
        token->GetSubType() == Tokenizer::Token::SubTypes::Minus) {
        tokenizer.Next();
        e = Node::PNode(new BinaryOperation(token, e, ParseTerm()));
        token = tokenizer.Current();
    }
    return e;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseTerm() {
    Node::PNode e = ParseFactor();
    auto token = tokenizer.Current();
    while (token->GetSubType() == Tokenizer::Token::SubTypes::Mult ||
        token->GetSubType() == Tokenizer::Token::SubTypes::Divide) {
        tokenizer.Next();
        e = Node::PNode(new BinaryOperation(token, e, ParseFactor()));
        token = tokenizer.Current();
    }
    return e;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseFactor() {
    const auto& token = tokenizer.Current(); tokenizer.Next();
    switch (token->GetSubType()) {
    case Tokenizer::Token::SubTypes::Identifier:
        return Node::PNode(new VariableNode(token));
    case Tokenizer::Token::SubTypes::IntegerConst:
        return Node::PNode(new IntNode(token));
    case Tokenizer::Token::SubTypes::FloatConst:
        return Node::PNode(new FloatNode(token));
    case Tokenizer::Token::SubTypes::StringConst:
        return Node::PNode(new StringNode(token));
    case Tokenizer::Token::SubTypes::CharConst:
        return Node::PNode(new CharNode(token));
    case Tokenizer::Token::SubTypes::OpenParenthesis:
    {
        auto e = ParseExpression();
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseParenthesis);
        tokenizer.Next();
        return e;
    }
    default:
        throw SyntaxErrorException(token, Tokenizer::Token::SubTypes::Identifier);
    }
}

void My::SyntaxAnalyzer::Parse() {
    tokenizer.Next();
    if (tokenizer.Current() != tokenizer.GetEndToken()) {
        root = ParseExpression();
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::EndOfFile);
    }
}

std::string My::SyntaxAnalyzer::ToString() {
    return walk(root, "", true);
}
