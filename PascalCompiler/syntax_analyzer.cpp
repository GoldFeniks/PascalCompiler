#include "syntax_analyzer.hpp"
#include "boost/format.hpp"
#include <iomanip>
#include <vector>
#include <unordered_set>

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
    return std::string(boost::str(boost::format("(%4%, %5%) Syntax Error: %1% expected but %2% \"%3%\" found") %
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
        if (!var->IsConst || var->type != nullptr)
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
    //TODO allow char
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
    //TODO allow char
    if (left->MyTypeIdentifier != TypeNode::TypeIdentifier::Scalar)
        throw SyntaxErrorException("Can't convert");
    auto l = std::dynamic_pointer_cast<ScalarNode>(left);
    if (l->MyScalarType == ScalarNode::ScalarType::Real && right->ScalarType == ScalarNode::ScalarType::Integer ||
        l->MyScalarType == right->ScalarType)
        return;
    throw SyntaxErrorException("Can't convert");
}

void My::SyntaxAnalyzer::requireTypesCompatibility(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right) {
    if (left->ScalarType != ScalarNode::ScalarType::Char && left->ScalarType != ScalarNode::ScalarType::Char)
        return;
}

void My::SyntaxAnalyzer::requireInteger(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right) {
    if (left->ScalarType != My::SyntaxAnalyzer::ScalarNode::ScalarType::Integer ||
        right->ScalarType != My::SyntaxAnalyzer::ScalarNode::ScalarType::Integer)
        throw My::SyntaxAnalyzer::SyntaxErrorException("Illegal expression");
}

My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::getBaseType(TypeNode::PTypeNode type) {
    while (type->MyTypeIdentifier == TypeNode::TypeIdentifier::Alias)
        type = std::dynamic_pointer_cast<TypeNode>(type->Children[0]);
    return type;
}

My::SyntaxAnalyzer::TypeNode::PTypeNode My::SyntaxAnalyzer::findTypeDeclaration(const std::string& name) {
    for (auto it = declarations.rbegin(); it != declarations.rend(); ++it) {
        auto t = it->Types.find(name);
        if (t != it->Types.end())
            return std::dynamic_pointer_cast<TypeNode>(t->second);
    }
    return nullptr;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::findVarDeclaration(const std::string& name) {
    for (auto it = declarations.rbegin(); it != declarations.rend(); ++it) {
        auto t = it->Vars.find(name);
        if (t != it->Vars.end())
            return t->second;
    }
    return nullptr;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::findProcedureDeclaration(const std::string& name) {
    for (auto it = declarations.rbegin(); it != declarations.rend(); ++it) {
        auto t = it->Procedures.find(name);
        if (t != it->Procedures.end())
            return t->second;
    }
    return nullptr;
}

void My::SyntaxAnalyzer::requireFirstDecl(const std::string& name) {
    requireUnique(name, declarations.back().Types);
    requireUnique(name, declarations.back().Vars);
    requireUnique(name, declarations.back().Procedures);
}

void My::SyntaxAnalyzer::requireDeclaration(const std::string& name) {
    //TODO accept token
    for (auto it = declarations.rbegin(); it != declarations.rend(); ++it)
        if (it->Types.count(name) > 0 ||
            it->Vars.count(name) > 0 ||
            it->Procedures.count(name) > 0)
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
    if (node->MyType == Node::Type::Variable && std::dynamic_pointer_cast<VariableNode>(node)->IsConst)
        type = "const ";
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

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseField(Node::PNode var) {
    auto t = tokenizer.Current();
    if (t->GetSubType() == Tokenizer::Token::SubTypes::Dot) {
        auto type = std::dynamic_pointer_cast<RecordNode>(var->Children[0]);
        if (!type)
            throw SyntaxErrorException("Operand should be record");
        auto ti = tokenizer.Next();
        require(ti, Tokenizer::Token::SubTypes::Identifier);
        for (auto it : type->Children)
            if (it->ToString() == ti->GetStringValue()) {
                tokenizer.Next();
                return Node::PNode(new OperationNode(t, var, ParseField(it)));
            }
    }
    else
        return var;
    throw SyntaxErrorException("Field not found");
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseFactor() {
    auto token = tokenizer.Current();
    if (simpleExpressionOperators(token->GetSubType())) {
        tokenizer.Next();
        return Node::PNode(new OperationNode(token, ParseFactor()));
    }
    switch (token->GetSubType()) {
    case Tokenizer::Token::SubTypes::Identifier:
    {
        //TODO somehow check for type
        requireDeclaration(token->GetStringValue());
        auto t = tokenizer.Next();
        if (t->GetSubType() == Tokenizer::Token::SubTypes::OpenParenthesis) {
            tokenizer.Next();
            auto n = Node::PNode(new CallNode(token->GetStringValue(), ParseActualParameterList()));
            return n;
        }
        if (t->GetSubType() == Tokenizer::Token::SubTypes::OpenBracket) {
            tokenizer.Next();
            auto n = Node::PNode(new Node("i_list", Node::Type::Block));
            ParseExprressionList(n);
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseBracket);
            tokenizer.Next();
            return Node::PNode(new IndexNode(token->GetValueString(), n));
        }
        if (t->GetSubType() == Tokenizer::Token::SubTypes::Dot) {
            return ParseField(findVarDeclaration(token->GetStringValue()));
        }
        return findVarDeclaration(token->GetStringValue());
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
    declarations.back().Types["integer"] = Node::PNode(new ScalarNode("integer", ScalarNode::ScalarType::Integer));
    declarations.back().Types["real"] = Node::PNode(new ScalarNode("real", ScalarNode::ScalarType::Real));
    declarations.back().Types["char"] = Node::PNode(new ScalarNode("char", ScalarNode::ScalarType::Char));

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
            n->Add(ParseProcedure());
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
            tokenizer.Next();
            break;
        case Tokenizer::Token::SubTypes::Function:
            tokenizer.Next();
            n->Add(ParseFunction());
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
    p->Add(ParseStatement());
    while (tokenizer.Current()->GetSubType() == Tokenizer::Token::SubTypes::Semicolon) {
        tokenizer.Next();
        p->Add(ParseStatement());
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
            n->Add(ParseTypedConst(a->type));
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
        n->Add(ParseTypedConst(it->type));
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
    ParseExprressionList(n);
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseParenthesis);
    tokenizer.Next();
    return n;
}

void My::SyntaxAnalyzer::ParseExprressionList(Node::PNode p) {
    p->Add(ParseExpression());
    while (tokenizer.Current()->GetSubType() == Tokenizer::Token::SubTypes::Comma) {
        tokenizer.Next();
        p->Add(ParseExpression());
    }
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseProcedure() {
    auto t = tokenizer.Current();
    require(t, Tokenizer::Token::SubTypes::Identifier);
    requireFirstDecl(t->GetValueString());
    declarations.push_back(Declaration());
    tokenizer.Next();
    auto pl = ParseFormalParameterList();
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
    tokenizer.Next();
    auto n = Node::PNode(new ProcedureNode(t->GetValueString(), pl, ParseBlock()));
    declarations.pop_back();
    declarations.back().Procedures[n->ToString()] = n;
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseFunction() {
    auto t = tokenizer.Current();
    require(t, Tokenizer::Token::SubTypes::Identifier);
    requireFirstDecl(t->GetValueString());
    declarations.push_back(Declaration());
    tokenizer.Next();
    auto pl = ParseFormalParameterList();
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
    auto rt = tokenizer.Next();
    require(rt, Tokenizer::Token::SubTypes::Identifier);
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
    tokenizer.Next();
    auto n = Node::PNode(new FunctionNode(t->GetValueString(), pl, findTypeDeclaration(rt->GetValueString()), ParseBlock()));
    declarations.pop_back();
    declarations.back().Procedures[n->ToString()] = n;
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseFormalParameterList() {
    auto n = Node::PNode(new Node("p_list", Node::Type::Block));
    auto t = tokenizer.Current();
    auto set = std::unordered_set<std::string>();
    bool end = false;
    while (t->GetSubType() != Tokenizer::Token::SubTypes::CloseParenthesis) {
        Node::PNode c;
        bool last = false;
        if (t->GetSubType() == Tokenizer::Token::SubTypes::Var) {
            c = Node::PNode(new Node("var", Node::Type::Block));
            tokenizer.Next();
            ParseIdentifierList(c, set);
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
            require(tokenizer.Next(), Tokenizer::Token::SubTypes::Identifier);
            auto type = findTypeDeclaration(tokenizer.Current()->GetValueString());
            for (int i = 0; i < c->Children.size(); ++i) {
                c->Children[i] = Node::PNode(new VariableNode(c->Children[i]->ToString(), type, false));
                declarations.back().Vars[c->Children[i]->ToString()] = c->Children[i];
            }
            tokenizer.Next();
        }
        else {
            bool is_const = false;
            if (t->GetSubType() == Tokenizer::Token::SubTypes::Const) {
                c = Node::PNode(new Node("const", Node::Type::Block));
                is_const = true;
                tokenizer.Next();
            }
            else {
                require(t, Tokenizer::Token::SubTypes::Identifier);
                c = Node::PNode(new Node("value", Node::Type::Block));
            }
            ParseIdentifierList(c, set);
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
            require(tokenizer.Next(), Tokenizer::Token::SubTypes::Identifier);
            //TODO rewrite
            auto type = findTypeDeclaration(tokenizer.Current()->GetValueString());
            for (int i = 0; i < c->Children.size(); ++i) {
                c->Children[i] = Node::PNode(new VariableNode(c->Children[i]->ToString(), type, is_const));
                declarations.back().Vars[c->Children[i]->ToString()] = c->Children[i];
            }
            if (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::Equal) {
                if (c->Children.size() > 1)
                    //TODO add more info
                    throw SyntaxErrorException("Only 1 variable can be initialized");
                //TODO check type
                tokenizer.Next();
                //TODO check constexpr 
                auto value = calculateConstExpr(ParseExpression());
                auto var = std::dynamic_pointer_cast<VariableNode>(c->Children.back());
                requireTypesCompatibility(var->type, value);
                var->SetValue(value);
                c->Add(var);
                declarations.back().Vars[var->ToString()] = var;
                end = true;
                last = true;
            }
        }
        if (end && !last)
            //TODO more info
            throw SyntaxErrorException("Default parameter should be last");
        n->Add(c);
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
        requireFirstDecl(t->GetStringValue());
        require(tokenizer.Next(), Tokenizer::Token::SubTypes::Equal);
        tokenizer.Next();
        auto type = ParseType(t->GetStringValue());
        p->Add(type);
        declarations.back().Types[t->GetStringValue()] = type;
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
        requireDeclaration(t->GetStringValue());
        auto type = findTypeDeclaration(t->GetStringValue());
        if (name == "anonymous")
            return type;
        return TypeNode::PTypeNode(new TypeAliasNode(name, type));
    }
    case Tokenizer::Token::SubTypes::Array:
    {
        require(tokenizer.Next(), Tokenizer::Token::SubTypes::OpenBracket);
        tokenizer.Next();
        auto from = ParseExpression();
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Range);
        tokenizer.Next();
        auto to = ParseExpression();
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseBracket);
        require(tokenizer.Next(), Tokenizer::Token::SubTypes::Of);
        tokenizer.Next();
        return TypeNode::PTypeNode(new ArrayNode(name, from, to, ParseType()));
    }
    case Tokenizer::Token::SubTypes::Record:
    {
        auto t = tokenizer.Next();
        auto n = TypeNode::PTypeNode(new RecordNode(name));
        //TODO identifier_list
        while (t->GetSubType() == Tokenizer::Token::SubTypes::Identifier) {
            require(tokenizer.Next(), Tokenizer::Token::SubTypes::Colon);
            for (auto it : n->Children)
                if (it->Children.front()->ToString() == t->GetValueString())
                    //TODO more info
                    throw SyntaxErrorException("Dublicate identifier");
            tokenizer.Next();
            n->Add(Node::PNode(new VariableNode(t->GetValueString(), ParseType(), false)));
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
        requireFirstDecl(t->GetValueString());
        std::vector<std::string> vars;
        vars.push_back(t->GetValueString());
        while (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::Comma) {
            require(t = tokenizer.Next(), Tokenizer::Token::SubTypes::Identifier);
            requireFirstDecl(t->GetValueString());
            vars.push_back(t->GetValueString());
        }
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
        tokenizer.Next();
        auto type = ParseType();
        for (auto it : vars) {
            auto var = Node::PNode(new VariableNode(it, type, false));
            declarations.back().Vars[it] = var;
            p->Add(var);
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
        requireFirstDecl(t->GetValueString());
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
            //deduce type
        }
        auto var = Node::PNode(new VariableNode(t->GetValueString(), type, true, value));
        declarations.back().Vars[t->GetValueString()] = var;
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
        p->Add(var);
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
        auto n = Node::PNode(new OperationNode(t));
        auto t = tokenizer.Next();
        require(t, Tokenizer::Token::SubTypes::OpenParenthesis);
        t = tokenizer.Next();
        require(t, Tokenizer::Token::SubTypes::Identifier);
        requireDeclaration(t->GetStringValue());
        auto var = std::dynamic_pointer_cast<VariableNode>(findVarDeclaration(t->GetStringValue()));
        if (var->IsConst)
            throw SyntaxErrorException("Can't read to const variable");
        n->Add(var);
        while ((t = tokenizer.Next())->GetSubType() != Tokenizer::Token::SubTypes::CloseParenthesis) {
            require(t, Tokenizer::Token::SubTypes::Comma);
            t = tokenizer.Next();
            require(t, Tokenizer::Token::SubTypes::Identifier);
            requireDeclaration(t->GetStringValue());
            auto var = std::dynamic_pointer_cast<VariableNode>(findVarDeclaration(t->GetStringValue()));
            if (var->IsConst)
                throw SyntaxErrorException("Can't read to const variable");
            n->Add(var);
        }
        tokenizer.Next();
        return n;
    }
    case Tokenizer::Token::SubTypes::Write:
    {
        auto n = Node::PNode(new OperationNode(t));
        auto t = tokenizer.Next();
        require(t, Tokenizer::Token::SubTypes::OpenParenthesis);
        t = tokenizer.Next();
        if (t->GetSubType() == Tokenizer::Token::SubTypes::StringConst) {
            n->Add(Node::PNode(new StringNode(t)));
            t = tokenizer.Next();
        }
        else {
            n->Add(ParseExpression());
            t = tokenizer.Current();
        }
        while (t->GetSubType() != Tokenizer::Token::SubTypes::CloseParenthesis) {
            require(t, Tokenizer::Token::SubTypes::Comma);
            t = tokenizer.Next();
            if (t->GetSubType() == Tokenizer::Token::SubTypes::StringConst) {
                n->Add(Node::PNode(new StringNode(t)));
                t = tokenizer.Next();
            }
            else {
                n->Add(ParseExpression());
                t = tokenizer.Current();
            }
        }
        tokenizer.Next();
        return n;
    }
    case Tokenizer::Token::SubTypes::Identifier:
    {
        requireDeclaration(t->GetStringValue());
        std::string name = t->GetValueString();
        t = tokenizer.Next();
        auto n = findVarDeclaration(name);
        if (t->GetSubType() == Tokenizer::Token::SubTypes::Dot) {
            n = ParseField(n);
            t = tokenizer.Current();
        }
        if (t->GetSubType() == Tokenizer::Token::SubTypes::Assign ||
            t->GetSubType() == Tokenizer::Token::SubTypes::PlusAssign ||
            t->GetSubType() == Tokenizer::Token::SubTypes::MinusAssign ||
            t->GetSubType() == Tokenizer::Token::SubTypes::MultAssign ||
            t->GetSubType() == Tokenizer::Token::SubTypes::DivideAssign) {
            tokenizer.Next();
            return Node::PNode(new OperationNode(t, n, ParseExpression()));
        }
        return Node::PNode(new CallNode(name, ParseActualParameterList()));
    }
    default:
        break;
    }
    return nullptr;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseIfStatement() {
    auto n = Node::PNode(new Node("if", Node::Type::Block));
    tokenizer.Next();
    n->Add(ParseExpression());
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Then);
    tokenizer.Next();
    n->Add(ParseStatement());
    if (tokenizer.Current()->GetSubType() == Tokenizer::Token::SubTypes::Else) {
        tokenizer.Next();
        n->Add(ParseStatement());
    }
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseWhileStatement() {
    auto n = Node::PNode(new Node("while", Node::Type::Block));
    tokenizer.Next();
    n->Add(ParseExpression());
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Do);
    tokenizer.Next();
    n->Add(ParseStatement());
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseForStatement() {
    auto n = Node::PNode(new Node("for", Node::Type::Block));
    auto t = tokenizer.Next();
    require(t, Tokenizer::Token::SubTypes::Identifier);
    //TODO check variable
    requireDeclaration(t->GetValueString());
    n->Add(findVarDeclaration(t->GetValueString()));
    require(tokenizer.Next(), Tokenizer::Token::SubTypes::Assign);
    tokenizer.Next();
    n->Add(ParseExpression());
    t = tokenizer.Current();
    if (t->GetSubType() != Tokenizer::Token::SubTypes::Downto)
        require(t, Tokenizer::Token::SubTypes::To);
    n->Add(Node::PNode(new Node(t->GetValueString(), Node::Type::Block)));
    tokenizer.Next();
    n->Add(ParseExpression());
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Do);
    tokenizer.Next();
    n->Add(ParseStatement());
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseRepeatStatement() {
    auto n = Node::PNode(new Node("repeat", Node::Type::Block));
    tokenizer.Next();
    ParseStatementList(n);
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Until);
    tokenizer.Next();
    n->Add(ParseExpression());
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

const std::string & My::SyntaxAnalyzer::Node::ToString() {
    return name;
}
