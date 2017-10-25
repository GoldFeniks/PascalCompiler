#pragma once
#include <memory>
#include <vector>
#include "tokenizer.hpp"
#include "boost/format.hpp"
#include <unordered_map>
#include <exception>

namespace My {

    class SyntaxAnalyzer {

    public:

        class SyntaxErrorException : std::exception {

        public:

            SyntaxErrorException(const std::string message = "") : message(message) {};

            virtual const char* what() const override;

        protected:

            std::string message;

        };// class SyntaxErrorException

        class ExpectedException : public SyntaxErrorException {

        public:

            ExpectedException(const My::Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type);

        };// class ExpectedException

        class DudlicateIdentifierException : public SyntaxErrorException {

        public:

            DudlicateIdentifierException(const My::Tokenizer::PToken token);

        };// class DublicateIdentifierException

        class IllegalInitializationOrderException : public SyntaxErrorException {

        public:

            IllegalInitializationOrderException(Tokenizer::PToken token);

        };// class IllegalInitializationOrderException

        class InitializationOverloadException : public SyntaxErrorException {

        public:

            InitializationOverloadException(Tokenizer::PToken token);

        };// class InitializationOverloadException

        class DeclarationNotFoundException : public SyntaxErrorException {

        public:

            DeclarationNotFoundException(Tokenizer::PToken token);

        };// class DeclarationNotFoundException

        class Node {

        public:

            enum class Type {
                Variable,
                Type,
                Const,
                TypedConst,
                Operation,
                Procedure,
                Function,
                Block
            };

            static const std::string TypeNames[];

            typedef std::shared_ptr<Node> PNode;

            template<typename... C>
            Node(const std::string name, Type type, Tokenizer::PToken token, C... children) : name(name), MyType(type), Token(token) {
                push_back(children...);
            };

            Node(const std::string message, Type type, Tokenizer::PToken token) : name(message), MyType(type), Token(token) {};

            template<typename... C>
            void push_back(PNode node, C... children) {
                push_back(node);
                push_back(children...);
            }

            virtual void push_back(PNode node) {
                if (node == nullptr)
                    return;
                Children.push_back(node);
            }

            const std::string& ToString();

            void ChangeName(const std::string& name) {
                this->name = name;
            }

            std::vector<PNode> Children;
            Tokenizer::PToken Token;
            Type MyType;

        private:

            std::string name;

        };//class Node

        class TypeNode : public Node {

        public:

            typedef std::shared_ptr<TypeNode> PTypeNode;

            enum class TypeIdentifier { Scalar, Array, Record, Alias };

            TypeIdentifier MyTypeIdentifier; 

            static const std::string TypeIdentifierNames[];

        protected:

            template<typename... C>
            TypeNode(std::string name, TypeIdentifier identifier, Tokenizer::PToken token, C... children) : Node(name, Type::Type, token, children...), MyTypeIdentifier(identifier) {};

        };//class TypeNode

        class TypeAliasNode : public TypeNode {

        public:

            TypeAliasNode(std::string name, Node::PNode node, Tokenizer::PToken token) : TypeNode(name, TypeIdentifier::Alias, token, node) {};

        };

        class ScalarNode : public TypeNode {

        public:

            enum class ScalarType { Integer, Real, Char };

            ScalarNode(std::string name, ScalarType scalar_type, Tokenizer::PToken token, Node::PNode node = nullptr) : 
                TypeNode(name, TypeIdentifier::Scalar, token, node), MyScalarType(scalar_type) {};

            ScalarType MyScalarType;

        };//class ScalarNode

        class ArrayNode : public TypeNode {

        public:

            ArrayNode(std::string name, PNode from, PNode to, PTypeNode node, Tokenizer::PToken token) :
                TypeNode(name, TypeIdentifier::Array, token, from, to, node), type(node) {};

            PTypeNode type;

        };//class ArrayNode

        class RecordNode : public TypeNode {

        public:

            template<typename... C>
            RecordNode(std::string name, Tokenizer::PToken token, C... children) : TypeNode(name, TypeIdentifier::Record, token, children...) {};

        };//class RecordNode

        class ConstantNode : public Node {

        public:

            typedef std::shared_ptr<ConstantNode> PConstantNode;

            ConstantNode(std::string name, Tokenizer::Token::Value value, ScalarNode::ScalarType scalar_type, Tokenizer::PToken token) : Node(name, Type::Const, token),
                value(value), ScalarType(scalar_type) {};

            ScalarNode::ScalarType ScalarType;

            template<typename T>
            const T GetValue() {
                switch (ScalarType) {
                case ScalarNode::ScalarType::Integer:
                    return T(long long(value));
                case ScalarNode::ScalarType::Real:
                    return T(double(value));
                case ScalarNode::ScalarType::Char:
                    return T(*((char*)(value)));
                default:
                    break;
                }
            }

