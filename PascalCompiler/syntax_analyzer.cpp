#include "syntax_analyzer.hpp"
#include "boost/format.hpp"
#include <iomanip>
#include <vector>
#include <unordered_set>

const My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::integerNode = 
    My::SyntaxAnalyzer::TypeNode::PTypeNode(new My::SyntaxAnalyzer::ScalarNode("integer", My::SyntaxAnalyzer::ScalarNode::ScalarType::Integer));

const My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::realNode =
    My::SyntaxAnalyzer::TypeNode::PTypeNode(new My::SyntaxAnalyzer::ScalarNode("real", My::SyntaxAnalyzer::ScalarNode::ScalarType::Real));

const My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::charNode =
    My::SyntaxAnalyzer::TypeNode::PTypeNode(new My::SyntaxAnalyzer::ScalarNode("char", My::SyntaxAnalyzer::ScalarNode::ScalarType::Char));

const std::string My::SyntaxAnalyzer::Node::TypeNames[] = {
    "",
    "type ",
    "",
    "",
    "call ",
    "index ",
    "procedure ",
    "function ",
    ""
};

const std::string My::SyntaxAnalyzer::TypeNode::TypeIdentifierNames[] = { " scalar", " array", " record", " alias" };
const std::string My::SyntaxAnalyzer::VariableNode::VariableTypeNames[] = { "value ", "const ", "var " };

const My::SyntaxAnalyzer::binaryOpMap_t My::SyntaxAnalyzer::binaryOpMap = {

    { "+", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::plus<double>>(left, right);
            }
    },

    { "-", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::minus<double>>(left, right);
            }
    },

    { "*", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::multiplies<double>>(left, right);
            }
    },

    { "/", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return My::SyntaxAnalyzer::calc<std::divides<double>>(left, right, true);
            }
    },

    { "mod", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                My::SyntaxAnalyzer::requireInteger(left, right);
                long long result = left->GetValue<long long>() % right->GetValue<long long>();
                return My::SyntaxAnalyzer::ConstantNode::PConstantNode(new My::SyntaxAnalyzer::ConstantNode(std::to_string(result),
                    My::Tokenizer::Token::Value(result), left->ScalarType));
            }
    },

    { "div", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                My::SyntaxAnalyzer::requireInteger(left, right);
                return My::SyntaxAnalyzer::calc<std::divides<double>>(left, right);
            }
    },

    { "shl", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                My::SyntaxAnalyzer::requireInteger(left, right);
                long long result = left->GetValue<long long>() << right->GetValue<long long>();
                return My::SyntaxAnalyzer::ConstantNode::PConstantNode(new My::SyntaxAnalyzer::ConstantNode(std::to_string(result),
                    My::Tokenizer::Token::Value(result), left->ScalarType));
            }
    },

    { "shr", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left, My::SyntaxAnalyzer::ConstantNode::PConstantNode right) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                My::SyntaxAnalyzer::requireInteger(left, right);
                long long result = left->GetValue<long long>() >> right->GetValue<long long>();
                return My::SyntaxAnalyzer::ConstantNode::PConstantNode(new My::SyntaxAnalyzer::ConstantNode(std::to_string(result),
                    My::Tokenizer::Token::Value(result), left->ScalarType));
            }
    }

};

const My::SyntaxAnalyzer::unaryOpMap_t My::SyntaxAnalyzer::unaryOpMap = {

    { "-", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                if (left->ScalarType == My::SyntaxAnalyzer::ScalarNode::ScalarType::Integer)
                    return My::SyntaxAnalyzer::ConstantNode::PConstantNode(new My::SyntaxAnalyzer::ConstantNode(std::to_string(-left->GetValue<long long>()),
                        My::Tokenizer::Token::Value(-left->GetValue<long long>()), left->ScalarType));
                return My::SyntaxAnalyzer::ConstantNode::PConstantNode(new My::SyntaxAnalyzer::ConstantNode(std::to_string(-left->GetValue<double>()),
                    My::Tokenizer::Token::Value(-left->GetValue<double>()), left->ScalarType));
            }
    },

    { "+", [](My::SyntaxAnalyzer::ConstantNode::PConstantNode left) -> My::SyntaxAnalyzer::ConstantNode::PConstantNode {
                return left;
            }
    }

};

