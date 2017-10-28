#include "syntax_analyzer.hpp"
#include "boost/format.hpp"
#include <iomanip>
#include <vector>
#include <unordered_set>

const pascal_compiler::SyntaxAnalyzer::TypeNode::PTypeNode pascal_compiler::SyntaxAnalyzer::integerNode = 
    pascal_compiler::SyntaxAnalyzer::TypeNode::PTypeNode(new pascal_compiler::SyntaxAnalyzer::ScalarNode("integer", pascal_compiler::SyntaxAnalyzer::ScalarNode::ScalarType::Integer, nullptr));

const pascal_compiler::SyntaxAnalyzer::TypeNode::PTypeNode pascal_compiler::SyntaxAnalyzer::realNode =
    pascal_compiler::SyntaxAnalyzer::TypeNode::PTypeNode(new pascal_compiler::SyntaxAnalyzer::ScalarNode("real", pascal_compiler::SyntaxAnalyzer::ScalarNode::ScalarType::Real, nullptr));

const pascal_compiler::SyntaxAnalyzer::TypeNode::PTypeNode pascal_compiler::SyntaxAnalyzer::charNode =
    pascal_compiler::SyntaxAnalyzer::TypeNode::PTypeNode(new pascal_compiler::SyntaxAnalyzer::ScalarNode("char", pascal_compiler::SyntaxAnalyzer::ScalarNode::ScalarType::Char, nullptr));

const std::string pascal_compiler::SyntaxAnalyzer::Node::TypeNames[] = {
    "",
    "type ",
    "",
    "",
    "",
    "procedure ",
    "function ",
    ""
};

const std::string pascal_compiler::SyntaxAnalyzer::TypeNode::TypeIdentifierNames[] = { " scalar", " array", " record", " alias" };
const std::string pascal_compiler::SyntaxAnalyzer::VariableNode::VariableTypeNames[] = { "value ", "const ", "var " };

const pascal_compiler::SyntaxAnalyzer::binaryOpMap_t pascal_compiler::SyntaxAnalyzer::binaryOpMap = {

    { "+", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return pascal_compiler::SyntaxAnalyzer::calc<std::plus>(left, right);
            }
    },

    { "-", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return pascal_compiler::SyntaxAnalyzer::calc<std::minus>(left, right);
            }
    },

    { "*", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return pascal_compiler::SyntaxAnalyzer::calc<std::multiplies>(left, right);
            }
    },

    { "/", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return pascal_compiler::SyntaxAnalyzer::calc<std::divides>(left, right, true);
            }
    },

    { "mod", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                pascal_compiler::SyntaxAnalyzer::requireInteger(left, right);
                long long result = left->GetValue<long long>() % right->GetValue<long long>();
                return pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode(new pascal_compiler::SyntaxAnalyzer::ConstantNode(std::to_string(result),
                    pascal_compiler::tokenizer::token::value(result), left->ScalarType, nullptr));
            }
    },

    { "div", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                pascal_compiler::SyntaxAnalyzer::requireInteger(left, right);
                return pascal_compiler::SyntaxAnalyzer::calc<std::divides>(left, right);
            }
    },

    { "shl", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                pascal_compiler::SyntaxAnalyzer::requireInteger(left, right);
                long long result = left->GetValue<long long>() << right->GetValue<long long>();
                return pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode(new pascal_compiler::SyntaxAnalyzer::ConstantNode(std::to_string(result),
                    pascal_compiler::tokenizer::token::value(result), left->ScalarType, nullptr));
            }
    },

    { "shr", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                pascal_compiler::SyntaxAnalyzer::requireInteger(left, right);
                long long result = left->GetValue<long long>() >> right->GetValue<long long>();
                return pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode(new pascal_compiler::SyntaxAnalyzer::ConstantNode(std::to_string(result),
                    pascal_compiler::tokenizer::token::value(result), left->ScalarType, nullptr));
            }
    },

    { "and", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                pascal_compiler::SyntaxAnalyzer::requireInteger(left, right);
                long long result = left->GetValue<long long>() & right->GetValue<long long>();
                return pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode(new pascal_compiler::SyntaxAnalyzer::ConstantNode(std::to_string(result),
                    pascal_compiler::tokenizer::token::value(result), left->ScalarType, nullptr));
            }
    },

    { "or", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                pascal_compiler::SyntaxAnalyzer::requireInteger(left, right);
                long long result = left->GetValue<long long>() | right->GetValue<long long>();
                return pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode(new pascal_compiler::SyntaxAnalyzer::ConstantNode(std::to_string(result),
                    pascal_compiler::tokenizer::token::value(result), left->ScalarType, nullptr));
            }
    },

    { "<", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return pascal_compiler::SyntaxAnalyzer::calc<std::less>(left, right);
            }
    },

    { "<=", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return pascal_compiler::SyntaxAnalyzer::calc<std::less_equal>(left, right);
            }
    },

    { ">", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return pascal_compiler::SyntaxAnalyzer::calc<std::greater>(left, right);
            }
    },

    { ">=", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return pascal_compiler::SyntaxAnalyzer::calc<std::greater_equal>(left, right);
            }
    },

    { "=", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return pascal_compiler::SyntaxAnalyzer::calc<std::equal_to>(left, right);
            }
    },

    { "<>", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left, pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return pascal_compiler::SyntaxAnalyzer::calc<std::not_equal_to>(left, right);
            }
    }

};

