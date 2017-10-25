#include "syntax_analyzer.hpp"
#include "boost/format.hpp"
#include <iomanip>
#include <vector>
#include <unordered_set>

const My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::integerNode = 
    My::SyntaxAnalyzer::TypeNode::PTypeNode(new My::SyntaxAnalyzer::ScalarNode("integer", My::SyntaxAnalyzer::ScalarNode::ScalarType::Integer, nullptr));

const My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::realNode =
    My::SyntaxAnalyzer::TypeNode::PTypeNode(new My::SyntaxAnalyzer::ScalarNode("real", My::SyntaxAnalyzer::ScalarNode::ScalarType::Real, nullptr));

const My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::charNode =
    My::SyntaxAnalyzer::TypeNode::PTypeNode(new My::SyntaxAnalyzer::ScalarNode("char", My::SyntaxAnalyzer::ScalarNode::ScalarType::Char, nullptr));

const std::string My::SyntaxAnalyzer::Node::TypeNames[] = {
    "",
    "type ",
    "",
    "",
    "",
    "procedure ",
    "function ",
    ""
};

const std::string My::SyntaxAnalyzer::TypeNode::TypeIdentifierNames[] = { " scalar", " array", " record", " alias" };
const std::string My::SyntaxAnalyzer::VariableNode::VariableTypeNames[] = { "value ", "const ", "var " };

const My::SyntaxAnalyzer::binaryOpMap_t My::SyntaxAnalyzer::binaryOpMap = {

    { "+", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::plus>(left, right);
            }
    },

    { "-", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::minus>(left, right);
            }
    },

    { "*", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::multiplies>(left, right);
            }
    },

    { "/", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::divides>(left, right, true);
            }
    },

    { "mod", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                My::SyntaxAnalyzer::requireInteger(left, right);
                long long result = left->GetValue<long long>() % right->GetValue<long long>();
                return My::SyntaxAnalyzer::ConstantNode::PConstantNode(new My::SyntaxAnalyzer::ConstantNode(std::to_string(result),
                    My::Tokenizer::Token::Value(result), left->ScalarType, nullptr));
            }
    },

    { "div", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                My::SyntaxAnalyzer::requireInteger(left, right);
                return My::SyntaxAnalyzer::calc<std::divides>(left, right);
            }
    },

    { "shl", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                My::SyntaxAnalyzer::requireInteger(left, right);
                long long result = left->GetValue<long long>() << right->GetValue<long long>();
                return My::SyntaxAnalyzer::ConstantNode::PConstantNode(new My::SyntaxAnalyzer::ConstantNode(std::to_string(result),
                    My::Tokenizer::Token::Value(result), left->ScalarType, nullptr));
            }
    },

    { "shr", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                My::SyntaxAnalyzer::requireInteger(left, right);
                long long result = left->GetValue<long long>() >> right->GetValue<long long>();
                return My::SyntaxAnalyzer::ConstantNode::PConstantNode(new My::SyntaxAnalyzer::ConstantNode(std::to_string(result),
                    My::Tokenizer::Token::Value(result), left->ScalarType, nullptr));
            }
    },

    { "and", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                My::SyntaxAnalyzer::requireInteger(left, right);
                long long result = left->GetValue<long long>() & right->GetValue<long long>();
                return My::SyntaxAnalyzer::ConstantNode::PConstantNode(new My::SyntaxAnalyzer::ConstantNode(std::to_string(result),
                    My::Tokenizer::Token::Value(result), left->ScalarType, nullptr));
            }
    },

    { "or", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                My::SyntaxAnalyzer::requireInteger(left, right);
                long long result = left->GetValue<long long>() | right->GetValue<long long>();
                return My::SyntaxAnalyzer::ConstantNode::PConstantNode(new My::SyntaxAnalyzer::ConstantNode(std::to_string(result),
                    My::Tokenizer::Token::Value(result), left->ScalarType, nullptr));
            }
    },

    { "<", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::less>(left, right);
            }
    },

    { "<=", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::less_equal>(left, right);
            }
    },

    { ">", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::greater>(left, right);
            }
    },

    { ">=", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::greater_equal>(left, right);
            }
    },

    { "=", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::equal_to>(left, right);
            }
    },

    { "<>", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::not_equal_to>(left, right);
            }
    }

};

const My::SyntaxAnalyzer::unaryOpMap_t My::SyntaxAnalyzer::unaryOpMap = {

    { "-", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                if (left->ScalarType == My::SyntaxAnalyzer::ScalarNode::ScalarType::Integer)
                    return My::SyntaxAnalyzer::ConstantNode::PConstantNode(new My::SyntaxAnalyzer::ConstantNode(std::to_string(-left->GetValue<long long>()),
                        My::Tokenizer::Token::Value(-left->GetValue<long long>()), left->ScalarType, nullptr));
                return My::SyntaxAnalyzer::ConstantNode::PConstantNode(new My::SyntaxAnalyzer::ConstantNode(std::to_string(-left->GetValue<double>()),
                    My::Tokenizer::Token::Value(-left->GetValue<double>()), left->ScalarType, nullptr));
            }
    },

    { "+", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return left;
            }
    },

    { "not", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::ConstantNode::PConstantNode(new My::SyntaxAnalyzer::ConstantNode(std::to_string(!left->GetValue<bool>()),
                    My::Tokenizer::Token::Value(!left->GetValue<bool>()), left->ScalarType, nullptr));
            }
    }

};