        private:

            Tokenizer::Token::Value value;

        };//class ConstantNode

        class VariableNode : public Node {

        public:

            enum class VariableType { Value, Const, Var };

            VariableNode(std::string name, TypeNode::PTypeNode type, VariableType v_type, Tokenizer::PToken token, Node::PNode value = nullptr) :
                Node(name, Type::Variable, token, type), vType(v_type), type(type) {
                SetValue(value);
            };

            TypeNode::PTypeNode type;
            VariableType vType;
            Node::PNode Value;

            void SetValue(Node::PNode value) {
                Value = value;
                push_back(value);
            }

            static const std::string VariableTypeNames[];

        };//class VariableNode

        class StringNode : public Node {

        public:

            StringNode(const Tokenizer::PToken t) : Node(t->GetString(), Type::Const, t) {};

        };//class StringNode

        class TypedConstantNode : public Node {

        public:

            template<typename... C>
            TypedConstantNode(Tokenizer::PToken token, TypeNode::PTypeNode type, C... children) : Node("typed_const", Type::TypedConst, token, children...), type(type) {};

            TypeNode::PTypeNode type;

        };

        class OperationNode : public Node {

        public:

            template<typename... C>
            OperationNode(std::string name, TypeNode::PTypeNode return_type, Tokenizer::PToken token, C... children) : 
                Node(name, Type::Operation, token, children..., return_type), ReturnType(return_type) {};

            TypeNode::PTypeNode ReturnType;

        };//class OperationNode

        class ProcedureNode : public Node {

        public:

            ProcedureNode(std::string name, PNode args, PNode block, Tokenizer::PToken token) : Node(name, Type::Procedure, token, args, block) {};

        };//class ProcedureNode

        class FunctionNode : public Node {

        public:

            FunctionNode(std::string name, PNode args, TypeNode::PTypeNode return_type, PNode block, Tokenizer::PToken token) :
                Node(name, Type::Function, token, args, return_type, block), ReturnType(return_type), Args(args) {};

            TypeNode::PTypeNode ReturnType;
            PNode Args;


        };//class FunctionNode

        class IncompatibleTypesException : public SyntaxErrorException {

        public:

            IncompatibleTypesException(const TypeNode::PTypeNode left, const TypeNode::PTypeNode right, const My::Tokenizer::PToken token);

        };//class IncompatibleTypesException

        class FunctionParameterException : public SyntaxErrorException {

        public:

            FunctionParameterException(const Tokenizer::PToken token, int c);

        };// class FunctionParameterException

        class IllegalExpressionException : public SyntaxErrorException {

        public:

            IllegalExpressionException(const Tokenizer::PToken token);

        };// class IllegalExpressionException

        class IllegalTypeException : public SyntaxErrorException {

        public:

            IllegalTypeException(const Tokenizer::PToken token);

        };

        struct Declaration {

            std::unordered_map<std::string, Node::PNode> Symbols;

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
        TypeNode::PTypeNode ParseType(std::string);
        void ParseVarDeclaration(Node::PNode p);
        void ParseConstDeclaration(Node::PNode p);
        Node::PNode ParseStatement();
        Node::PNode ParseIfStatement();
        Node::PNode ParseWhileStatement();
        Node::PNode ParseForStatement();
        Node::PNode ParseRepeatStatement();
        Node::PNode ParseDeclarationPart();
        void ParseStatementList(Node::PNode p);
        Node::PNode ParseTypedConst(TypeNode::PTypeNode type);
        Node::PNode ParseActualParameterList();
        void ParseExprressionList(Node::PNode p);
        Node::PNode ParseProcedure();
        Node::PNode ParseFunction();
        Node::PNode ParseFormalParameterList();
        Node::PNode ParseField(Node::PNode var, TypeNode::PTypeNode type);
        Node::PNode ParseFunctionCall(Node::PNode n);
        Node::PNode ParseProcedureCall(Node::PNode n);
        Node::PNode ParseArrayIndex(Node::PNode n, TypeNode::PTypeNode type);

        template<typename C, typename T, typename... Args>
        void ParseIdentifierList(C& p, T& set, Args... args) {
            auto t = tokenizer.Current();
            require(t, Tokenizer::Token::SubTypes::Identifier);
            requireUnique(t, set);
            set.emplace(t->GetValueString(), args...);
            p.push_back(t->GetValueString());
            while (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::Comma) {
                t = tokenizer.Next();
                require(t, Tokenizer::Token::SubTypes::Identifier);
                requireUnique(t, set);
                set.emplace(t->GetValueString(), args...);
                p.push_back(t->GetValueString());
            }
        };

        void Parse();
        std::string ToString();

    private:

        Tokenizer tokenizer;
        Node::PNode root = nullptr;
        std::vector<Declaration> declarations;

        static const TypeNode::PTypeNode integerNode, realNode, charNode;
        
        static ScalarNode::ScalarType getScalarType(Node::PNode n);
        static TypeNode::PTypeNode getConstantType(ConstantNode::PConstantNode n);
        static TypeNode::PTypeNode deduceType(Node::PNode left, Node::PNode right, Tokenizer::PToken token, bool allow_left_int = true);
        static TypeNode::PTypeNode getType(Node::PNode n);

        typedef std::unordered_map<std::string, std::function<ConstantNode::PConstantNode(ConstantNode::PConstantNode, ConstantNode::PConstantNode)>> binaryOpMap_t;
        typedef std::unordered_map<std::string, std::function<ConstantNode::PConstantNode(ConstantNode::PConstantNode)>> unaryOpMap_t;
        
        static const binaryOpMap_t binaryOpMap;
        static const unaryOpMap_t unaryOpMap;

        template<template<typename T> typename F>
        static ConstantNode::PConstantNode calc(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right, bool real_only = false) {
            if (left->ScalarType == ScalarNode::ScalarType::Integer && right->ScalarType == ScalarNode::ScalarType::Integer && !real_only) {
                F<long long> f;
                long long result = f(left->GetValue<long long>(), right->GetValue<long long>());
                return ConstantNode::PConstantNode(new ConstantNode(std::to_string(result), Tokenizer::Token::Value(result), ScalarNode::ScalarType::Integer, nullptr));
            }
            F<double> f;
            double result = f(left->GetValue<double>(), right->GetValue<double>());
            return ConstantNode::PConstantNode(new ConstantNode(std::to_string(result), Tokenizer::Token::Value(result), ScalarNode::ScalarType::Real, nullptr));
        }

        template<typename T>
        void requireUnique(const Tokenizer::PToken t, T& set) {
            if (set.count(t->GetValueString()) > 0)
                throw DudlicateIdentifierException(t);
        }

        ConstantNode::PConstantNode calculateConstExpr(Node::PNode n);
        static void requireTypesCompatibility(TypeNode::PTypeNode left, TypeNode::PTypeNode right, Tokenizer::PToken token, bool assign, bool allow_left_int = false);
        static void requireTypesCompatibility(TypeNode::PTypeNode left, ConstantNode::PConstantNode right, Tokenizer::PToken token);
        static void requireTypesCompatibility(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right, Tokenizer::PToken token);
        static void requireInteger(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right);
        void requireArgsCompatibility(Node::PNode f, Node::PNode n);
        static void requireType(TypeNode::PTypeNode type, TypeNode::TypeIdentifier id, Tokenizer::PToken token);
        static TypeNode::PTypeNode getBaseType(TypeNode::PTypeNode type);

        Node::PNode findDeclaration(const My::Tokenizer::PToken t);
        void requireNodeType(Node::PNode n, Node::Type type);
        void requireDeclaration(const My::Tokenizer::PToken t);
        static void require(const Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type);

        static std::string walk(const Node::PNode node, std::string prefix, bool last);

        inline static bool simpleExpressionOperators(const Tokenizer::Token::SubTypes type) {
            return type == Tokenizer::Token::SubTypes::Plus ||
                type == Tokenizer::Token::SubTypes::Minus;
        }

        inline static bool termOperators(const Tokenizer::Token::SubTypes type) {
            return type == Tokenizer::Token::SubTypes::Divide ||
                type == Tokenizer::Token::SubTypes::Mult ||
                type == Tokenizer::Token::SubTypes::Mod ||
                type == Tokenizer::Token::SubTypes::Div ||
                type == Tokenizer::Token::SubTypes::ShiftLeft ||
                type == Tokenizer::Token::SubTypes::ShiftRight;
        }
        
        inline static bool expressionOperators(const Tokenizer::Token::SubTypes type) {
            return type == Tokenizer::Token::SubTypes::Less ||
                type == Tokenizer::Token::SubTypes::LessEqual ||
                type == Tokenizer::Token::SubTypes::Equal ||
                type == Tokenizer::Token::SubTypes::Greater ||
                type == Tokenizer::Token::SubTypes::GreaterEqual ||
                type == Tokenizer::Token::SubTypes::NotEqual ||
                type == Tokenizer::Token::SubTypes::And ||
                type == Tokenizer::Token::SubTypes::Or;
        }

        inline static bool isRelational(const Tokenizer::Token::SubTypes type) {
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

                auto type = token->GetSubType() != Tokenizer::Token::SubTypes::Divide ?
                    deduceType(e, t, token, isRelational(token->GetSubType())) : realNode;
                e = Node::PNode(new NNode(token->GetStringValue(), type, token, e, t));
                //else
                    //e->push_back(t);
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