const pascal_compiler::SyntaxAnalyzer::unaryOpMap_t pascal_compiler::SyntaxAnalyzer::unaryOpMap = {

    { "-", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                if (left->ScalarType == pascal_compiler::SyntaxAnalyzer::ScalarNode::ScalarType::Integer)
                    return pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode(new pascal_compiler::SyntaxAnalyzer::ConstantNode(std::to_string(-left->GetValue<long long>()),
                        pascal_compiler::tokenizer::token::value(-left->GetValue<long long>()), left->ScalarType, nullptr));
                return pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode(new pascal_compiler::SyntaxAnalyzer::ConstantNode(std::to_string(-left->GetValue<double>()),
                    pascal_compiler::tokenizer::token::value(-left->GetValue<double>()), left->ScalarType, nullptr));
            }
    },

    { "+", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return left;
            }
    },

    { "not", [](pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode left) -> pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode(new pascal_compiler::SyntaxAnalyzer::ConstantNode(std::to_string(!left->GetValue<bool>()),
                    pascal_compiler::tokenizer::token::value(!left->GetValue<bool>()), left->ScalarType, nullptr));
            }
    }

};

pascal_compiler::SyntaxAnalyzer::ExpectedException::ExpectedException(const pascal_compiler::tokenizer::token_p token, pascal_compiler::tokenizer::token::sub_types type) {
    message = std::string(boost::str(boost::format("(%4%, %5%) Syntax Error: %1% expected but %2% \"%3%\" found") %
        pascal_compiler::tokenizer::token::sub_types_strings[static_cast<unsigned int>(type)] %
        pascal_compiler::tokenizer::token::types_strings[static_cast<unsigned int>(token->get_type())] %
        token->get_value_string() % token->get_position().first % token->get_position().second
    ));
}

const char* pascal_compiler::SyntaxAnalyzer::SyntaxErrorException::what() const {
    return message.c_str();
}