My::SyntaxAnalyzer::ExpectedException::ExpectedException(const My::Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type) {
    message = std::string(boost::str(boost::format("(%4%, %5%) Syntax Error: %1% expected but %2% \"%3%\" found") %
        My::Tokenizer::Token::SubTypesStrings[static_cast<unsigned int>(type)] %
        My::Tokenizer::Token::TypesStrings[static_cast<unsigned int>(token->GetType())] %
        token->GetValueString() % token->GetPosition().first % token->GetPosition().second
    ));
}

const char* My::SyntaxAnalyzer::SyntaxErrorException::what() const {
    return message.c_str();
}

My::SyntaxAnalyzer::ConstantNode::PConstantNode My::SyntaxAnalyzer::calculateConstExpr(Node::PNode n) {
    switch (n->MyType) {
    case Node::Type::Const:
        return std::dynamic_pointer_cast<ConstantNode>(n);
    case Node::Type::Variable:
    {
        auto var = std::dynamic_pointer_cast<VariableNode>(n);
        if (var->vType != VariableNode::VariableType::Const || var->type != nullptr)
            throw IllegalExpressionException(tokenizer.Current());
        return std::dynamic_pointer_cast<ConstantNode>(var->Value);
    }
    case Node::Type::Operation:
    {
        auto op = std::dynamic_pointer_cast<OperationNode>(n);
        auto left = calculateConstExpr(op->Children[0]);
        if (op->Children.size() > 1) {
            for (int i = 1; i < op->Children.size() - 1; ++i) {
                auto right = calculateConstExpr(op->Children[i]);
                requireTypesCompatibility(left, right, op->Token);
                left = binaryOpMap.at(op->ToString())(left, right);
            }
        }
        else
            left = unaryOpMap.at(op->ToString())(left);
        return left;
    }
    default:
        break;
    }
    throw std::exception();
}

void My::SyntaxAnalyzer::requireTypesCompatibility(TypeNode::PTypeNode left, TypeNode::PTypeNode right, My::Tokenizer::PToken token, bool assign, bool allow_left_int) {
    left = getBaseType(left); right = getBaseType(right);
    if (left == right && assign)
        return;
    if (left->MyTypeIdentifier != right->MyTypeIdentifier)
        throw IncompatibleTypesException(left, right, token);
    auto l = std::dynamic_pointer_cast<ScalarNode>(left);
    auto r = std::dynamic_pointer_cast<ScalarNode>(right);
    if (!left || !right)
        throw IncompatibleTypesException(left, right, token);
    if (l->MyScalarType == ScalarNode::ScalarType::Char || r->MyScalarType == ScalarNode::ScalarType::Char ||
        !allow_left_int && l->MyScalarType == ScalarNode::ScalarType::Integer && r->MyScalarType == ScalarNode::ScalarType::Real)
        throw IncompatibleTypesException(left, right, token);
}

void My::SyntaxAnalyzer::requireTypesCompatibility(TypeNode::PTypeNode left, ConstantNode::PConstantNode right, My::Tokenizer::PToken token) {
    if (left->MyTypeIdentifier != TypeNode::TypeIdentifier::Scalar)
        throw IncompatibleTypesException(left, getType(right), token);
    auto l = std::dynamic_pointer_cast<ScalarNode>(left);
    if (l->MyScalarType == ScalarNode::ScalarType::Real && right->ScalarType == ScalarNode::ScalarType::Integer ||
        l->MyScalarType == right->ScalarType)
        return;
    throw IncompatibleTypesException(left, getType(right), token);
}   

void My::SyntaxAnalyzer::requireTypesCompatibility(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right, My::Tokenizer::PToken token) {
    if (left->ScalarType != ScalarNode::ScalarType::Char && left->ScalarType != ScalarNode::ScalarType::Char)
        return;
    throw IncompatibleTypesException(getType(left), getType(right), token);
}

void My::SyntaxAnalyzer::requireInteger(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right) {
    if (left->ScalarType != My::SyntaxAnalyzer::ScalarNode::ScalarType::Integer ||
        right->ScalarType != My::SyntaxAnalyzer::ScalarNode::ScalarType::Integer)
        throw My::SyntaxAnalyzer::SyntaxErrorException("Illegal expression");
}

void My::SyntaxAnalyzer::requireArgsCompatibility(Node::PNode f, Node::PNode n) {
    int i = 0;
    for (; i < f->Children.size() && f->Children[i]->MyType == Node::Type::Variable; ++i) {
        auto var = std::dynamic_pointer_cast<VariableNode>(f->Children[i]);
        if (var->Value)
            return;
        if (i == n->Children.size())
            throw FunctionParameterException(tokenizer.Current(), i);
        requireTypesCompatibility(getType(f->Children[i]), var->type, tokenizer.Current(), true, false);
    }
    if (i != n->Children.size())
        throw FunctionParameterException(tokenizer.Current(), i);
}

void My::SyntaxAnalyzer::requireType(TypeNode::PTypeNode type, TypeNode::TypeIdentifier id, Tokenizer::PToken token) {
    if (type->MyTypeIdentifier != id)
        throw IncompatibleTypesException(type, nullptr, token);
}

