#include "syntax_analyzer.hpp"
#include "boost/format.hpp"
#include <iomanip>
#include <vector>

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

std::string My::SyntaxAnalyzer::walk(const Node::PNode node, std::string prefix, bool last) {
    if (node == nullptr)
        return std::string();
    std::string result = boost::str(boost::format("%1%%2%%3%\n") % prefix % 
        (last ? "\xE2\x94\x94\xE2\x94\x80" : "\xE2\x94\x9C\xE2\x94\x80") % node->Token->GetString());
    if (!node->Children.size())
        return result;
    std::string spaces = std::string(node->Token->GetString().length() - 1, ' ');
    prefix += last ? "  " : "\xE2\x94\x82 ";
    prefix += spaces;
    for (int i = 0; i < node->Children.size() - 1; ++i)
        result += walk(node->Children[i], prefix, false);
    result += walk(node->Children.back(), prefix, true);
    return result;
}

My::SyntaxAnalyzer& My::SyntaxAnalyzer::operator=(SyntaxAnalyzer&& other) {
    std::swap(this->tokenizer, other.tokenizer);
    std::swap(this->root, other.root);
    return *this;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseExpression() {
    auto token = tokenizer.Current();
    if (expressionOperators(token->GetSubType())) {
        tokenizer.Next();
        return parse<Operation, &SyntaxAnalyzer::ParseTerm, expressionOperators>(
            Node::PNode(new UnaryOperation(token, ParseTerm()))
        );
    }
    return parse<Operation, &SyntaxAnalyzer::ParseTerm, expressionOperators>(ParseTerm());
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseTerm() {
    return parse<Operation, &SyntaxAnalyzer::ParseFactor, termOperators>(ParseFactor());
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