pascal_compiler::SyntaxAnalyzer::ConstantNode::PConstantNode pascal_compiler::SyntaxAnalyzer::calculateConstExpr(Node::PNode n) {
    switch (n->MyType) {
    case Node::Type::Const:
        return std::dynamic_pointer_cast<ConstantNode>(n);
    case Node::Type::Variable:
    {
        auto var = std::dynamic_pointer_cast<VariableNode>(n);
        if (var->vType != VariableNode::VariableType::Const || var->type != nullptr)
            throw IllegalExpressionException(tokenizer_.current());
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

void pascal_compiler::SyntaxAnalyzer::requireTypesCompatibility(TypeNode::PTypeNode left, TypeNode::PTypeNode right, pascal_compiler::tokenizer::token_p token, bool assign, bool allow_left_int) {
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

void pascal_compiler::SyntaxAnalyzer::requireTypesCompatibility(TypeNode::PTypeNode left, ConstantNode::PConstantNode right, pascal_compiler::tokenizer::token_p token) {
    if (left->MyTypeIdentifier != TypeNode::TypeIdentifier::Scalar)
        throw IncompatibleTypesException(left, getType(right), token);
    auto l = std::dynamic_pointer_cast<ScalarNode>(left);
    if (l->MyScalarType == ScalarNode::ScalarType::Real && right->ScalarType == ScalarNode::ScalarType::Integer ||
        l->MyScalarType == right->ScalarType)
        return;
    throw IncompatibleTypesException(left, getType(right), token);
}   

void pascal_compiler::SyntaxAnalyzer::requireTypesCompatibility(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right, pascal_compiler::tokenizer::token_p token) {
    if (left->ScalarType != ScalarNode::ScalarType::Char && left->ScalarType != ScalarNode::ScalarType::Char)
        return;
    throw IncompatibleTypesException(getType(left), getType(right), token);
}

void pascal_compiler::SyntaxAnalyzer::requireInteger(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right) {
    if (left->ScalarType != pascal_compiler::SyntaxAnalyzer::ScalarNode::ScalarType::Integer ||
        right->ScalarType != pascal_compiler::SyntaxAnalyzer::ScalarNode::ScalarType::Integer)
        throw pascal_compiler::SyntaxAnalyzer::SyntaxErrorException("Illegal expression");
}

void pascal_compiler::SyntaxAnalyzer::requireArgsCompatibility(Node::PNode f, Node::PNode n) {
    int i = 0;
    for (; i < f->Children.size() && f->Children[i]->MyType == Node::Type::Variable; ++i) {
        auto var = std::dynamic_pointer_cast<VariableNode>(f->Children[i]);
        if (var->Value)
            return;
        if (i == n->Children.size())
            throw FunctionParameterException(tokenizer_.current(), i);
        requireTypesCompatibility(getType(f->Children[i]), var->type, tokenizer_.current(), true, false);
    }
    if (i != n->Children.size())
        throw FunctionParameterException(tokenizer_.current(), i);
}

void pascal_compiler::SyntaxAnalyzer::requireType(TypeNode::PTypeNode type, TypeNode::TypeIdentifier id, tokenizer::token_p token) {
    if (type->MyTypeIdentifier != id)
        throw IncompatibleTypesException(type, nullptr, token);
}

pascal_compiler::SyntaxAnalyzer::TypeNode::PTypeNode pascal_compiler::SyntaxAnalyzer::getBaseType(TypeNode::PTypeNode type) {
    while (type->MyTypeIdentifier == TypeNode::TypeIdentifier::Alias)
        type = std::dynamic_pointer_cast<TypeNode>(type->Children[0]);
    return type;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::findDeclaration(const pascal_compiler::tokenizer::token_p token) {
    requireDeclaration(token);
    for (auto it = declarations.rbegin(); it != declarations.rend(); ++it) {
        auto t = it->Symbols.find(token->get_value_string());
        if (t != it->Symbols.end())
            return t->second;
    }
    return nullptr;
}

void pascal_compiler::SyntaxAnalyzer::requireNodeType(Node::PNode n, Node::Type type) {
    if (n->MyType != type)
        throw IllegalTypeException(tokenizer_.current());
}

void pascal_compiler::SyntaxAnalyzer::requireDeclaration(const pascal_compiler::tokenizer::token_p t) {
    for (auto it = declarations.rbegin(); it != declarations.rend(); ++it)
        if (it->Symbols.count(t->get_value_string()) > 0)
            return;
    throw DeclarationNotFoundException(t);
}

void pascal_compiler::SyntaxAnalyzer::require(const tokenizer::token_p token, pascal_compiler::tokenizer::token::sub_types type) {
    if (token->get_sub_type() != type)
        throw ExpectedException(token, type);
}

std::string pascal_compiler::SyntaxAnalyzer::walk(const Node::PNode node, std::string prefix, bool last) {
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

pascal_compiler::SyntaxAnalyzer& pascal_compiler::SyntaxAnalyzer::operator=(SyntaxAnalyzer&& other) {
    std::swap(this->tokenizer_, other.tokenizer_);
    std::swap(this->root, other.root);
    return *this;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseExpression() {
    return parse<OperationNode, &SyntaxAnalyzer::ParseSimpleExpression, expressionOperators>(ParseSimpleExpression());
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseSimpleExpression() {
    return parse<OperationNode, &SyntaxAnalyzer::ParseTerm, simpleExpressionOperators>(ParseTerm());
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseTerm() {
    return parse<OperationNode, &SyntaxAnalyzer::ParseFactor, termOperators>(ParseFactor());
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseField(Node::PNode n, TypeNode::PTypeNode type) {
    auto t = tokenizer_.current();
    while (t->get_sub_type() == tokenizer::token::sub_types::dot) {
        auto tt = t;
        requireType(type, TypeNode::TypeIdentifier::Record, t);
        t = tokenizer_.next();
        require(t, tokenizer::token::sub_types::identifier);
        for (auto it : type->Children)
            if (it->ToString() == t->get_value_string()) {
                auto var = std::dynamic_pointer_cast<VariableNode>(it);
                n = Node::PNode(new OperationNode(".", var->type, tt, n, var));
                type = var->type;
                t = tokenizer_.next();
                goto end;
            }
        throw SyntaxAnalyzer("Field not found");
    end:;
    }
    if (type->MyTypeIdentifier == TypeNode::TypeIdentifier::Array)
        return ParseArrayIndex(n, type);
    return n;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseFunctionCall(Node::PNode n) {
    requireNodeType(n, Node::Type::Function);
    auto args = ParseActualParameterList();
    requireArgsCompatibility(n->Children[0], args);
    auto f = std::dynamic_pointer_cast<FunctionNode>(n);
    auto r = Node::PNode(new OperationNode("()", f->ReturnType, tokenizer_.current(), f, args));
    if (f->ReturnType->MyTypeIdentifier == TypeNode::TypeIdentifier::Array)
        return ParseArrayIndex(r, f->ReturnType);
    else if (f->ReturnType->MyTypeIdentifier == TypeNode::TypeIdentifier::Record)
        return ParseField(r, f->ReturnType);
    return r;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseProcedureCall(Node::PNode n) {
    requireNodeType(n, Node::Type::Procedure);
    auto args = ParseActualParameterList();
    requireArgsCompatibility(n->Children[0], args);
    return Node::PNode(new OperationNode("()", nullptr, tokenizer_.current(), n, args));
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseArrayIndex(Node::PNode n, TypeNode::PTypeNode type) {
    auto t = tokenizer_.current();
    while (t->get_sub_type() == tokenizer::token::sub_types::open_bracket) {
        requireType(type, TypeNode::TypeIdentifier::Array, t);
        auto a = std::dynamic_pointer_cast<ArrayNode>(type);
        tokenizer_.next();
        n = Node::PNode(new OperationNode("[]", a->type, t, n, ParseExpression()));
        require(tokenizer_.current(), tokenizer::token::sub_types::close_bracket);
        type = a->type;
        t = tokenizer_.next();
    }
    if (type->MyTypeIdentifier == TypeNode::TypeIdentifier::Record)
        return ParseField(n, type);
    return n;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseFactor() {
    auto token = tokenizer_.current();
    if (simpleExpressionOperators(token->get_sub_type()) || token->get_sub_type() == tokenizer::token::sub_types::not) {
        tokenizer_.next();
        auto f = ParseFactor();
        return Node::PNode(new OperationNode(token->get_value_string(), getType(f), token, f));
    }
    switch (token->get_sub_type()) {
    case tokenizer::token::sub_types::identifier:
    {
        auto n = findDeclaration(token);
        switch (n->MyType) {
        case Node::Type::Function:
            tokenizer_.next();
            return ParseFunctionCall(n);
        case Node::Type::Variable:
        {
            tokenizer_.next();
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
    case tokenizer::token::sub_types::integer_const:
        tokenizer_.next();
        return Node::PNode(new ConstantNode(token->get_string(), token->get_value(), ScalarNode::ScalarType::Integer, token));
    case tokenizer::token::sub_types::real_const:
        tokenizer_.next();
        return Node::PNode(new ConstantNode(token->get_string(), token->get_value(), ScalarNode::ScalarType::Real, token));
    case tokenizer::token::sub_types::char_const:
        tokenizer_.next();
        return Node::PNode(new ConstantNode(token->get_string(), token->get_value(), ScalarNode::ScalarType::Char, token));
    case tokenizer::token::sub_types::open_parenthesis:
    {
        tokenizer_.next();
        auto e = ParseExpression();
        require(tokenizer_.current(), tokenizer::token::sub_types::close_parenthesis);
        tokenizer_.next();
        return e;
    }
    default:
        throw ExpectedException(token, tokenizer::token::sub_types::identifier);
    }
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseProgram() {
    //Init base types
    declarations.push_back(Declaration());
    declarations.back().Symbols["integer"] = integerNode;
    declarations.back().Symbols["real"] = realNode;
    declarations.back().Symbols["char"] = charNode;

    auto t = tokenizer_.current();
    require(t, tokenizer::token::sub_types::program);
    t = tokenizer_.next();
    require(t, tokenizer::token::sub_types::identifier);
    require(tokenizer_.next(), tokenizer::token::sub_types::semicolon);
    tokenizer_.next();
    declarations.push_back(Declaration());
    auto n = Node::PNode(new Node(t->get_value_string(), Node::Type::Empty, nullptr, ParseBlock()));
    require(tokenizer_.current(), tokenizer::token::sub_types::dot);
    declarations.pop_back();
    declarations.pop_back();
    tokenizer_.next();
    return n;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseBlock() {
    auto d = ParseDeclarationPart();
    auto n = Node::PNode(new Node("block", Node::Type::Empty, nullptr, d, ParseCompoundStatement()));
    return n;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseDeclarationPart() {
    auto n = Node::PNode(new Node("decl", Node::Type::Empty, nullptr));
    while (true) {
        switch (tokenizer_.current()->get_sub_type()) {
        case tokenizer::token::sub_types::type:
            tokenizer_.next();
            ParseTypeDeclaration(n);
            break;
        case tokenizer::token::sub_types::var:
            tokenizer_.next();
            ParseVarDeclaration(n);
            break;
        case tokenizer::token::sub_types::const_op:
            tokenizer_.next();
            ParseConstDeclaration(n);
            break;
        case tokenizer::token::sub_types::procedure:
            tokenizer_.next();
            n->push_back(ParseProcedure());
            require(tokenizer_.current(), tokenizer::token::sub_types::semicolon);
            tokenizer_.next();
            break;
        case tokenizer::token::sub_types::function:
            tokenizer_.next();
            n->push_back(ParseFunction());
            require(tokenizer_.current(), tokenizer::token::sub_types::semicolon);
            tokenizer_.next();
            break;
        default:
            goto end;
        }
    }
    end:
    return n;
}

void pascal_compiler::SyntaxAnalyzer::ParseStatementList(Node::PNode p) {
    p->push_back(ParseStatement());
    while (tokenizer_.current()->get_sub_type() == tokenizer::token::sub_types::semicolon) {
        tokenizer_.next();
        p->push_back(ParseStatement());
    }
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseTypedConst(TypeNode::PTypeNode type) {
    type = getBaseType(type);
    if (type->MyTypeIdentifier == TypeNode::TypeIdentifier::Scalar)
        return ParseExpression();
    require(tokenizer_.current(), tokenizer::token::sub_types::open_parenthesis);
    Node::PNode n = Node::PNode(new TypedConstantNode(tokenizer_.current(), type));
    if (type->MyTypeIdentifier == TypeNode::TypeIdentifier::Array) {
        auto a = std::dynamic_pointer_cast<ArrayNode>(type);
        auto end = std::dynamic_pointer_cast<ConstantNode>(a->Children[1])->GetValue<long long>();
        for (auto i = std::dynamic_pointer_cast<ConstantNode>(a->Children[0])->GetValue<long long>();
            i <= end; ++i) {
            tokenizer_.next();
            auto c = ParseTypedConst(a->type);
            requireTypesCompatibility(a->type, getType(c), tokenizer_.current(), true, false);
            n->push_back(c);
            if (i == end)
                require(tokenizer_.current(), tokenizer::token::sub_types::close_parenthesis);
            else
                require(tokenizer_.current(), tokenizer::token::sub_types::comma);
        }
        tokenizer_.next();
        return n;
    }
    auto t = tokenizer_.next();
    for (int i = 0; i < type->Children.size(); ++i) {
        auto it = std::dynamic_pointer_cast<VariableNode>(type->Children[i]);
        require(t, tokenizer::token::sub_types::identifier);
        require(tokenizer_.next(), tokenizer::token::sub_types::colon);
        if (it->ToString() != t->get_value_string())
            throw IllegalInitializationOrderException(t);
        tokenizer_.next();
        auto c = ParseTypedConst(it->type);
        requireTypesCompatibility(it->type, getType(c), tokenizer_.current(), true, false);
        n->push_back(c);
        if (i + 1 == type->Children.size() && tokenizer_.current()->get_sub_type() != tokenizer::token::sub_types::semicolon)
            break;
        require(tokenizer_.current(), tokenizer::token::sub_types::semicolon);
        t = tokenizer_.next();
    }
    require(tokenizer_.current(), tokenizer::token::sub_types::close_parenthesis);
    tokenizer_.next();
    return n;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseActualParameterList() {
    auto n = Node::PNode(new Node("p_list", Node::Type::Empty, nullptr));
    if (tokenizer_.current()->get_sub_type() != tokenizer::token::sub_types::open_parenthesis)
        return n;
    if (tokenizer_.next()->get_sub_type() == tokenizer::token::sub_types::close_parenthesis) {
        tokenizer_.next();
        return n;
    }
    ParseExprressionList(n);
    require(tokenizer_.current(), tokenizer::token::sub_types::close_parenthesis);
    tokenizer_.next();
    return n;
}

void pascal_compiler::SyntaxAnalyzer::ParseExprressionList(Node::PNode p) {
    p->push_back(ParseExpression());
    while (tokenizer_.current()->get_sub_type() == tokenizer::token::sub_types::comma) {
        tokenizer_.next();
        p->push_back(ParseExpression());
    }
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseProcedure() {
    auto t = tokenizer_.current();
    require(t, tokenizer::token::sub_types::identifier);
    requireUnique(t, declarations.back().Symbols);
    declarations.push_back(Declaration());
    tokenizer_.next();
    auto pl = ParseFormalParameterList();
    require(tokenizer_.current(), tokenizer::token::sub_types::semicolon);
    tokenizer_.next();
    auto n = Node::PNode(new ProcedureNode(t->get_value_string(), pl, ParseBlock(), nullptr));
    declarations.pop_back();
    declarations.back().Symbols[n->ToString()] = n;
    return n;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseFunction() {
    auto t = tokenizer_.current();
    require(t, tokenizer::token::sub_types::identifier);
    requireUnique(t, declarations.back().Symbols);
    declarations.push_back(Declaration());
    declarations.back().Symbols["result"] = nullptr;
    tokenizer_.next();
    auto pl = ParseFormalParameterList();
    require(tokenizer_.current(), tokenizer::token::sub_types::colon);
    auto rt = tokenizer_.next();
    require(rt, tokenizer::token::sub_types::identifier);
    require(tokenizer_.next(), tokenizer::token::sub_types::semicolon);
    tokenizer_.next();
    auto type = findDeclaration(rt);
    requireNodeType(type, Node::Type::Type);
    declarations.back().Symbols["result"] = Node::PNode(new VariableNode("result", std::dynamic_pointer_cast<TypeNode>(type), VariableNode::VariableType::Value, rt));
    auto n = Node::PNode(new FunctionNode(t->get_value_string(), pl, std::dynamic_pointer_cast<TypeNode>(type), ParseBlock(), nullptr));
    declarations.pop_back();
    declarations.back().Symbols[n->ToString()] = n;
    return n;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseFormalParameterList() {
    auto n = Node::PNode(new Node("p_list", Node::Type::Empty, nullptr));
    if (tokenizer_.current()->get_sub_type() != tokenizer::token::sub_types::open_parenthesis)
        return n;
    auto t = tokenizer_.next();
    auto set = std::unordered_set<std::string>();
    bool end = false;
    while (t->get_sub_type() != tokenizer::token::sub_types::close_parenthesis) {
        bool last = false;
        std::vector<std::string> ids;
        if (t->get_sub_type() == tokenizer::token::sub_types::var) {
            tokenizer_.next();
            ParseIdentifierList(ids, set);
            require(tokenizer_.current(), tokenizer::token::sub_types::colon);
            require(tokenizer_.next(), tokenizer::token::sub_types::identifier);
            auto type = findDeclaration(tokenizer_.current());
            requireNodeType(type, Node::Type::Type);
            for (auto it : ids) {
                n->push_back(Node::PNode(new VariableNode(it, std::dynamic_pointer_cast<TypeNode>(type), VariableNode::VariableType::Var, nullptr)));
                declarations.back().Symbols[it] = n->Children.back();
            }
            tokenizer_.next();
        }
        else {
            auto v_type = VariableNode::VariableType::Value;
            if (t->get_sub_type() == tokenizer::token::sub_types::const_op) {
                v_type = VariableNode::VariableType::Const;
                tokenizer_.next();
            }
            else
                require(t, tokenizer::token::sub_types::identifier);
            ParseIdentifierList(ids, set);
            require(tokenizer_.current(), tokenizer::token::sub_types::colon);
            require(tokenizer_.next(), tokenizer::token::sub_types::identifier);
            auto type = findDeclaration(tokenizer_.current());
            requireNodeType(type, Node::Type::Type);
            for (auto it : ids) {
                n->push_back(Node::PNode(new VariableNode(it, std::dynamic_pointer_cast<TypeNode>(type), v_type, nullptr)));
                declarations.back().Symbols[it] = n->Children.back();
            }
            if (tokenizer_.next()->get_sub_type() == tokenizer::token::sub_types::equal) {
                if (ids.size() > 1)
                    throw InitializationOverloadException(tokenizer_.current());
                tokenizer_.next();
                auto value = calculateConstExpr(ParseExpression());
                auto var = std::dynamic_pointer_cast<VariableNode>(n->Children.back());
                requireTypesCompatibility(var->type, value, tokenizer_.current());
                var->SetValue(value);
                end = true;
                last = true;
            }
        }
        if (end && !last)
            throw SyntaxErrorException("Default parameter should be last");
        if (tokenizer_.current()->get_sub_type() == tokenizer::token::sub_types::close_parenthesis)
            break;
        require(tokenizer_.current(), tokenizer::token::sub_types::semicolon);
        t = tokenizer_.next();
    }
    tokenizer_.next();
    return n;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseCompoundStatement() {
    require(tokenizer_.current(), tokenizer::token::sub_types::begin);
    auto n = Node::PNode(new Node("statements", Node::Type::Empty, nullptr));
    tokenizer_.next();
    ParseStatementList(n);
    require(tokenizer_.current(), tokenizer::token::sub_types::end);
    tokenizer_.next();
    return n;
}

void pascal_compiler::SyntaxAnalyzer::ParseTypeDeclaration(Node::PNode p) {
    tokenizer::token_p t;
    while ((t = tokenizer_.current())->get_type() == tokenizer::token::types::identifier) {
        requireUnique(t, declarations.back().Symbols);
        require(tokenizer_.next(), tokenizer::token::sub_types::equal);
        tokenizer_.next();
        auto type = ParseType(t->get_string_value());
        p->push_back(type);
        declarations.back().Symbols[t->get_string_value()] = type;
        require(tokenizer_.current(), tokenizer::token::sub_types::semicolon);
        tokenizer_.next();
    }
}

pascal_compiler::SyntaxAnalyzer::TypeNode::PTypeNode pascal_compiler::SyntaxAnalyzer::ParseType(std::string name = "*") {
    auto t = tokenizer_.current();
    switch (t->get_sub_type()) {
    case tokenizer::token::sub_types::identifier:
    {
        tokenizer_.next();
        auto type = findDeclaration(t);
        requireNodeType(type, Node::Type::Type);
        if (name == "*")
            return std::dynamic_pointer_cast<TypeNode>(type);
        return TypeNode::PTypeNode(new TypeAliasNode(name, type, t));
    }
    case tokenizer::token::sub_types::array:
    {
        require(tokenizer_.next(), tokenizer::token::sub_types::open_bracket);
        tokenizer_.next();
        auto from = calculateConstExpr(ParseExpression());
        require(tokenizer_.current(), tokenizer::token::sub_types::range);
        tokenizer_.next();
        auto to = calculateConstExpr(ParseExpression());
        requireInteger(from, to);
        require(tokenizer_.current(), tokenizer::token::sub_types::close_bracket);
        require(tokenizer_.next(), tokenizer::token::sub_types::of);
        tokenizer_.next();
        return TypeNode::PTypeNode(new ArrayNode(name, from, to, ParseType(), nullptr));
    }
    case tokenizer::token::sub_types::record:
    {
        t = tokenizer_.next();
        auto n = TypeNode::PTypeNode(new RecordNode(name, t));
        while (t->get_sub_type() == tokenizer::token::sub_types::identifier) {
            require(tokenizer_.next(), tokenizer::token::sub_types::colon);
            for (auto it : n->Children)
                if (it->ToString() == t->get_value_string())
                    throw DudlicateIdentifierException(t);
            tokenizer_.next();
            n->push_back(Node::PNode(new VariableNode(t->get_value_string(), ParseType(), VariableNode::VariableType::Value, nullptr)));
            t = tokenizer_.current();
            if (t->get_sub_type() != tokenizer::token::sub_types::end) {
                require(tokenizer_.current(), tokenizer::token::sub_types::semicolon);
                t = tokenizer_.next();
            }
        }
        require(t, tokenizer::token::sub_types::end);
        tokenizer_.next();
        return n;
    }
    default:
        throw ExpectedException(t, tokenizer::token::sub_types::identifier);
    }
    return nullptr;
}

void pascal_compiler::SyntaxAnalyzer::ParseVarDeclaration(Node::PNode p) {
    auto t = tokenizer_.current();
    while (t->get_sub_type() == tokenizer::token::sub_types::identifier) {
        requireUnique(t, declarations.back().Symbols);
        std::vector<std::string> vars;
        ParseIdentifierList(vars, declarations.back().Symbols, nullptr);
        require(tokenizer_.current(), tokenizer::token::sub_types::colon);
        tokenizer_.next();
        auto type = ParseType();
        for (auto it : vars) {
            auto var = Node::PNode(new VariableNode(it, type, VariableNode::VariableType::Value, nullptr));
            declarations.back().Symbols[it] = var;
            p->push_back(var);
        }
        t = tokenizer_.current();
        if (t->get_sub_type() == tokenizer::token::sub_types::equal) {
            if (vars.size() > 1)
                throw InitializationOverloadException(t);
            tokenizer_.next();
            auto c = ParseTypedConst(type);
            requireTypesCompatibility(type, getType(c), t, true, false);
            std::dynamic_pointer_cast<VariableNode>(p->Children.back())->SetValue(c);
        }
        require(tokenizer_.current(), tokenizer::token::sub_types::semicolon);
        t = tokenizer_.next();
    }
}

void pascal_compiler::SyntaxAnalyzer::ParseConstDeclaration(Node::PNode p) {
    auto t = tokenizer_.current();
    while (t->get_sub_type() == tokenizer::token::sub_types::identifier) {
        requireUnique(t, declarations.back().Symbols);
        TypeNode::PTypeNode type = nullptr;
        Node::PNode value;
        if (tokenizer_.next()->get_sub_type() == tokenizer::token::sub_types::colon) {
            tokenizer_.next();
            type = ParseType();
            require(tokenizer_.current(), tokenizer::token::sub_types::equal);
            tokenizer_.next();
            value = ParseTypedConst(type);
        }
        else {
            require(tokenizer_.current(), tokenizer::token::sub_types::equal);
            tokenizer_.next();
            value = calculateConstExpr(ParseExpression());
            type = getType(value);
        }
        auto var = Node::PNode(new VariableNode(t->get_value_string(), type, VariableNode::VariableType::Const, nullptr, value));
        declarations.back().Symbols[t->get_value_string()] = var;
        require(tokenizer_.current(), tokenizer::token::sub_types::semicolon);
        p->push_back(var);
        t = tokenizer_.next();
    }
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseStatement() {
    auto t = tokenizer_.current();
    switch (t->get_sub_type()) {
    case tokenizer::token::sub_types::begin:
        return ParseCompoundStatement();
    case tokenizer::token::sub_types::if_op:
        return ParseIfStatement();
    case tokenizer::token::sub_types::while_op:
        return ParseWhileStatement();
    case tokenizer::token::sub_types::for_op:
        return ParseForStatement();
    case tokenizer::token::sub_types::repeat:
        return ParseRepeatStatement();
    case tokenizer::token::sub_types::break_op:
        tokenizer_.next();
        return Node::PNode(new Node("break", Node::Type::Empty , t));
    case tokenizer::token::sub_types::continue_op:
        tokenizer_.next();
        return Node::PNode(new Node("continue", Node::Type::Empty, t));
    case tokenizer::token::sub_types::read:
    {
        auto n = Node::PNode(new OperationNode(t->get_value_string(), nullptr, nullptr));
        auto t = tokenizer_.next();
        require(t, tokenizer::token::sub_types::open_parenthesis);
        t = tokenizer_.next();
        require(t, tokenizer::token::sub_types::identifier);
        auto d = findDeclaration(t);
        requireNodeType(d, Node::Type::Variable);
        auto var = std::dynamic_pointer_cast<VariableNode>(d);
        if (var->vType == VariableNode::VariableType::Const)
            throw SyntaxErrorException("Can't read to const variable");
        n->push_back(var);
        while ((t = tokenizer_.next())->get_sub_type() != tokenizer::token::sub_types::close_parenthesis) {
            require(t, tokenizer::token::sub_types::comma);
            t = tokenizer_.next();
            require(t, tokenizer::token::sub_types::identifier);
            d = findDeclaration(t);
            requireNodeType(d, Node::Type::Variable);
            var = std::dynamic_pointer_cast<VariableNode>(d);
            if (var->vType == VariableNode::VariableType::Const)
                throw SyntaxErrorException("Can't read to const variable");
            n->push_back(var);
        }
        tokenizer_.next();
        return n;
    }
    case tokenizer::token::sub_types::write:
    {
        auto n = Node::PNode(new OperationNode(t->get_value_string(), nullptr, nullptr));
        auto t = tokenizer_.next();
        require(t, tokenizer::token::sub_types::open_parenthesis);
        t = tokenizer_.next();
        if (t->get_sub_type() == tokenizer::token::sub_types::string_const) {
            n->push_back(Node::PNode(new StringNode(t)));
            t = tokenizer_.next();
        }
        else {
            n->push_back(ParseExpression());
            t = tokenizer_.current();
        }
        while (t->get_sub_type() != tokenizer::token::sub_types::close_parenthesis) {
            require(t, tokenizer::token::sub_types::comma);
            t = tokenizer_.next();
            if (t->get_sub_type() == tokenizer::token::sub_types::string_const) {
                n->push_back(Node::PNode(new StringNode(t)));
                t = tokenizer_.next();
            }
            else {
                n->push_back(ParseExpression());
                t = tokenizer_.current();
            }
        }
        tokenizer_.next();
        return n;
    }
    case tokenizer::token::sub_types::identifier:
    {
        auto d = findDeclaration(t);
        tokenizer_.next();
        if (d->MyType == Node::Type::Variable) {
            auto var = std::dynamic_pointer_cast<VariableNode>(d);
            if (var->type->MyTypeIdentifier == TypeNode::TypeIdentifier::Array)
                d = ParseArrayIndex(var, var->type);
            else if (var->type->MyTypeIdentifier == TypeNode::TypeIdentifier::Record)
                d = ParseField(var, var->type);
            t = tokenizer_.current();
            if (t->get_sub_type() == tokenizer::token::sub_types::assign ||
                t->get_sub_type() == tokenizer::token::sub_types::plus_assign ||
                t->get_sub_type() == tokenizer::token::sub_types::minus_assign ||
                t->get_sub_type() == tokenizer::token::sub_types::mult_assign ||
                t->get_sub_type() == tokenizer::token::sub_types::divide_assign) {
                tokenizer_.next();
                auto e = ParseExpression();
                if (var->vType == VariableNode::VariableType::Const)
                    throw SyntaxErrorException("Can't assign to const variable");
                requireTypesCompatibility(getType(d), getType(e), t, true, false);;
                return Node::PNode(new OperationNode(t->get_value_string(), nullptr, t, d, e));
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

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseIfStatement() {
    auto n = Node::PNode(new Node("if", Node::Type::Empty, nullptr));
    tokenizer_.next();
    n->push_back(ParseExpression());
    require(tokenizer_.current(), tokenizer::token::sub_types::then);
    tokenizer_.next();
    n->push_back(ParseStatement());
    if (tokenizer_.current()->get_sub_type() == tokenizer::token::sub_types::else_op) {
        tokenizer_.next();
        n->push_back(ParseStatement());
    }
    return n;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseWhileStatement() {
    auto n = Node::PNode(new Node("while", Node::Type::Empty, nullptr));
    tokenizer_.next();
    n->push_back(ParseExpression());
    require(tokenizer_.current(), tokenizer::token::sub_types::do_op);
    tokenizer_.next();
    n->push_back(ParseStatement());
    return n;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseForStatement() {
    auto n = Node::PNode(new Node("for", Node::Type::Empty, nullptr));
    auto t = tokenizer_.next();
    require(t, tokenizer::token::sub_types::identifier);
    auto d = findDeclaration(t);
    requireNodeType(d, Node::Type::Variable);
    n->push_back(d);
    require(tokenizer_.next(), tokenizer::token::sub_types::assign);
    tokenizer_.next();
    auto e = ParseExpression();
    requireTypesCompatibility(getType(d), getType(e), tokenizer_.current(), true, false);
    n->push_back(e);
    t = tokenizer_.current();
    if (t->get_sub_type() != tokenizer::token::sub_types::downto)
        require(t, tokenizer::token::sub_types::to);
    n->push_back(Node::PNode(new Node(t->get_value_string(), Node::Type::Empty, nullptr)));
    tokenizer_.next();
    e = ParseExpression();
    requireTypesCompatibility(getType(d), getType(e), tokenizer_.current(), true, false);
    n->push_back(e);
    require(tokenizer_.current(), tokenizer::token::sub_types::do_op);
    tokenizer_.next();
    n->push_back(ParseStatement());
    return n;
}

pascal_compiler::SyntaxAnalyzer::Node::PNode pascal_compiler::SyntaxAnalyzer::ParseRepeatStatement() {
    auto n = Node::PNode(new Node("repeat", Node::Type::Empty, nullptr));
    tokenizer_.next();
    ParseStatementList(n);
    require(tokenizer_.current(), tokenizer::token::sub_types::until);
    tokenizer_.next();
    n->push_back(ParseExpression());
    return n;
}

void pascal_compiler::SyntaxAnalyzer::Parse() {
    tokenizer_.next();
    if (tokenizer_.current() != tokenizer_.get_end_token()) {
        root = ParseProgram();
        require(tokenizer_.current(), tokenizer::token::sub_types::end_of_file);
    }
}

std::string pascal_compiler::SyntaxAnalyzer::ToString() {
    return walk(root, "", true);
}

pascal_compiler::SyntaxAnalyzer::ScalarNode::ScalarType pascal_compiler::SyntaxAnalyzer::getScalarType(Node::PNode n) {
    if (n->MyType == Node::Type::Const)
        return std::dynamic_pointer_cast<ConstantNode>(n)->ScalarType;
    return std::dynamic_pointer_cast<ScalarNode>(n)->MyScalarType;
}

pascal_compiler::SyntaxAnalyzer::TypeNode::PTypeNode pascal_compiler::SyntaxAnalyzer::getConstantType(ConstantNode::PConstantNode n) {
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

pascal_compiler::SyntaxAnalyzer::TypeNode::PTypeNode pascal_compiler::SyntaxAnalyzer::deduceType(Node::PNode left, Node::PNode right, tokenizer::token_p token, bool is_relational) {
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

pascal_compiler::SyntaxAnalyzer::TypeNode::PTypeNode pascal_compiler::SyntaxAnalyzer::getType(Node::PNode n) {
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

const std::string & pascal_compiler::SyntaxAnalyzer::Node::ToString() {
    return name;
}

pascal_compiler::SyntaxAnalyzer::DudlicateIdentifierException::DudlicateIdentifierException(const pascal_compiler::tokenizer::token_p token) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: Dublicate identifier \"%3%\"") % token->get_position().first %
        token->get_position().second % token->get_value_string()));
}

pascal_compiler::SyntaxAnalyzer::IncompatibleTypesException::IncompatibleTypesException(const TypeNode::PTypeNode left, const TypeNode::PTypeNode right, const pascal_compiler::tokenizer::token_p token) {
    if (!right)
        message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: Incompatible type \"%3%\"") % token->get_position().first %
            token->get_position().second % left->ToString()));
    else
        message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: Incompatible types \"%3%\", \"%4%\"") % token->get_position().first %
            token->get_position().second % left->ToString() % right->ToString()));
}

pascal_compiler::SyntaxAnalyzer::IllegalInitializationOrderException::IllegalInitializationOrderException(tokenizer::token_p token) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: Illegal initialization order") % token->get_position().first %
        token->get_position().second));
}

pascal_compiler::SyntaxAnalyzer::InitializationOverloadException::InitializationOverloadException(tokenizer::token_p token) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: Only 1 variable can be initialized") % token->get_position().first %
        token->get_position().second));
}

pascal_compiler::SyntaxAnalyzer::DeclarationNotFoundException::DeclarationNotFoundException(tokenizer::token_p token) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: \"%3%\" declaration not found") % token->get_position().first %
        token->get_position().second % token->get_value_string()));
}

pascal_compiler::SyntaxAnalyzer::FunctionParameterException::FunctionParameterException(const tokenizer::token_p token, int c) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: function doesn't take %3% argument(s)") % token->get_position().first %
        token->get_position().second % c));
}

pascal_compiler::SyntaxAnalyzer::IllegalExpressionException::IllegalExpressionException(const tokenizer::token_p token) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: illegal expresion") % token->get_position().first %
        token->get_position().second));
}

pascal_compiler::SyntaxAnalyzer::IllegalTypeException::IllegalTypeException(const tokenizer::token_p token) {
    message = std::string(boost::str(boost::format("(%1%, %2%) Syntax Error: illegal type") % token->get_position().first %
        token->get_position().second));
}