My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::getBaseType(TypeNode::PTypeNode type) {
    while (type->MyTypeIdentifier == TypeNode::TypeIdentifier::Alias)
        type = std::dynamic_pointer_cast<TypeNode>(type->Children[0]);
    return type;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::findDeclaration(const My::Tokenizer::PToken token) {
    requireDeclaration(token);
    for (auto it = declarations.rbegin(); it != declarations.rend(); ++it) {
        auto t = it->Symbols.find(token->GetValueString());
        if (t != it->Symbols.end())
            return t->second;
    }
    return nullptr;
}

void My::SyntaxAnalyzer::requireNodeType(Node::PNode n, Node::Type type) {
    if (n->MyType != type)
        throw IllegalTypeException(tokenizer.Current());
}

void My::SyntaxAnalyzer::requireDeclaration(const My::Tokenizer::PToken t) {
    for (auto it = declarations.rbegin(); it != declarations.rend(); ++it)
        if (it->Symbols.count(t->GetValueString()) > 0)
            return;
    throw DeclarationNotFoundException(t);
}

void My::SyntaxAnalyzer::require(const Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type) {
    if (token->GetSubType() != type)
        throw ExpectedException(token, type);
}

std::string My::SyntaxAnalyzer::walk(const Node::PNode node, std::string prefix, bool last) {
    if (node == nullptr)
        return std::string();
    std::string type = ""; 
    if (node->MyType == Node::Type::Variable)
        type = VariableNode::VariableTypeNames[static_cast<int>(std::dynamic_pointer_cast<VariableNode>(node)->vType)];
    else
        type = Node::TypeNames[static_cast<int>(node->MyType)];
    std::string type_i = "";
    if (node->MyType == Node::Type::Type)
        type_i = TypeNode::TypeIdentifierNames[static_cast<int>(std::dynamic_pointer_cast<TypeNode>(node)->MyTypeIdentifier)];
    std::string name = type + node->ToString() + type_i;
    std::string result = boost::str(boost::format("%1%%2%%3%\n") % prefix % 
        (last ? "\xE2\x94\x94\xE2\x94\x80" : "\xE2\x94\x9C\xE2\x94\x80") % name);
    if (!node->Children.size())
        return result;
    std::string spaces = std::string(name.length() - 1, ' ');
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
    return parse<OperationNode, &SyntaxAnalyzer::ParseSimpleExpression, expressionOperators>(ParseSimpleExpression());
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseSimpleExpression() {
    return parse<OperationNode, &SyntaxAnalyzer::ParseTerm, simpleExpressionOperators>(ParseTerm());
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseTerm() {
    return parse<OperationNode, &SyntaxAnalyzer::ParseFactor, termOperators>(ParseFactor());
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseField(Node::PNode n, TypeNode::PTypeNode type) {
    auto t = tokenizer.Current();
    while (t->GetSubType() == Tokenizer::Token::SubTypes::Dot) {
        auto tt = t;
        requireType(type, TypeNode::TypeIdentifier::Record, t);
        t = tokenizer.Next();
        require(t, Tokenizer::Token::SubTypes::Identifier);
        for (auto it : type->Children)
            if (it->ToString() == t->GetValueString()) {
                auto var = std::dynamic_pointer_cast<VariableNode>(it);
                n = Node::PNode(new OperationNode(".", var->type, tt, n, var));
                type = var->type;
                t = tokenizer.Next();
                goto end;
            }
        throw SyntaxAnalyzer("Field not found");
    end:
        continue;
    }
    if (type->MyTypeIdentifier == TypeNode::TypeIdentifier::Array)
        return ParseArrayIndex(n, type);
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseFunctionCall(Node::PNode n) {
    requireNodeType(n, Node::Type::Function);
    auto args = ParseActualParameterList();
    requireArgsCompatibility(n->Children[0], args);
    auto f = std::dynamic_pointer_cast<FunctionNode>(n);
    auto r = Node::PNode(new OperationNode("()", f->ReturnType, tokenizer.Current(), f, args));
    if (f->ReturnType->MyTypeIdentifier == TypeNode::TypeIdentifier::Array)
        return ParseArrayIndex(r, f->ReturnType);
    else if (f->ReturnType->MyTypeIdentifier == TypeNode::TypeIdentifier::Record)
        return ParseField(r, f->ReturnType);
    return r;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseProcedureCall(Node::PNode n) {
    requireNodeType(n, Node::Type::Procedure);
    auto args = ParseActualParameterList();
    requireArgsCompatibility(n->Children[0], args);
    return Node::PNode(new OperationNode("()", nullptr, tokenizer.Current(), n, args));
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseArrayIndex(Node::PNode n, TypeNode::PTypeNode type) {
    auto t = tokenizer.Current();
    while (t->GetSubType() == Tokenizer::Token::SubTypes::OpenBracket) {
        requireType(type, TypeNode::TypeIdentifier::Array, t);
        auto a = std::dynamic_pointer_cast<ArrayNode>(type);
        tokenizer.Next();
        n = Node::PNode(new OperationNode("[]", a->type, t, n, ParseExpression()));
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseBracket);
        type = a->type;
        t = tokenizer.Next();
    }
    if (type->MyTypeIdentifier == TypeNode::TypeIdentifier::Record)
        return ParseField(n, type);
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseFactor() {
    auto token = tokenizer.Current();
    if (simpleExpressionOperators(token->GetSubType()) || token->GetSubType() == Tokenizer::Token::SubTypes::Not) {
        tokenizer.Next();
        auto f = ParseFactor();
        return Node::PNode(new OperationNode(token->GetValueString(), getType(f), token, f));
    }
    switch (token->GetSubType()) {
    case Tokenizer::Token::SubTypes::Identifier:
    {
        auto n = findDeclaration(token);
        switch (n->MyType) {
        case Node::Type::Function:
            tokenizer.Next();
            return ParseFunctionCall(n);
        case Node::Type::Variable:
        {
            tokenizer.Next();
            auto var = std::dynamic_pointer_cast<VariableNode>(n);
            if (var->type->MyTypeIdentifier == TypeNode::TypeIdentifier::Record)
                return ParseField(var, var->type);
            if (var->type->MyTypeIdentifier == TypeNode::TypeIdentifier::Array)
                return ParseArrayIndex(var, var->type);
            return var;
        }
        default:
            requireNodeType(n, Node::Type::Variable);
        }
    }
    case Tokenizer::Token::SubTypes::IntegerConst:
        tokenizer.Next();
        return Node::PNode(new ConstantNode(token->GetString(), token->GetValue(), ScalarNode::ScalarType::Integer, token));
    case Tokenizer::Token::SubTypes::FloatConst:
        tokenizer.Next();
        return Node::PNode(new ConstantNode(token->GetString(), token->GetValue(), ScalarNode::ScalarType::Real, token));
    case Tokenizer::Token::SubTypes::CharConst:
        tokenizer.Next();
        return Node::PNode(new ConstantNode(token->GetString(), token->GetValue(), ScalarNode::ScalarType::Char, token));
    case Tokenizer::Token::SubTypes::OpenParenthesis:
    {
        tokenizer.Next();
        auto e = ParseExpression();
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseParenthesis);
        tokenizer.Next();
        return e;
    }
    default:
        throw ExpectedException(token, Tokenizer::Token::SubTypes::Identifier);
    }
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseProgram() {
    //Init base types
    declarations.push_back(Declaration());
    declarations.back().Symbols["integer"] = integerNode;
    declarations.back().Symbols["real"] = realNode;
    declarations.back().Symbols["char"] = charNode;

    auto t = tokenizer.Current();
    require(t, Tokenizer::Token::SubTypes::Program);
    t = tokenizer.Next();
    require(t, Tokenizer::Token::SubTypes::Identifier);
    require(tokenizer.Next(), Tokenizer::Token::SubTypes::Semicolon);
    tokenizer.Next();
    declarations.push_back(Declaration());
    auto n = Node::PNode(new Node(t->GetValueString(), Node::Type::Block, nullptr, ParseBlock()));
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Dot);
    declarations.pop_back();
    declarations.pop_back();
    tokenizer.Next();
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseBlock() {
    auto d = ParseDeclarationPart();
    auto n = Node::PNode(new Node("block", Node::Type::Block, nullptr, d, ParseCompoundStatement()));
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseDeclarationPart() {
    auto n = Node::PNode(new Node("decl", Node::Type::Block, nullptr));
    while (true) {
        switch (tokenizer.Current()->GetSubType()) {
        case Tokenizer::Token::SubTypes::Type:
            tokenizer.Next();
            ParseTypeDeclaration(n);
            break;
        case Tokenizer::Token::SubTypes::Var:
            tokenizer.Next();
            ParseVarDeclaration(n);
            break;
        case Tokenizer::Token::SubTypes::Const:
            tokenizer.Next();
            ParseConstDeclaration(n);
            break;
        case Tokenizer::Token::SubTypes::Procedure:
            tokenizer.Next();
            n->push_back(ParseProcedure());
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
            tokenizer.Next();
            break;
        case Tokenizer::Token::SubTypes::Function:
            tokenizer.Next();
            n->push_back(ParseFunction());
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
            tokenizer.Next();
            break;
        default:
            goto end;
        }
    }
    end:
    return n;
}

void My::SyntaxAnalyzer::ParseStatementList(Node::PNode p) {
    p->push_back(ParseStatement());
    while (tokenizer.Current()->GetSubType() == Tokenizer::Token::SubTypes::Semicolon) {
        tokenizer.Next();
        p->push_back(ParseStatement());
    }
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseTypedConst(TypeNode::PTypeNode type) {
    type = getBaseType(type);
    if (type->MyTypeIdentifier == TypeNode::TypeIdentifier::Scalar)
        return ParseExpression();
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::OpenParenthesis);
    Node::PNode n = Node::PNode(new TypedConstantNode(tokenizer.Current(), type));
    if (type->MyTypeIdentifier == TypeNode::TypeIdentifier::Array) {
        auto a = std::dynamic_pointer_cast<ArrayNode>(type);
        auto end = std::dynamic_pointer_cast<ConstantNode>(a->Children[1])->GetValue<long long>();
        for (auto i = std::dynamic_pointer_cast<ConstantNode>(a->Children[0])->GetValue<long long>();
            i <= end; ++i) {
            tokenizer.Next();
            auto c = ParseTypedConst(a->type);
            requireTypesCompatibility(a->type, getType(c), tokenizer.Current(), true, false);
            n->push_back(c);
            if (i == end)
                require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseParenthesis);
            else
                require(tokenizer.Current(), Tokenizer::Token::SubTypes::Comma);
        }
        tokenizer.Next();
        return n;
    }
    auto t = tokenizer.Next();
    for (int i = 0; i < type->Children.size(); ++i) {
        auto it = std::dynamic_pointer_cast<VariableNode>(type->Children[i]);
        require(t, Tokenizer::Token::SubTypes::Identifier);
        require(tokenizer.Next(), Tokenizer::Token::SubTypes::Colon);
        if (it->ToString() != t->GetValueString())
            throw IllegalInitializationOrderException(t);
        tokenizer.Next();
        auto c = ParseTypedConst(it->type);
        requireTypesCompatibility(it->type, getType(c), tokenizer.Current(), true, false);
        n->push_back(c);
        if (i + 1 == type->Children.size() && tokenizer.Current()->GetSubType() != Tokenizer::Token::SubTypes::Semicolon)
            break;
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
        t = tokenizer.Next();
    }
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseParenthesis);
    tokenizer.Next();
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseActualParameterList() {
    auto n = Node::PNode(new Node("p_list", Node::Type::Block, nullptr));
    if (tokenizer.Current()->GetSubType() != Tokenizer::Token::SubTypes::OpenParenthesis)
        return n;
    if (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::CloseParenthesis) {
        tokenizer.Next();
        return n;
    }
    ParseExprressionList(n);
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseParenthesis);
    tokenizer.Next();
    return n;
}

void My::SyntaxAnalyzer::ParseExprressionList(Node::PNode p) {
    p->push_back(ParseExpression());
    while (tokenizer.Current()->GetSubType() == Tokenizer::Token::SubTypes::Comma) {
        tokenizer.Next();
        p->push_back(ParseExpression());
    }
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseProcedure() {
    auto t = tokenizer.Current();
    require(t, Tokenizer::Token::SubTypes::Identifier);
    requireUnique(t, declarations.back().Symbols);
    declarations.push_back(Declaration());
    tokenizer.Next();
    auto pl = ParseFormalParameterList();
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
    tokenizer.Next();
    auto n = Node::PNode(new ProcedureNode(t->GetValueString(), pl, ParseBlock(), nullptr));
    declarations.pop_back();
    declarations.back().Symbols[n->ToString()] = n;
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseFunction() {
    auto t = tokenizer.Current();
    require(t, Tokenizer::Token::SubTypes::Identifier);
    requireUnique(t, declarations.back().Symbols);
    declarations.push_back(Declaration());
    declarations.back().Symbols["result"] = nullptr;
    tokenizer.Next();
    auto pl = ParseFormalParameterList();
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
    auto rt = tokenizer.Next();
    require(rt, Tokenizer::Token::SubTypes::Identifier);
    require(tokenizer.Next(), Tokenizer::Token::SubTypes::Semicolon);
    tokenizer.Next();
    auto type = findDeclaration(rt);
    requireNodeType(type, Node::Type::Type);
    declarations.back().Symbols["result"] = Node::PNode(new VariableNode("result", std::dynamic_pointer_cast<TypeNode>(type), VariableNode::VariableType::Value, rt));
    auto n = Node::PNode(new FunctionNode(t->GetValueString(), pl, std::dynamic_pointer_cast<TypeNode>(type), ParseBlock(), nullptr));
    declarations.pop_back();
    declarations.back().Symbols[n->ToString()] = n;
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseFormalParameterList() {
    auto n = Node::PNode(new Node("p_list", Node::Type::Block, nullptr));
    if (tokenizer.Current()->GetSubType() != Tokenizer::Token::SubTypes::OpenParenthesis)
        return n;
    auto t = tokenizer.Next();
    auto set = std::unordered_set<std::string>();
    bool end = false;
    while (t->GetSubType() != Tokenizer::Token::SubTypes::CloseParenthesis) {
        bool last = false;
        std::vector<std::string> ids;
        if (t->GetSubType() == Tokenizer::Token::SubTypes::Var) {
            tokenizer.Next();
            ParseIdentifierList(ids, set);
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
            require(tokenizer.Next(), Tokenizer::Token::SubTypes::Identifier);
            auto type = findDeclaration(tokenizer.Current());
            requireNodeType(type, Node::Type::Type);
            for (auto it : ids) {
                n->push_back(Node::PNode(new VariableNode(it, std::dynamic_pointer_cast<TypeNode>(type), VariableNode::VariableType::Var, nullptr)));
                declarations.back().Symbols[it] = n->Children.back();
            }
            tokenizer.Next();
        }
        else {
            auto v_type = VariableNode::VariableType::Value;
            if (t->GetSubType() == Tokenizer::Token::SubTypes::Const) {
                v_type = VariableNode::VariableType::Const;
                tokenizer.Next();
            }
            else
                require(t, Tokenizer::Token::SubTypes::Identifier);
            ParseIdentifierList(ids, set);
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
            require(tokenizer.Next(), Tokenizer::Token::SubTypes::Identifier);
            auto type = findDeclaration(tokenizer.Current());
            requireNodeType(type, Node::Type::Type);
            for (auto it : ids) {
                n->push_back(Node::PNode(new VariableNode(it, std::dynamic_pointer_cast<TypeNode>(type), v_type, nullptr)));
                declarations.back().Symbols[it] = n->Children.back();
            }
            if (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::Equal) {
                if (ids.size() > 1)
                    throw InitializationOverloadException(tokenizer.Current());
                tokenizer.Next();
                auto value = calculateConstExpr(ParseExpression());
                auto var = std::dynamic_pointer_cast<VariableNode>(n->Children.back());
                requireTypesCompatibility(var->type, value, tokenizer.Current());
                var->SetValue(value);
                end = true;
                last = true;
            }
        }
        if (end && !last)
            throw SyntaxErrorException("Default parameter should be last");
        if (tokenizer.Current()->GetSubType() == Tokenizer::Token::SubTypes::CloseParenthesis)
            break;
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
        t = tokenizer.Next();
    }
    tokenizer.Next();
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseCompoundStatement() {
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Begin);
    auto n = Node::PNode(new Node("statements", Node::Type::Block, nullptr));
    tokenizer.Next();
    ParseStatementList(n);
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::End);
    tokenizer.Next();
    return n;
}

void My::SyntaxAnalyzer::ParseTypeDeclaration(Node::PNode p) {
    Tokenizer::PToken t;
    while ((t = tokenizer.Current())->GetType() == Tokenizer::Token::Types::Identifier) {
        requireUnique(t, declarations.back().Symbols);
        require(tokenizer.Next(), Tokenizer::Token::SubTypes::Equal);
        tokenizer.Next();
        auto type = ParseType(t->GetStringValue());
        p->push_back(type);
        declarations.back().Symbols[t->GetStringValue()] = type;
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
        tokenizer.Next();
    }
}

My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::ParseType(std::string name = "anonymous") {
    auto t = tokenizer.Current();
    switch (t->GetSubType()) {
    case Tokenizer::Token::SubTypes::Identifier:
    {
        tokenizer.Next();
        auto type = findDeclaration(t);
        requireNodeType(type, Node::Type::Type);
        if (name == "anonymous")
            return std::dynamic_pointer_cast<TypeNode>(type);
        return TypeNode::PTypeNode(new TypeAliasNode(name, type, t));
    }
    case Tokenizer::Token::SubTypes::Array:
    {
        require(tokenizer.Next(), Tokenizer::Token::SubTypes::OpenBracket);
        tokenizer.Next();
        auto from = calculateConstExpr(ParseExpression());
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Range);
        tokenizer.Next();
        auto to = calculateConstExpr(ParseExpression());
        requireInteger(from, to);
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseBracket);
        require(tokenizer.Next(), Tokenizer::Token::SubTypes::Of);
        tokenizer.Next();
        return TypeNode::PTypeNode(new ArrayNode(name, from, to, ParseType(), nullptr));
    }
    case Tokenizer::Token::SubTypes::Record:
    {
        t = tokenizer.Next();
        auto n = TypeNode::PTypeNode(new RecordNode(name, t));
        while (t->GetSubType() == Tokenizer::Token::SubTypes::Identifier) {
            require(tokenizer.Next(), Tokenizer::Token::SubTypes::Colon);
            for (auto it : n->Children)
                if (it->ToString() == t->GetValueString())
                    throw DudlicateIdentifierException(t);
            tokenizer.Next();
            n->push_back(Node::PNode(new VariableNode(t->GetValueString(), ParseType(), VariableNode::VariableType::Value, nullptr)));
            t = tokenizer.Current();
            if (t->GetSubType() != Tokenizer::Token::SubTypes::End) {
                require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
                t = tokenizer.Next();
            }
        }
        require(t, Tokenizer::Token::SubTypes::End);
        tokenizer.Next();
        return n;
    }
    default:
        throw ExpectedException(t, Tokenizer::Token::SubTypes::Identifier);
    }
    return nullptr;
}

void My::SyntaxAnalyzer::ParseVarDeclaration(Node::PNode p) {
    auto t = tokenizer.Current();
    while (t->GetSubType() == Tokenizer::Token::SubTypes::Identifier) {
        requireUnique(t, declarations.back().Symbols);
        std::vector<std::string> vars;
        ParseIdentifierList(vars, declarations.back().Symbols, nullptr);
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
        tokenizer.Next();
        auto type = ParseType();
        for (auto it : vars) {
            auto var = Node::PNode(new VariableNode(it, type, VariableNode::VariableType::Value, nullptr));
            declarations.back().Symbols[it] = var;
            p->push_back(var);
        }
        t = tokenizer.Current();
        if (t->GetSubType() == Tokenizer::Token::SubTypes::Equal) {
            if (vars.size() > 1)
                throw InitializationOverloadException(t);
            tokenizer.Next();
            auto c = ParseTypedConst(type);
            requireTypesCompatibility(type, getType(c), t, true, false);
            std::dynamic_pointer_cast<VariableNode>(p->Children.back())->SetValue(c);
        }
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
        t = tokenizer.Next();
    }
}

void My::SyntaxAnalyzer::ParseConstDeclaration(Node::PNode p) {
    auto t = tokenizer.Current();
    while (t->GetSubType() == Tokenizer::Token::SubTypes::Identifier) {
        requireUnique(t, declarations.back().Symbols);
        TypeNode::PTypeNode type = nullptr;
        Node::PNode value;
        if (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::Colon) {
            tokenizer.Next();
            type = ParseType();
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Equal);
            tokenizer.Next();
            value = ParseTypedConst(type);
        }
        else {
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Equal);
            tokenizer.Next();
            value = calculateConstExpr(ParseExpression());
            type = getType(value);
        }
        auto var = Node::PNode(new VariableNode(t->GetValueString(), type, VariableNode::VariableType::Const, nullptr, value));
        declarations.back().Symbols[t->GetValueString()] = var;
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
        p->push_back(var);
        t = tokenizer.Next();
    }
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseStatement() {
    auto t = tokenizer.Current();
    switch (t->GetSubType()) {
    case Tokenizer::Token::SubTypes::Begin:
        return ParseCompoundStatement();
    case Tokenizer::Token::SubTypes::If:
        return ParseIfStatement();
    case Tokenizer::Token::SubTypes::While:
        return ParseWhileStatement();
    case Tokenizer::Token::SubTypes::For:
        return ParseForStatement();
    case Tokenizer::Token::SubTypes::Repeat:
        return ParseRepeatStatement();
    case Tokenizer::Token::SubTypes::Read:
    {
        auto n = Node::PNode(new OperationNode(t->GetValueString(), nullptr, nullptr));
        auto t = tokenizer.Next();
        require(t, Tokenizer::Token::SubTypes::OpenParenthesis);
        t = tokenizer.Next();
        require(t, Tokenizer::Token::SubTypes::Identifier);
        auto d = findDeclaration(t);
        requireNodeType(d, Node::Type::Variable);
        auto var = std::dynamic_pointer_cast<VariableNode>(d);
        if (var->vType == VariableNode::VariableType::Const)
            throw SyntaxErrorException("Can't read to const variable");
        n->push_back(var);
        while ((t = tokenizer.Next())->GetSubType() != Tokenizer::Token::SubTypes::CloseParenthesis) {
            require(t, Tokenizer::Token::SubTypes::Comma);
            t = tokenizer.Next();
            require(t, Tokenizer::Token::SubTypes::Identifier);
            d = findDeclaration(t);
            requireNodeType(d, Node::Type::Variable);
            var = std::dynamic_pointer_cast<VariableNode>(d);
            if (var->vType == VariableNode::VariableType::Const)
                throw SyntaxErrorException("Can't read to const variable");
            n->push_back(var);
        }
        tokenizer.Next();
        return n;
    }
    case Tokenizer::Token::SubTypes::Write:
    {
        auto n = Node::PNode(new OperationNode(t->GetValueString(), nullptr, nullptr));
        auto t = tokenizer.Next();
        require(t, Tokenizer::Token::SubTypes::OpenParenthesis);
        t = tokenizer.Next();
        if (t->GetSubType() == Tokenizer::Token::SubTypes::StringConst) {
            n->push_back(Node::PNode(new StringNode(t)));
            t = tokenizer.Next();
        }
        else {
            n->push_back(ParseExpression());
            t = tokenizer.Current();
        }
        while (t->GetSubType() != Tokenizer::Token::SubTypes::CloseParenthesis) {
            require(t, Tokenizer::Token::SubTypes::Comma);
            t = tokenizer.Next();
            if (t->GetSubType() == Tokenizer::Token::SubTypes::StringConst) {
                n->push_back(Node::PNode(new StringNode(t)));
                t = tokenizer.Next();
            }
            else {
                n->push_back(ParseExpression());
                t = tokenizer.Current();
            }
        }
        tokenizer.Next();
        return n;
    }
    case Tokenizer::Token::SubTypes::Identifier:
    {
        auto d = findDeclaration(t);
        tokenizer.Next();
        if (d->MyType == Node::Type::Variable) {
            auto var = std::dynamic_pointer_cast<VariableNode>(d);
            if (var->type->MyTypeIdentifier == TypeNode::TypeIdentifier::Array)
                d = ParseArrayIndex(var, var->type);
            else if (var->type->MyTypeIdentifier == TypeNode::TypeIdentifier::Record)
                d = ParseField(var, var->type);
            t = tokenizer.Current();
            if (t->GetSubType() == Tokenizer::Token::SubTypes::Assign ||
                t->GetSubType() == Tokenizer::Token::SubTypes::PlusAssign ||
                t->GetSubType() == Tokenizer::Token::SubTypes::MinusAssign ||
                t->GetSubType() == Tokenizer::Token::SubTypes::MultAssign ||
                t->GetSubType() == Tokenizer::Token::SubTypes::DivideAssign) {
                tokenizer.Next();
                auto e = ParseExpression();
                if (var->vType == VariableNode::VariableType::Const)
                    throw SyntaxErrorException("Can't assign to const variable");
                requireTypesCompatibility(getType(d), getType(e), t, true, false);;
                return Node::PNode(new OperationNode(t->GetValueString(), nullptr, t, d, e));
            }
            return d;
        }
        if (d->MyType == Node::Type::Function)
            return ParseFunctionCall(d);
        if (d->MyType == Node::Type::Procedure)
            return ParseProcedureCall(d);
        requireNodeType(d, Node::Type::Variable);
    }
    default:
        break;
    }
    return nullptr;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseIfStatement() {
    auto n = Node::PNode(new Node("if", Node::Type::Block, nullptr));
    tokenizer.Next();
    n->push_back(ParseExpression());
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Then);
    tokenizer.Next();
    n->push_back(ParseStatement());
    if (tokenizer.Current()->GetSubType() == Tokenizer::Token::SubTypes::Else) {
        tokenizer.Next();
        n->push_back(ParseStatement());
    }
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseWhileStatement() {
    auto n = Node::PNode(new Node("while", Node::Type::Block, nullptr));
    tokenizer.Next();
    n->push_back(ParseExpression());
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Do);
    tokenizer.Next();
    n->push_back(ParseStatement());
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseForStatement() {
    auto n = Node::PNode(new Node("for", Node::Type::Block, nullptr));
    auto t = tokenizer.Next();
    require(t, Tokenizer::Token::SubTypes::Identifier);
    auto d = findDeclaration(t);
    requireNodeType(d, Node::Type::Variable);
    n->push_back(d);
    require(tokenizer.Next(), Tokenizer::Token::SubTypes::Assign);
    tokenizer.Next();
    auto e = ParseExpression();
    requireTypesCompatibility(getType(d), getType(e), tokenizer.Current(), true, false);
    n->push_back(e);
    t = tokenizer.Current();
    if (t->GetSubType() != Tokenizer::Token::SubTypes::Downto)
        require(t, Tokenizer::Token::SubTypes::To);
    n->push_back(Node::PNode(new Node(t->GetValueString(), Node::Type::Block, nullptr)));
    tokenizer.Next();
    e = ParseExpression();
    requireTypesCompatibility(getType(d), getType(e), tokenizer.Current(), true, false);
    n->push_back(e);
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Do);
    tokenizer.Next();
    n->push_back(ParseStatement());
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseRepeatStatement() {
    auto n = Node::PNode(new Node("repeat", Node::Type::Block, nullptr));
    tokenizer.Next();
    ParseStatementList(n);
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Until);
    tokenizer.Next();
    n->push_back(ParseExpression());
    return n;
}

void My::SyntaxAnalyzer::Parse() {
    tokenizer.Next();
    if (tokenizer.Current() != tokenizer.GetEndToken()) {
        root = ParseProgram();
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::EndOfFile);
    }
}

std::string My::SyntaxAnalyzer::ToString() {
    return walk(root, "", true);
}

My::SyntaxAnalyzer::ScalarNode::ScalarType My::SyntaxAnalyzer::getScalarType(Node::PNode n) {
    if (n->MyType == Node::Type::Const)
        return std::dynamic_pointer_cast<ConstantNode>(n)->ScalarType;
    return std::dynamic_pointer_cast<ScalarNode>(n)->MyScalarType;
}

My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::getConstantType(ConstantNode::PConstantNode n) {
    switch (n->ScalarType) {
    case ScalarNode::ScalarType::Integer:
        return integerNode;
    case ScalarNode::ScalarType::Real:
        return realNode;
    case ScalarNode::ScalarType::Char:
        return charNode;
    default:
        throw std::exception();
    }
}

My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::deduceType(Node::PNode left, Node::PNode right, Tokenizer::PToken token, bool is_relational) {
    auto lt = getBaseType(getType(left)), rt = getBaseType(getType(right));
    requireTypesCompatibility(lt, rt, token, is_relational, true);
    if (is_relational)
        return integerNode;
    if (lt->MyTypeIdentifier == TypeNode::TypeIdentifier::Scalar &&
        rt->MyTypeIdentifier == TypeNode::TypeIdentifier::Scalar) {
        auto ls = std::dynamic_pointer_cast<ScalarNode>(lt), rs = std::dynamic_pointer_cast<ScalarNode>(rt);
        if (ls->MyScalarType == ScalarNode::ScalarType::Real ||
            rs->MyScalarType == ScalarNode::ScalarType::Real)
            return realNode;
        return ls;
    }
    if (lt == rt)
        return lt;
    throw SyntaxErrorException("Incompatible types");
}

My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::getType(Node::PNode n) {
    if (n->MyType == Node::Type::Const)
        return getConstantType(std::dynamic_pointer_cast<ConstantNode>(n));
    if (n->MyType == Node::Type::Operation)
        return std::dynamic_pointer_cast<OperationNode>(n)->ReturnType;
    if (n->MyType == Node::Type::Variable)
        return std::dynamic_pointer_cast<VariableNode>(n)->type;
    if (n->MyType == Node::Type::TypedConst)
        return std::dynamic_pointer_cast<TypedConstantNode>(n)->type;
    return std::dynamic_pointer_cast<TypeNode>(n);
}

const std::string & My::SyntaxAnalyzer::Node::ToString() {
    return name;
}

My::SyntaxAnalyzer::DudlicateIdentifierException::DudlicateIdentifierException(const My::Tokenizer::PToken token) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: Dublicate identifier \"%3%\"") % token->GetPosition().first %
        token->GetPosition().second % token->GetValueString()));
}

