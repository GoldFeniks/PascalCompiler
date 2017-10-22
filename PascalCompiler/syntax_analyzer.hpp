#pragma once
#include <memory>
#include <vector>
#include "tokenizer.hpp"
#include "boost/format.hpp"
#include <unordered_map>

namespace My {

    class SyntaxAnalyzer {

    public:

        class SyntaxErrorException : std::exception {

        public:

            SyntaxErrorException(const My::Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type);

            virtual const char* what() const override;

        private:

            std::string message;

        };

        class Node {

        public:

            typedef std::shared_ptr<Node> PNode;

            template<typename... C>
            Node(const std::string message, C... children) : message(message) {
                Add(children...);
            };

            Node(const std::string message) : message(message) {};

            template<typename... C>
            void Add(PNode node, C... children) {
                Add(node);
                Add(children...);
            }

            virtual void Add(PNode node) {
                if (node == nullptr)
                    return;
                Children.push_back(node);
            }

            const std::string& ToString();
            void ChangeName(const std::string& name) {
                message = name;
            }

            std::vector<PNode> Children;

        private:

            std::string message;

        };//class Node

        class ExpressionNode : public Node {

        public:

            typedef std::shared_ptr<ExpressionNode> PExpressionNode;

            template<typename... C>
            ExpressionNode(const Tokenizer::PToken token, C... children) : 
                Node(token->GetString(), children...), Token(token) {};

            Tokenizer::PToken Token;

        };//class ExpressionNode

        class VariableNode : public ExpressionNode {

        public:

            VariableNode(const Tokenizer::PToken token) : ExpressionNode(token) {};

        };//class VariableNode

        template <typename T>
        class ConstantNode : public ExpressionNode {

        public:

            ConstantNode(const Tokenizer::PToken token) : ExpressionNode(token) {};

            const T GetValue() {
                return Token->GetValue<T>();
            }

        };//class ConstantNode

        using IntNode = ConstantNode<unsigned long long>;
        using FloatNode = ConstantNode<double>;
        using StringNode = ConstantNode<char*>;
        using CharNode = ConstantNode<char>;
        //ConstantNode specifications

        class UnaryOperation : public ExpressionNode {

        public:

            UnaryOperation(const My::Tokenizer::PToken token, PNode operand) :
                ExpressionNode(token, operand) {};

        };//class UnaryOperation

        class Operation : public ExpressionNode {

        public:

            Operation(const My::Tokenizer::PToken token, PNode left, PNode right) : 
                ExpressionNode(token, left, right) {};

        };//class BinaryOperation

        struct Declaration {

            std::unordered_map<std::string, Node::PNode> Types, Vars, Consts, Procedures, Functions;

        };//struct Declaration

        SyntaxAnalyzer(Tokenizer&& tokenizer) : tokenizer(std::move(tokenizer)) {};
        SyntaxAnalyzer(const std::string file) : tokenizer(Tokenizer(file)) {};
        SyntaxAnalyzer(std::ifstream&& file) : tokenizer(std::move(file)) {};

        SyntaxAnalyzer(const SyntaxAnalyzer&) = delete;
        SyntaxAnalyzer(SyntaxAnalyzer&& other) : tokenizer(std::move(other.tokenizer)) { std::swap(root, other.root); };

        SyntaxAnalyzer& operator=(const SyntaxAnalyzer&) = delete;
        SyntaxAnalyzer& operator=(SyntaxAnalyzer&& other);

        Node::PNode ParseExpression();
        Node::PNode ParseSimpleExpression();
        Node::PNode ParseTerm();
        Node::PNode ParseFactor();
        Node::PNode ParseProgram();
        Node::PNode ParseBlock();
        Node::PNode ParseCompoundStatement();
        void ParseTypeDeclaration(Node::PNode p);
        Node::PNode ParseType();
        void ParseVarDeclaration(Node::PNode p);
        void ParseConstDeclaration(Node::PNode p);
        Node::PNode ParseStatement();
        Node::PNode ParseIfStatement();
        Node::PNode ParseWhileStatement();
        Node::PNode ParseForStatement();
        Node::PNode ParseRepeatStatement();
        Node::PNode ParseDeclarationPart();
        void ParseStatementList(Node::PNode p);
        Node::PNode ParseTypedConst(Node::PNode type);
        Node::PNode ParseActualParameterList();
        void ParseExprressionList(Node::PNode p);
        Node::PNode ParseProcedure();
        Node::PNode ParseFunction();
        Node::PNode ParseFormalParameterList();

