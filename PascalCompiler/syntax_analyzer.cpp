#include "syntax_analyzer.hpp"
#include "boost/format.hpp"
#include <iomanip>
#include <vector>
#include <unordered_set>

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

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::findTypeDeclaration(const std::string& name) {
    for (auto it = declarations.rbegin(); it != declarations.rend(); ++it) {
        auto t = it->Types.find(name);
        if (t != it->Types.end())
            return t->second;
    }
    return nullptr; //declaration not found
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
        t = it->Functions.find(name);
        if (t != it->Procedures.end())
            return t->second;
    }
    return nullptr;
}

void My::SyntaxAnalyzer::requireFirstDecl(const std::string& name) {
    requireUnique(name, declarations.back().Types);
    requireUnique(name, declarations.back().Vars);
    requireUnique(name, declarations.back().Consts);
    requireUnique(name, declarations.back().Procedures);
    requireUnique(name, declarations.back().Functions);
}

void My::SyntaxAnalyzer::requireDeclaration(const std::string& name) {
    for (auto it = declarations.rbegin(); it != declarations.rend(); ++it)
        if (it->Types.count(name) > 0 ||
            it->Vars.count(name) > 0 ||
            it->Consts.count(name) > 0 ||
            it->Functions.count(name) > 0 ||
            it->Procedures.count(name) > 0)
            return;
    throw std::exception(); //declaration not found
}

void My::SyntaxAnalyzer::require(const Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type) {
    if (token->GetSubType() != type)
        throw SyntaxErrorException(token, type);
}