My::SyntaxAnalyzer::IncompatibleTypesException::IncompatibleTypesException(const TypeNode::PTypeNode left, const TypeNode::PTypeNode right, const My::Tokenizer::PToken token) {
    if (!right)
        message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: Incompatible type \"%3%\"") % token->GetPosition().first %
            token->GetPosition().second % left->ToString()));
    else
        message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: Incompatible types \"%3%\", \"%4%\"") % token->GetPosition().first %
            token->GetPosition().second % left->ToString() % right->ToString()));
}

My::SyntaxAnalyzer::IllegalInitializationOrderException::IllegalInitializationOrderException(Tokenizer::PToken token) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: Illegal initialization order") % token->GetPosition().first %
        token->GetPosition().second));
}

My::SyntaxAnalyzer::InitializationOverloadException::InitializationOverloadException(Tokenizer::PToken token) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: Only 1 variable can be initialized") % token->GetPosition().first %
        token->GetPosition().second));
}

My::SyntaxAnalyzer::DeclarationNotFoundException::DeclarationNotFoundException(Tokenizer::PToken token) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: \"%3%\" declaration not found") % token->GetPosition().first %
        token->GetPosition().second % token->GetValueString()));
}

My::SyntaxAnalyzer::FunctionParameterException::FunctionParameterException(const Tokenizer::PToken token, int c) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: function doesn't take %3% argument(s)") % token->GetPosition().first %
        token->GetPosition().second % c));
}

My::SyntaxAnalyzer::IllegalExpressionException::IllegalExpressionException(const Tokenizer::PToken token) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: illegal expresion") % token->GetPosition().first %
        token->GetPosition().second));
}

My::SyntaxAnalyzer::IllegalTypeException::IllegalTypeException(const Tokenizer::PToken token) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: illegal type") % token->GetPosition().first %
        token->GetPosition().second));
}