        template<typename T, typename... Args>
        void ParseIdentifierList(Node::PNode p, T& set, Args... args) {
            auto t = tokenizer.Current();
            require(t, Tokenizer::Token::SubTypes::Identifier);
            requireUnique(t->GetValueString(), set);
            set.emplace(t->GetValueString(), args...);
            p->Add(Node::PNode(new Node(t->GetValueString())));
            while (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::Comma) {
                t = tokenizer.Next();
                require(t, Tokenizer::Token::SubTypes::Identifier);
                requireUnique(t->GetValueString(), set);
                set.emplace(t->GetValueString(), args...);
                p->Add(Node::PNode(new Node(t->GetValueString())));
            }
        };

        void Parse();
        std::string ToString();

    private:

        Tokenizer tokenizer;
        Node::PNode root = nullptr;
        std::vector<Declaration> declarations;

        template<typename T>
        void requireUnique(const std::string& value, T& set) {
            if (set.count(value) > 0)
                throw std::exception(); // dublicate
        }

        Node::PNode findTypeDeclaration(const std::string& name);
        Node::PNode findVarDeclaration(const std::string& name);
        Node::PNode findProcedureDeclaration(const std::string& name);
        void requireFirstDecl(const std::string& name);
        void requireDeclaration(const std::string& name);

        void require(const Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type);
        std::string walk(const Node::PNode node, std::string prefix, bool last);

        inline static bool simpleExpressionOperators(const Tokenizer::Token::SubTypes type) {
            return type == Tokenizer::Token::SubTypes::Plus ||
                type == Tokenizer::Token::SubTypes::Minus;
        }

        inline static bool termOperators(const Tokenizer::Token::SubTypes type) {
            return type == Tokenizer::Token::SubTypes::Divide ||
                type == Tokenizer::Token::SubTypes::Mult ||
                type == Tokenizer::Token::SubTypes::Mod ||
                type == Tokenizer::Token::SubTypes::Div;
        }
        
        inline static bool expressionOperators(const Tokenizer::Token::SubTypes type) {
            return type == Tokenizer::Token::SubTypes::Less ||
                type == Tokenizer::Token::SubTypes::LessEqual ||
                type == Tokenizer::Token::SubTypes::Equal ||
                type == Tokenizer::Token::SubTypes::Greater ||
                type == Tokenizer::Token::SubTypes::GreaterEqual ||
                type == Tokenizer::Token::SubTypes::NotEqual;
        }

        template<typename NNode, Node::PNode(SyntaxAnalyzer::*NParse)(void), bool(*Cond)(const Tokenizer::Token::SubTypes)>
        Node::PNode parse(Node::PNode e) {
            auto token = tokenizer.Current();
            while (Cond(token->GetSubType())) {
                tokenizer.Next();
                auto t = (this->*NParse)();
                //if (e->Token->GetSubType() != token->GetSubType())
                e = Node::PNode(new NNode(token, e, t));
                //else
                    //e->Add(t);
                token = tokenizer.Current();
            }
            return e;
        }

        inline bool IsDeclaration(const Tokenizer::PToken token) {
            auto t = token->GetSubType();
            return t == Tokenizer::Token::SubTypes::Type ||
                t == Tokenizer::Token::SubTypes::Var ||
                t == Tokenizer::Token::SubTypes::Const;
        }

        inline bool CurrentTokenSubType(const Tokenizer::Token::SubTypes type) {
            return tokenizer.Current()->GetSubType() == type;
        }

        inline bool CurrentTokenType(const Tokenizer::Token::Types type) {
            return tokenizer.Current()->GetType() == type;
        }

    };//class SyntaxAnalyzer

}// namespace My