std::string My::SyntaxAnalyzer::walk(const Node::PNode node, std::string prefix, bool last) {
    if (node == nullptr)
        return std::string();
    std::string result = boost::str(boost::format("%1%%2%%3%\n") % prefix % 
        (last ? "\xE2\x94\x94\xE2\x94\x80" : "\xE2\x94\x9C\xE2\x94\x80") % node->ToString());
    if (!node->Children.size())
        return result;
    std::string spaces = std::string(node->ToString().length() - 1, ' ');
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
    return parse<Operation, &SyntaxAnalyzer::ParseSimpleExpression, expressionOperators>(ParseSimpleExpression());
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseSimpleExpression() {
    return parse<Operation, &SyntaxAnalyzer::ParseTerm, simpleExpressionOperators>(ParseTerm());
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseTerm() {
    return parse<Operation, &SyntaxAnalyzer::ParseFactor, termOperators>(ParseFactor());
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseFactor() {
    auto token = tokenizer.Current();
    if (simpleExpressionOperators(token->GetSubType())) {
        tokenizer.Next();
        return Node::PNode(new UnaryOperation(token, ParseFactor()));
    }
    switch (token->GetSubType()) {
    case Tokenizer::Token::SubTypes::Identifier:
    {
        //TODO somehow check for type
        requireDeclaration(token->GetStringValue());
        auto t = tokenizer.Next();
        if (t->GetSubType() == Tokenizer::Token::SubTypes::OpenParenthesis) {
            tokenizer.Next();
            auto n = Node::PNode(new Node("call", Node::PNode(new Node(token->GetStringValue())), ParseActualParameterList()));
            return n;
        }
        if (t->GetSubType() == Tokenizer::Token::SubTypes::OpenBracket) {
            tokenizer.Next();
            auto n = Node::PNode(new Node("index", Node::PNode(new Node(token->GetStringValue()))));
            ParseExprressionList(n);
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseBracket);
            tokenizer.Next();
            return n;
        }
        return My::SyntaxAnalyzer::ExpressionNode::PExpressionNode(new VariableNode(token));
    }
    case Tokenizer::Token::SubTypes::IntegerConst:
        tokenizer.Next();
        return My::SyntaxAnalyzer::ExpressionNode::PExpressionNode(new IntNode(token));
    case Tokenizer::Token::SubTypes::FloatConst:
        tokenizer.Next();
        return My::SyntaxAnalyzer::ExpressionNode::PExpressionNode(new FloatNode(token));
    case Tokenizer::Token::SubTypes::CharConst:
        tokenizer.Next();
        return My::SyntaxAnalyzer::ExpressionNode::PExpressionNode(new CharNode(token));
    case Tokenizer::Token::SubTypes::OpenParenthesis:
    {
        tokenizer.Next();
        auto e = ParseExpression();
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseParenthesis);
        tokenizer.Next();
        return e;
    }
    default:
        throw SyntaxErrorException(token, Tokenizer::Token::SubTypes::Identifier);
    }
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseProgram() {
    //Init base types
    declarations.push_back(Declaration());
    declarations.back().Types["integer"] = Node::PNode(new Node("integer"));
    declarations.back().Types["real"] = Node::PNode(new Node("real"));
    declarations.back().Types["char"] = Node::PNode(new Node("char"));

    auto t = tokenizer.Current();
    require(t, Tokenizer::Token::SubTypes::Program);
    t = tokenizer.Next();
    require(t, Tokenizer::Token::SubTypes::Identifier);
    require(tokenizer.Next(), Tokenizer::Token::SubTypes::Semicolon);
    tokenizer.Next();
    declarations.push_back(Declaration());
    auto n = Node::PNode(new Node("program", Node::PNode(new Node(t->GetValueString())), ParseBlock()));
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Dot);
    declarations.pop_back();
    declarations.pop_back();
    tokenizer.Next();
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseBlock() {
    auto d = ParseDeclarationPart();
    auto n = Node::PNode(new Node("block", d, ParseCompoundStatement()));
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseDeclarationPart() {
    auto n = Node::PNode(new Node("decl"));
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

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseTypedConst(Node::PNode type) {
    //TODO rewrite???
    auto a = type->ToString() == "type" ? type->Children[1] : type;
    if (a->ToString() == "integer" ||
        a->ToString() == "real" ||
        a->ToString() == "char")
        return ParseExpression();
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::OpenParenthesis);
    if (a->ToString() == "array") {
        Node::PNode n = Node::PNode(new Node("array"));
        //TODO calculate possible expression
        int end = std::dynamic_pointer_cast<IntNode>(a->Children[1])->GetValue();
        for (int i = std::dynamic_pointer_cast<IntNode>(a->Children[0])->GetValue();
            i <= end; ++i) {
            tokenizer.Next();
            n->Add(ParseTypedConst(a->Children[2]));
            if (i == end)
                require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseParenthesis);
            else
                require(tokenizer.Current(), Tokenizer::Token::SubTypes::Comma);
        }
        tokenizer.Next();
        return n;
    }
    Node::PNode n = Node::PNode(new Node("record"));
    auto t = tokenizer.Next();
    for (int i = 0; i < a->Children.size(); ++i) {
        auto it = a->Children[i];
        require(t, Tokenizer::Token::SubTypes::Identifier);
        require(tokenizer.Next(), Tokenizer::Token::SubTypes::Colon);
        if (it->Children[0]->ToString() != t->GetValueString())
            throw std::exception(); //illegal order
        tokenizer.Next();
        n->Add(ParseTypedConst(it->Children[1]));
        if (i + 1 == a->Children.size() && tokenizer.Current()->GetSubType() != Tokenizer::Token::SubTypes::Semicolon)
            break;
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
        t = tokenizer.Next();
    }
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseParenthesis);
    tokenizer.Next();
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseActualParameterList() {
    auto n = Node::PNode(new Node("p_list"));
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
    auto n = Node::PNode(new Node("procedure", Node::PNode(new Node(t->GetValueString()))));
    declarations.push_back(Declaration());
    if (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::OpenParenthesis) {
        tokenizer.Next();
        n->Add(ParseFormalParameterList());
    }
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
    tokenizer.Next();
    n->Add(ParseBlock());
    declarations.pop_back();
    declarations.back().Procedures[n->Children[0]->ToString()] = n;
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseFunction() {
    auto t = tokenizer.Current();
    require(t, Tokenizer::Token::SubTypes::Identifier);
    requireFirstDecl(t->GetValueString());
    auto n = Node::PNode(new Node("function", Node::PNode(new Node(t->GetValueString()))));
    declarations.push_back(Declaration());
    if (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::OpenParenthesis) {
        tokenizer.Next();
        n->Add(ParseFormalParameterList());
    }
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
    require(tokenizer.Next(), Tokenizer::Token::SubTypes::Identifier);
    n->Add(findTypeDeclaration(tokenizer.Current()->GetValueString()));
    require(tokenizer.Next(), Tokenizer::Token::SubTypes::Semicolon);
    tokenizer.Next();
    n->Add(ParseBlock());
    declarations.pop_back();
    declarations.back().Functions[n->Children[0]->ToString()] = n;
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseFormalParameterList() {
    auto n = Node::PNode(new Node("p_list"));
    auto t = tokenizer.Current();
    auto set = std::unordered_set<std::string>();
    bool end = false;
    while (t->GetSubType() != Tokenizer::Token::SubTypes::CloseParenthesis) {
        Node::PNode c;
        bool last = false;
        if (t->GetSubType() == Tokenizer::Token::SubTypes::Var) {
            c = Node::PNode(new Node("var"));
            tokenizer.Next();
            ParseIdentifierList(c, set);
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
            require(tokenizer.Next(), Tokenizer::Token::SubTypes::Identifier);
            c->Add(findTypeDeclaration(tokenizer.Current()->GetValueString()));
            for (int i = 0; i < c->Children.size() - 1; ++i)
                declarations.back().Vars[c->Children[i]->ToString()] = c->Children.back();
            tokenizer.Next();
        }
        else {
            if (t->GetSubType() == Tokenizer::Token::SubTypes::Const) {
                c = Node::PNode(new Node("const"));
                tokenizer.Next();
            }
            else {
                require(t, Tokenizer::Token::SubTypes::Identifier);
                c = Node::PNode(new Node("value"));
            }
            ParseIdentifierList(c, set);
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
            require(tokenizer.Next(), Tokenizer::Token::SubTypes::Identifier);
            c->Add(findTypeDeclaration(tokenizer.Current()->GetValueString()));
            for (int i = 0; i < c->Children.size() - 1; ++i)
                declarations.back().Vars[c->Children[i]->ToString()] = c->Children.back();
            if (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::Equal) {
                if (c->Children.size() > 2)
                    throw std::exception();//only 1 variable can be initialized
                //TODO check type
                tokenizer.Next();
                //TODO check constexpr 
                c->Add(ParseExpression());
                end = true;
                last = true;
            }
        }
        if (end && !last)
            throw std::exception(); //default parameter should be last
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
    auto n = Node::PNode(new Node("statements"));
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
        auto type = Node::PNode(new Node("type", Node::PNode(new Node(t->GetStringValue())), ParseType()));
        p->Add(type);
        declarations.back().Types[t->GetStringValue()] = type;
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
        tokenizer.Next();
    }
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseType() {
    auto t = tokenizer.Current();
    switch (t->GetSubType()) {
    case Tokenizer::Token::SubTypes::Identifier:
        tokenizer.Next();
        return findTypeDeclaration(t->GetStringValue());
    case Tokenizer::Token::SubTypes::Array:
    {
        require(tokenizer.Next(), Tokenizer::Token::SubTypes::OpenBracket);
        auto n = Node::PNode(new Node("array"));
        tokenizer.Next();
        n->Add(ParseExpression());
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Range);
        tokenizer.Next();
        n->Add(ParseExpression());
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::CloseBracket);
        require(tokenizer.Next(), Tokenizer::Token::SubTypes::Of);
        tokenizer.Next();
        n->Add(ParseType());
        return n;
    }
    case Tokenizer::Token::SubTypes::Record:
    {
        Tokenizer::PToken t;
        auto n = Node::PNode(new Node("record"));
        //TODO identifier_list
        while ((t = tokenizer.Next())->GetSubType() == Tokenizer::Token::SubTypes::Identifier) {
            require(tokenizer.Next(), Tokenizer::Token::SubTypes::Colon);
            for (auto it : n->Children)
                if (it->Children.front()->ToString() == t->GetValueString())
                    throw std::exception();// duplicate identifier
            tokenizer.Next();
            n->Add(Node::PNode(new Node("field", Node::PNode(new Node(t->GetValueString())), ParseType())));
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
        }
        require(t, Tokenizer::Token::SubTypes::End);
        tokenizer.Next();
        return n;
    }
    default:
        throw SyntaxErrorException(t, Tokenizer::Token::SubTypes::Identifier);
    }
    return nullptr;
}

void My::SyntaxAnalyzer::ParseVarDeclaration(Node::PNode p) {
    auto t = tokenizer.Current();
    while (t->GetSubType() == Tokenizer::Token::SubTypes::Identifier) {
        auto n = Node::PNode(new Node("var"));
        //TODO make function
        requireFirstDecl(t->GetValueString());
        declarations.back().Vars[t->GetValueString()] = nullptr;
        n->Add(Node::PNode(new Node(t->GetValueString())));
        while (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::Comma) {
            require(t = tokenizer.Next(), Tokenizer::Token::SubTypes::Identifier);
            requireFirstDecl(t->GetValueString());
            declarations.back().Vars[t->GetValueString()] = nullptr;
            n->Add(Node::PNode(new Node(t->GetValueString())));
        }
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Colon);
        tokenizer.Next();
        n->Add(ParseType());
        if (tokenizer.Current()->GetSubType() == Tokenizer::Token::SubTypes::Equal) {
            if (n->Children.size() > 2)
                throw std::exception();//only 1 variable can be iitialized
            tokenizer.Next();
            n->Add(ParseTypedConst(n->Children.back()));
            for (int i = 0; i < n->Children.size() - 2; ++i)
                declarations.back().Vars[n->Children[i]->ToString()] = n->Children.back();
        }
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
        p->Add(n);
        t = tokenizer.Next();
    }
}

void My::SyntaxAnalyzer::ParseConstDeclaration(Node::PNode p) {
    auto t = tokenizer.Current();
    while (t->GetSubType() == Tokenizer::Token::SubTypes::Identifier) {
        auto n = Node::PNode(new Node("const"));
        requireFirstDecl(t->GetValueString());
        n->Add(Node::PNode(new Node(t->GetValueString())));
        if (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::Colon) {
            tokenizer.Next();
            n->Add(ParseType());
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Equal);
            tokenizer.Next();
            n->Add(ParseTypedConst(n->Children.back()));
        }
        else {
            require(tokenizer.Current(), Tokenizer::Token::SubTypes::Equal);
            tokenizer.Next();
            n->Add(ParseExpression());
        }
        declarations.back().Consts[t->GetValueString()] = n->Children.back();
        require(tokenizer.Current(), Tokenizer::Token::SubTypes::Semicolon);
        p->Add(n);
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
    case Tokenizer::Token::SubTypes::Identifier:
    {
        requireDeclaration(t->GetStringValue());
        auto n = Node::PNode(new Node(t->GetValueString()));
        t = tokenizer.Next();
        if (t->GetSubType() == Tokenizer::Token::SubTypes::Assign ||
            t->GetSubType() == Tokenizer::Token::SubTypes::PlusAssign ||
            t->GetSubType() == Tokenizer::Token::SubTypes::MinusAssign ||
            t->GetSubType() == Tokenizer::Token::SubTypes::MultAssign ||
            t->GetSubType() == Tokenizer::Token::SubTypes::DivideAssign) {
            tokenizer.Next();
            return Node::PNode(new Operation(t, n, ParseExpression()));
        }
        n = Node::PNode(new Node("call", n));
        if (t->GetSubType() == Tokenizer::Token::SubTypes::OpenParenthesis) {
            tokenizer.Next();
            n->Add(ParseActualParameterList());
        }
        return n;
    }
    default:
        break;
    }
    return nullptr;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseIfStatement() {
    auto n = Node::PNode(new Node("if"));
    tokenizer.Next();
    n->Add(ParseExpression());
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Then);
    tokenizer.Next();
    n->Add(ParseStatement());
    if (tokenizer.Current()->GetSubType() == Tokenizer::Token::SubTypes::Else)
        n->Add(ParseStatement());
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseWhileStatement() {
    auto n = Node::PNode(new Node("while"));
    tokenizer.Next();
    n->Add(ParseExpression());
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Do);
    tokenizer.Next();
    n->Add(ParseStatement());
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseForStatement() {
    auto n = Node::PNode(new Node("for"));
    auto t = tokenizer.Next();
    require(t, Tokenizer::Token::SubTypes::Identifier);
    findVarDeclaration(t->GetValueString());
    n->Add(Node::PNode(new Node(t->GetValueString())));
    require(tokenizer.Next(), Tokenizer::Token::SubTypes::Assign);
    tokenizer.Next();
    n->Add(ParseExpression());
    t = tokenizer.Current();
    if (t->GetSubType() != Tokenizer::Token::SubTypes::Downto)
        require(t, Tokenizer::Token::SubTypes::To);
    n->Add(Node::PNode(new Node(t->GetValueString())));
    tokenizer.Next();
    n->Add(ParseExpression());
    require(tokenizer.Current(), Tokenizer::Token::SubTypes::Do);
    tokenizer.Next();
    n->Add(ParseStatement());
    return n;
}

My::SyntaxAnalyzer::Node::PNode My::SyntaxAnalyzer::ParseRepeatStatement() {
    auto n = Node::PNode(new Node("repeat"));
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
    return message;
}
