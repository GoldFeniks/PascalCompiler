#pragma once
#include <memory>
#include <vector>
#include "tokenizer.hpp"
#include "boost/format.hpp"
#include <unordered_map>
#include <exception>

namespace pascal_compiler {

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

            ExpectedException(const pascal_compiler::tokenizer::token_p token, pascal_compiler::tokenizer::token::sub_types type);

        };// class ExpectedException

        class DudlicateIdentifierException : public SyntaxErrorException {

        public:

            DudlicateIdentifierException(const pascal_compiler::tokenizer::token_p token);

        };// class DublicateIdentifierException

        class IllegalInitializationOrderException : public SyntaxErrorException {

        public:

            IllegalInitializationOrderException(tokenizer::token_p token);

        };// class IllegalInitializationOrderException

        class InitializationOverloadException : public SyntaxErrorException {

        public:

            InitializationOverloadException(tokenizer::token_p token);

        };// class InitializationOverloadException

        class DeclarationNotFoundException : public SyntaxErrorException {

        public:

            DeclarationNotFoundException(tokenizer::token_p token);

        };// class DeclarationNotFoundException

        class Node {

        public:

            enum class Type { Variable, Type, Const, TypedConst, Operation, Procedure, Function, Empty };

            static const std::string TypeNames[];

            typedef std::shared_ptr<Node> PNode;

            template<typename... C>
            Node(const std::string name, Type type, tokenizer::token_p token, C... children) : name(name), MyType(type), Token(token) {
                push_back(children...);
            };

            Node(const std::string message, Type type, tokenizer::token_p token) : name(message), MyType(type), Token(token) {};

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
            tokenizer::token_p Token;
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
            TypeNode(std::string name, TypeIdentifier identifier, tokenizer::token_p token, C... children) : Node(name, Type::Type, token, children...), MyTypeIdentifier(identifier) {};

        };//class TypeNode

        class TypeAliasNode : public TypeNode {

        public:

            TypeAliasNode(std::string name, Node::PNode node, tokenizer::token_p token) : TypeNode(name, TypeIdentifier::Alias, token, node) {};

        };

        class ScalarNode : public TypeNode {

        public:

            enum class ScalarType { Integer, Real, Char };

            ScalarNode(std::string name, ScalarType scalar_type, tokenizer::token_p token, Node::PNode node = nullptr) : 
                TypeNode(name, TypeIdentifier::Scalar, token, node), MyScalarType(scalar_type) {};

            ScalarType MyScalarType;

        };//class ScalarNode

        class ArrayNode : public TypeNode {

        public:

            ArrayNode(std::string name, PNode from, PNode to, PTypeNode node, tokenizer::token_p token) :
                TypeNode(name, TypeIdentifier::Array, token, from, to, node), type(node) {};

            PTypeNode type;

        };//class ArrayNode

        class RecordNode : public TypeNode {

        public:

            template<typename... C>
            RecordNode(std::string name, tokenizer::token_p token, C... children) : TypeNode(name, TypeIdentifier::Record, token, children...) {};

        };//class RecordNode

        class ConstantNode : public Node {

        public:

            typedef std::shared_ptr<ConstantNode> PConstantNode;

            ConstantNode(std::string name, tokenizer::token::value value, ScalarNode::ScalarType scalar_type, tokenizer::token_p token) : Node(name, Type::Const, token),
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

            tokenizer::token::value value;

        };//class ConstantNode

        class VariableNode : public Node {

        public:

            enum class VariableType { Value, Const, Var };

            VariableNode(std::string name, TypeNode::PTypeNode type, VariableType v_type, tokenizer::token_p token, Node::PNode value = nullptr) :
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

            StringNode(const tokenizer::token_p t) : Node(t->get_string(), Type::Const, t) {};

        };//class StringNode

        class TypedConstantNode : public Node {

        public:

            template<typename... C>
            TypedConstantNode(tokenizer::token_p token, TypeNode::PTypeNode type, C... children) : Node("typed_const", Type::TypedConst, token, children...), type(type) {};

            TypeNode::PTypeNode type;

        };

        class OperationNode : public Node {

        public:

            template<typename... C>
            OperationNode(std::string name, TypeNode::PTypeNode return_type, tokenizer::token_p token, C... children) : 
                Node(name, Type::Operation, token, children..., return_type), ReturnType(return_type) {};

            TypeNode::PTypeNode ReturnType;

        };//class OperationNode

        class ProcedureNode : public Node {

        public:

            ProcedureNode(std::string name, PNode args, PNode block, tokenizer::token_p token) : Node(name, Type::Procedure, token, args, block) {};

        };//class ProcedureNode

        class FunctionNode : public Node {

        public:

            FunctionNode(std::string name, PNode args, TypeNode::PTypeNode return_type, PNode block, tokenizer::token_p token) :
                Node(name, Type::Function, token, args, return_type, block), ReturnType(return_type), Args(args) {};

            TypeNode::PTypeNode ReturnType;
            PNode Args;


        };//class FunctionNode

        class IncompatibleTypesException : public SyntaxErrorException {

        public:

            IncompatibleTypesException(const TypeNode::PTypeNode left, const TypeNode::PTypeNode right, const pascal_compiler::tokenizer::token_p token);

        };//class IncompatibleTypesException

        class FunctionParameterException : public SyntaxErrorException {

        public:

            FunctionParameterException(const tokenizer::token_p token, int c);

        };// class FunctionParameterException

        class IllegalExpressionException : public SyntaxErrorException {

        public:

            IllegalExpressionException(const tokenizer::token_p token);

        };// class IllegalExpressionException

        class IllegalTypeException : public SyntaxErrorException {

        public:

            IllegalTypeException(const tokenizer::token_p token);

        };

        struct Declaration {

            std::unordered_map<std::string, Node::PNode> Symbols;

        };//struct Declaration

        SyntaxAnalyzer(tokenizer&& tokenizer) : tokenizer_(std::move(tokenizer)) {};
        SyntaxAnalyzer(const std::string file) : tokenizer_(tokenizer(file)) {};
        SyntaxAnalyzer(std::ifstream&& file) : tokenizer_(std::move(file)) {};

        SyntaxAnalyzer(const SyntaxAnalyzer&) = delete;
        SyntaxAnalyzer(SyntaxAnalyzer&& other) : tokenizer_(std::move(other.tokenizer_)) { std::swap(root, other.root); };

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
            auto t = tokenizer_.current();
            require(t, tokenizer::token::sub_types::identifier);
            requireUnique(t, set);
            set.emplace(t->get_value_string(), args...);
            p.push_back(t->get_value_string());
            while (tokenizer_.next()->get_sub_type() == tokenizer::token::sub_types::comma) {
                t = tokenizer_.next();
                require(t, tokenizer::token::sub_types::identifier);
                requireUnique(t, set);
                set.emplace(t->get_value_string(), args...);
                p.push_back(t->get_value_string());
            }
        };

        void Parse();
        std::string ToString();

    private:

        tokenizer tokenizer_;
        Node::PNode root = nullptr;
        std::vector<Declaration> declarations;

        static const TypeNode::PTypeNode integerNode, realNode, charNode;
        
        static ScalarNode::ScalarType getScalarType(Node::PNode n);
        static TypeNode::PTypeNode getConstantType(ConstantNode::PConstantNode n);
        static TypeNode::PTypeNode deduceType(Node::PNode left, Node::PNode right, tokenizer::token_p token, bool allow_left_int = true);
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
                return ConstantNode::PConstantNode(new ConstantNode(std::to_string(result), tokenizer::token::value(result), ScalarNode::ScalarType::Integer, nullptr));
            }
            F<double> f;
            double result = f(left->GetValue<double>(), right->GetValue<double>());
            return ConstantNode::PConstantNode(new ConstantNode(std::to_string(result), tokenizer::token::value(result), ScalarNode::ScalarType::Real, nullptr));
        }

        template<typename T>
        void requireUnique(const tokenizer::token_p t, T& set) {
            if (set.count(t->get_value_string()) > 0)
                throw DudlicateIdentifierException(t);
        }

        ConstantNode::PConstantNode calculateConstExpr(Node::PNode n);
        static void requireTypesCompatibility(TypeNode::PTypeNode left, TypeNode::PTypeNode right, tokenizer::token_p token, bool assign, bool allow_left_int = false);
        static void requireTypesCompatibility(TypeNode::PTypeNode left, ConstantNode::PConstantNode right, tokenizer::token_p token);
        static void requireTypesCompatibility(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right, tokenizer::token_p token);
        static void requireInteger(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right);
        void requireArgsCompatibility(Node::PNode f, Node::PNode n);
        static void requireType(TypeNode::PTypeNode type, TypeNode::TypeIdentifier id, tokenizer::token_p token);
        static TypeNode::PTypeNode getBaseType(TypeNode::PTypeNode type);

        Node::PNode findDeclaration(const pascal_compiler::tokenizer::token_p t);
        void requireNodeType(Node::PNode n, Node::Type type);
        void requireDeclaration(const pascal_compiler::tokenizer::token_p t);
        static void require(const tokenizer::token_p token, pascal_compiler::tokenizer::token::sub_types type);

        static std::string walk(const Node::PNode node, std::string prefix, bool last);

        inline static bool simpleExpressionOperators(const tokenizer::token::sub_types type) {
            return type == tokenizer::token::sub_types::plus ||
                type == tokenizer::token::sub_types::minus;
        }

        inline static bool termOperators(const tokenizer::token::sub_types type) {
            return type == tokenizer::token::sub_types::divide ||
                type == tokenizer::token::sub_types::mult ||
                type == tokenizer::token::sub_types::mod ||
                type == tokenizer::token::sub_types::div ||
                type == tokenizer::token::sub_types::shift_left ||
                type == tokenizer::token::sub_types::shift_right;
        }
        
        inline static bool expressionOperators(const tokenizer::token::sub_types type) {
            return type == tokenizer::token::sub_types::less ||
                type == tokenizer::token::sub_types::less_equal ||
                type == tokenizer::token::sub_types::equal ||
                type == tokenizer::token::sub_types::greater ||
                type == tokenizer::token::sub_types::greater_equal ||
                type == tokenizer::token::sub_types::not_equal ||
                type == tokenizer::token::sub_types::and ||
                type == tokenizer::token::sub_types::or;
        }

        inline static bool isRelational(const tokenizer::token::sub_types type) {
            return type == tokenizer::token::sub_types::less ||
                type == tokenizer::token::sub_types::less_equal ||
                type == tokenizer::token::sub_types::equal ||
                type == tokenizer::token::sub_types::greater ||
                type == tokenizer::token::sub_types::greater_equal ||
                type == tokenizer::token::sub_types::not_equal;
        }

        template<typename NNode, Node::PNode(SyntaxAnalyzer::*NParse)(void), bool(*Cond)(const tokenizer::token::sub_types)>
        Node::PNode parse(Node::PNode e) {
            auto token = tokenizer_.current();
            while (Cond(token->get_sub_type())) {
                tokenizer_.next();
                auto t = (this->*NParse)();
                //if (e->Token->GetSubType() != token->GetSubType())

                auto type = token->get_sub_type() != tokenizer::token::sub_types::divide ?
                    deduceType(e, t, token, isRelational(token->get_sub_type())) : realNode;
                e = Node::PNode(new NNode(token->get_string_value(), type, token, e, t));
                //else
                    //e->push_back(t);
                token = tokenizer_.current();
            }
            return e;
        }

        inline bool IsDeclaration(const tokenizer::token_p token) {
            auto t = token->get_sub_type();
            return t == tokenizer::token::sub_types::type ||
                t == tokenizer::token::sub_types::var ||
                t == tokenizer::token::sub_types::const_op;
        }

        inline bool CurrentTokenSubType(const tokenizer::token::sub_types type) {
            return tokenizer_.current()->get_sub_type() == type;
        }

        inline bool CurrentTokenType(const tokenizer::token::types type) {
            return tokenizer_.current()->get_type() == type;
        }

    };//class SyntaxAnalyzer

}// namespace pascal_compiler