std::string My::SyntaxAnalyzer::ExpectedException::getMessage(const My::Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type) {
    return boost::str(boost::format("(%4%, %5%) Syntax Error: %1% expected but %2% \"%3%\" found") %
        My::Tokenizer::Token::SubTypesStrings[static_cast<unsigned int>(type)] %
        My::Tokenizer::Token::TypesStrings[static_cast<unsigned int>(token->GetType())] %
        token->GetValueString() % token->GetPosition().first % token->GetPosition().second
    );
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
            throw SyntaxErrorException("Illegal expression"); // only const in constexpr
        return std::dynamic_pointer_cast<ConstantNode>(var->Value);
    }
    case Node::Type::Operation:
    {
        auto op = std::dynamic_pointer_cast<OperationNode>(n);
        auto left = calculateConstExpr(op->Children[0]);
        if (op->Children.size() > 1) {
            for (int i = 1; i < op->Children.size(); ++i) {
                auto right = calculateConstExpr(op->Children[i]);
                requireTypesCompatibility(left, right);
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

void My::SyntaxAnalyzer::requireTypesCompatibility(TypeNode::PTypeNode left, TypeNode::PTypeNode right, bool allow_left_int = true) {
    left = getBaseType(left); right = getBaseType(right);
    if (left == right)
        return;
    if (left->MyTypeIdentifier != right->MyTypeIdentifier)
        throw SyntaxErrorException("Can't convert");
    auto l = std::dynamic_pointer_cast<ScalarNode>(left);
    auto r = std::dynamic_pointer_cast<ScalarNode>(right);
    if (l->MyScalarType == ScalarNode::ScalarType::Char && r->MyScalarType != ScalarNode::ScalarType::Char ||
        !allow_left_int && l->MyScalarType == ScalarNode::ScalarType::Integer && r->MyScalarType == ScalarNode::ScalarType::Real)
        throw SyntaxErrorException("Can't convert");
}

void My::SyntaxAnalyzer::requireTypesCompatibility(TypeNode::PTypeNode left, ConstantNode::PConstantNode right) {
    if (left->MyTypeIdentifier != TypeNode::TypeIdentifier::Scalar)
        throw SyntaxErrorException("Can't convert");
    auto l = std::dynamic_pointer_cast<ScalarNode>(left);
    if (l->MyScalarType == ScalarNode::ScalarType::Real && right->ScalarType == ScalarNode::ScalarType::Integer ||
        l->MyScalarType == right->ScalarType)
        return;
    throw SyntaxErrorException("Can't convert");
}

void My::SyntaxAnalyzer::requireTypesCompatibility(TypeNode::PTypeNode left, TypeNode::TypeIdentifier t) {
    if (left->MyTypeIdentifier != t)
        throw SyntaxErrorException("Can't convert");
}

void My::SyntaxAnalyzer::requireTypesCompatibility(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right) {
    if (left->ScalarType != ScalarNode::ScalarType::Char && left->ScalarType != ScalarNode::ScalarType::Char)
        return;
    throw SyntaxErrorException("Can't convert");
}

void My::SyntaxAnalyzer::requireInteger(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right) {
    if (left->ScalarType != My::SyntaxAnalyzer::ScalarNode::ScalarType::Integer ||
        right->ScalarType != My::SyntaxAnalyzer::ScalarNode::ScalarType::Integer)
        throw My::SyntaxAnalyzer::SyntaxErrorException("Illegal expression");
}

void My::SyntaxAnalyzer::requireArgsCompatibility(Node::PNode f, Node::PNode n) {
    //TODO More info
    int i = 0;
    for (; i < f->Children.size() && f->Children[i]->MyType == Node::Type::Variable; ++i) {
        if (i == n->Children.size())
            throw SyntaxErrorException("Function does not take");
        requireTypesCompatibility(getType(f->Children[i]), getType(n->Children[i]), false);
    }
    if (i != n->Children.size())
        throw SyntaxErrorException("Function does not take");
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
        //TODO MORE INFO
        throw SyntaxErrorException("Illegal type");
}

void My::SyntaxAnalyzer::requireDeclaration(const My::Tokenizer::PToken t) {
    for (auto it = declarations.rbegin(); it != declarations.rend(); ++it)
        if (it->Symbols.count(t->GetValueString()) > 0)
            return;
    //TODO more info
    throw SyntaxErrorException("Declaration not found");
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
        requireTypesCompatibility(type, TypeNode::TypeIdentifier::Record);
        t = tokenizer.Next();
        require(t, Tokenizer::Token::SubTypes::Identifier);
        for (auto it : type->Children)
            if (it->ToString() == t->GetValueString()) {
                auto var = std::dynamic_pointer_cast<VariableNode>(it);
                n = Node::PNode(new OperationNode(".", var->type, n, var));
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
    auto r = Node::PNode(new OperationNode("()", f->ReturnType, f, args));
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
    return Node::PNode(new OperationNode("()", nullptr, n, args));
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseArrayIndex(Node::PNode n, TypeNode::PTypeNode type) {
    auto t = tokenizer.Current();
    while (t->GetSubType() == Tokenizer::Token::SubTypes::OpenBracket) {
        requireTypesCompatibility(type, TypeNode::TypeIdentifier::Array);
        auto a = std::dynamic_pointer_cast<ArrayNode>(type);
        tokenizer.Next();
        n = Node::PNode(new OperationNode("[]", a->type, n, ParseExpression()));
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
    if (simpleExpressionOperators(token->GetSubType())) {
        tokenizer.Next();
        auto f = ParseFactor();
        return Node::PNode(new OperationNode(token->GetValueString(), getType(f), f));
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
            throw SyntaxAnalyzer("Incopatible type");
        }
    }
    case Tokenizer::Token::SubTypes::IntegerConst:
        tokenizer.Next();
        return Node::PNode(new ConstantNode(token->GetString(), token->GetValue(), ScalarNode::ScalarType::Integer));
    case Tokenizer::Token::SubTypes::FloatConst:
        tokenizer.Next();
        return Node::PNode(new ConstantNode(token->GetString(), token->GetValue(), ScalarNode::ScalarType::Real));
    case Tokenizer::Token::SubTypes::CharConst:
        tokenizer.Next();
        return Node::PNode(new ConstantNode(token->GetString(), token->GetValue(), ScalarNode::ScalarType::Char));
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
    auto n = Node::PNode(new Node(t->GetValueString(), Node::Type::Block, ParseBlock()));
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Dot);
    declarations.pop_back();
    declarations.pop_back();
    tokenizer.Next();
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseBlock() {
    auto d = ParseDeclarationPart();
    auto n = Node::PNode(new Node("block", Node::Type::Block, d, ParseCompoundStatement()));
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseDeclarationPart() {
    auto n = Node::PNode(new Node("decl", Node::Type::Block));
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
    //TODO rewrite???
    type = getBaseType(type);
    if (type->MyTypeIdentifier == TypeNode::TypeIdentifier::Scalar)
        return ParseExpression();
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::OpenParenthesis);
    Node::PNode n = Node::PNode(new TypedConstantNode(type));
    if (type->MyTypeIdentifier == TypeNode::TypeIdentifier::Array) {
        auto a = std::dynamic_pointer_cast<ArrayNode>(type);
        //TODO calculate possible expression
        auto end = std::dynamic_pointer_cast<ConstantNode>(a->Children[1])->GetValue<long long>();
        for (auto i = std::dynamic_pointer_cast<ConstantNode>(a->Children[0])->GetValue<long long>();
            i <= end; ++i) {
            tokenizer.Next();
            n->push_back(ParseTypedConst(a->type));
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
            //TODO more info
            throw SyntaxErrorException("Illegal initialization order");
        tokenizer.Next();
        n->push_back(ParseTypedConst(it->type));
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
    auto n = Node::PNode(new Node("p_list", Node::Type::Block));
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
    auto n = Node::PNode(new ProcedureNode(t->GetValueString(), pl, ParseBlock()));
    declarations.pop_back();
    declarations.back().Symbols[n->ToString()] = n;
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseFunction() {
    auto t = tokenizer.Current();
    require(t, Tokenizer::Token::SubTypes::Identifier);
    requireUnique(t, declarations.back().Symbols);
    declarations.push_back(Declaration());
    tokenizer.Next();
    auto pl = ParseFormalParameterList();
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
    auto rt = tokenizer.Next();
    require(rt, Tokenizer::Token::SubTypes::Identifier);
    require(tokenizer.Next(), Tokenizer::Token::SubTypes::Semicolon);
    tokenizer.Next();
    auto type = findDeclaration(rt);
    requireNodeType(type, Node::Type::Type);
    auto n = Node::PNode(new FunctionNode(t->GetValueString(), pl, std::dynamic_pointer_cast<TypeNode>(type), ParseBlock()));
    declarations.pop_back();
    declarations.back().Symbols[n->ToString()] = n;
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseFormalParameterList() {
    auto n = Node::PNode(new Node("p_list", Node::Type::Block));
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
                n->push_back(Node::PNode(new VariableNode(it, std::dynamic_pointer_cast<TypeNode>(type), VariableNode::VariableType::Var)));
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
            //TODO rewrite
            auto type = findDeclaration(tokenizer.Current());
            requireNodeType(type, Node::Type::Type);
            for (auto it : ids) {
                n->push_back(Node::PNode(new VariableNode(it, std::dynamic_pointer_cast<TypeNode>(type), v_type)));
                declarations.back().Symbols[it] = n->Children.back();
            }
            if (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::Equal) {
                if (ids.size() > 1)
                    //TODO add more info
                    throw SyntaxErrorException("Only 1 variable can be initialized");
                //TODO check type
                tokenizer.Next();
                //TODO check constexpr 
                auto value = calculateConstExpr(ParseExpression());
                auto var = std::dynamic_pointer_cast<VariableNode>(n->Children.back());
                requireTypesCompatibility(var->type, value);
                var->SetValue(value);
                end = true;
                last = true;
            }
        }
        if (end && !last)
            //TODO more info
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
    auto n = Node::PNode(new Node("statements", Node::Type::Block));
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
        return TypeNode::PTypeNode(new TypeAliasNode(name, type));
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
        return TypeNode::PTypeNode(new ArrayNode(name, from, to, ParseType()));
    }
    case Tokenizer::Token::SubTypes::Record:
    {
        auto t = tokenizer.Next();
        auto n = TypeNode::PTypeNode(new RecordNode(name));
        while (t->GetSubType() == Tokenizer::Token::SubTypes::Identifier) {
            require(tokenizer.Next(), Tokenizer::Token::SubTypes::Colon);
            for (auto it : n->Children)
                if (it->ToString() == t->GetValueString())
                    throw DudlicateIdentifierException(t);
            tokenizer.Next();
            n->push_back(Node::PNode(new VariableNode(t->GetValueString(), ParseType(), VariableNode::VariableType::Value)));
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
        //TODO make function
        //TODO use parse identifier list function
        requireUnique(t, declarations.back().Symbols);
        std::vector<std::string> vars;
        vars.push_back(t->GetValueString());
        while (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::Comma) {
            require(t = tokenizer.Next(), Tokenizer::Token::SubTypes::Identifier);
            requireUnique(t, declarations.back().Symbols);
            vars.push_back(t->GetValueString());
        }
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
        tokenizer.Next();
        auto type = ParseType();
        for (auto it : vars) {
            auto var = Node::PNode(new VariableNode(it, type, VariableNode::VariableType::Value));
            declarations.back().Symbols[it] = var;
            p->push_back(var);
        }
        if (tokenizer.Current()->GetSubType() == Tokenizer::Token::SubTypes::Equal) {
            if (vars.size() > 1)
                //TODO more info
                throw SyntaxErrorException("Only 1 variable can be initialized");
            tokenizer.Next();
            //check type!!!
            std::dynamic_pointer_cast<VariableNode>(p->Children.back())->SetValue(ParseTypedConst(type));
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
        }
        auto var = Node::PNode(new VariableNode(t->GetValueString(), type, VariableNode::VariableType::Const, value));
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
        auto n = Node::PNode(new OperationNode(t->GetValueString(), nullptr));
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
        auto n = Node::PNode(new OperationNode(t->GetValueString(), nullptr));
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
                deduceType(d, e, false);
                return Node::PNode(new OperationNode(t->GetValueString(), nullptr, d, e));
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
    auto n = Node::PNode(new Node("if", Node::Type::Block));
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
    auto n = Node::PNode(new Node("while", Node::Type::Block));
    tokenizer.Next();
    n->push_back(ParseExpression());
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Do);
    tokenizer.Next();
    n->push_back(ParseStatement());
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseForStatement() {
    auto n = Node::PNode(new Node("for", Node::Type::Block));
    auto t = tokenizer.Next();
    require(t, Tokenizer::Token::SubTypes::Identifier);
    //TODO check variable
    auto d = findDeclaration(t);
    requireNodeType(d, Node::Type::Variable);
    //TODO check scalar
    n->push_back(d);
    require(tokenizer.Next(), Tokenizer::Token::SubTypes::Assign);
    tokenizer.Next();
    n->push_back(ParseExpression());
    t = tokenizer.Current();
    if (t->GetSubType() != Tokenizer::Token::SubTypes::Downto)
        require(t, Tokenizer::Token::SubTypes::To);
    n->push_back(Node::PNode(new Node(t->GetValueString(), Node::Type::Block)));
    tokenizer.Next();
    n->push_back(ParseExpression());
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Do);
    tokenizer.Next();
    n->push_back(ParseStatement());
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseRepeatStatement() {
    auto n = Node::PNode(new Node("repeat", Node::Type::Block));
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

My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::deduceType(Node::PNode left, Node::PNode right, bool allow_left_int) {
    auto lt = getBaseType(getType(left)), rt = getBaseType(getType(right));
    requireTypesCompatibility(lt, rt, allow_left_int);
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
    return std::dynamic_pointer_cast<TypeNode>(n);
}

const std::string & My::SyntaxAnalyzer::Node::ToString() {
    return name;
}

std::string My::SyntaxAnalyzer::DudlicateIdentifierException::getMessage(const My::Tokenizer::PToken token) {
    return boost::str(boost::format("(%1%, %2%) Syntax Error: Dublicate identifier \"%3%\"") % token->GetPosition().first %
        token->GetPosition().second % token->GetValueString());
}
