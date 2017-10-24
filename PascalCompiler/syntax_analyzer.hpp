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

            SyntaxErrorException(const std::string& message) : message(message) {};

            virtual const char* what() const override;

        private:

            std::string message;

        };// class SyntaxErrorException

        class ExpectedException : public SyntaxErrorException {

        public:

            ExpectedException(const My::Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type) : SyntaxErrorException(getMessage(token, type)) {};

        private:

            std::string getMessage(const My::Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type);

        };// class ExpectedException

        class DudlicateIdentifierException : public SyntaxErrorException {

        public:

            DudlicateIdentifierException(const My::Tokenizer::PToken token) : SyntaxErrorException(getMessage(token)) {};

        private:

            std::string getMessage(const My::Tokenizer::PToken token);

        };// class DublicateIdentifierException

        class Node {

        public:

            enum class Type {
                Variable,
                Type,
                Const,
                Operation,
                Call,
                Index,
                Procedure,
                Function,
                Block
            };

            static const std::string TypeNames[];

            typedef std::shared_ptr<Node> PNode;

            template<typename... C>
            Node(const std::string name, Type type, C... children) : name(name), MyType(type) {
                push_back(children...);
            };

            Node(const std::string message, Type type) : name(message), MyType(type) {};

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
            Type MyType;

        private:

            std::string name;

        };//class Node

        class TypeNode : public Node {

        public:

            typedef std::shared_ptr<TypeNode> PTypeNode;

            enum class TypeIdentifier {
                Scalar,
                Array,
                Record,
                Alias
            };

            TypeIdentifier MyTypeIdentifier; 

            static const std::string TypeIdentifierNames[];

        protected:

            template<typename... C>
            TypeNode(std::string name, TypeIdentifier identifier, C... children) : Node(name, Type::Type, children...), MyTypeIdentifier(identifier) {};

        };//class TypeNode

        class TypeAliasNode : public TypeNode {

        public:

            TypeAliasNode(std::string name, Node::PNode node) : TypeNode(name, TypeIdentifier::Alias, node) {};

        };

        class ScalarNode : public TypeNode {

        public:

            enum class ScalarType {
                Integer,
                Real,
                Char,
            };

            ScalarNode(std::string name, ScalarType scalar_type, Node::PNode node = nullptr) : TypeNode(name, TypeIdentifier::Scalar, node), MyScalarType(scalar_type) {};

            ScalarType MyScalarType;

        };//class ScalarNode

        class ArrayNode : public TypeNode {

        public:

            ArrayNode(std::string name, PNode from, PNode to, PTypeNode node) : TypeNode(name, TypeIdentifier::Array, from, to, node), type(node) {};

            PTypeNode type;

        };//class ArrayNode

        class RecordNode : public TypeNode {

        public:

            template<typename... C>
            RecordNode(std::string name, C... children) : TypeNode(name, TypeIdentifier::Record, children...) {};

        };//class RecordNode

        class ConstantNode : public Node {

        public:

            typedef std::shared_ptr<ConstantNode> PConstantNode;

            ConstantNode(std::string name, Tokenizer::Token::Value value, ScalarNode::ScalarType scalar_type) : Node(name, Type::Const),
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

            VariableNode(std::string name, TypeNode::PTypeNode type, VariableType v_type, Node::PNode value = nullptr) :
                Node(name, Type::Variable, type), vType(v_type), type(type) {
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

            StringNode(const Tokenizer::PToken t) : Node(t->GetValueString(), Type::Const) {};

        };//class StringNode

        class TypedConstantNode : public Node {

        public:

            template<typename... C>
            TypedConstantNode(PNode node, C... children) : Node("typed_const", Type::Const, node, children...) {};

        };

        class OperationNode : public Node {

        public:

            template<typename... C>
            OperationNode(std::string name, TypeNode::PTypeNode return_type, C... children) : 
                Node(name, Type::Operation, children..., return_type), ReturnType(return_type) {};

            TypeNode::PTypeNode ReturnType;

        };//class OperationNode

        class CallNode : public Node {

        public:

            CallNode(std::string name, PNode obj, PNode args, TypeNode::PTypeNode return_type) : Node(name, Type::Call, obj, args, return_type), ReturnType(return_type) {};

            TypeNode::PTypeNode ReturnType;

        };//class CallNode

        class IndexNode : public Node {

        public:

            IndexNode(std::string name, PNode obj, PNode args) : Node(name, Type::Index, obj, args) {};

        };//class IndexNode

        class ProcedureNode : public Node {

        public:

            ProcedureNode(std::string name, PNode args, PNode block) : Node(name, Type::Procedure, args, block) {};

        };//class ProcedureNode

        class FunctionNode : public Node {

        public:

            FunctionNode(std::string name, PNode args, TypeNode::PTypeNode return_type, PNode block) : 
                Node(name, Type::Function, args, return_type, block), ReturnType(return_type), Args(args) {};

            TypeNode::PTypeNode ReturnType;
            PNode Args;


        };//class FunctionNode

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
        static TypeNode::PTypeNode deduceType(Node::PNode left, Node::PNode right, bool allow_left_int = true);
        static TypeNode::PTypeNode getType(Node::PNode n);

        typedef std::unordered_map<std::string, std::function<ConstantNode::PConstantNode(ConstantNode::PConstantNode, ConstantNode::PConstantNode)>> binaryOpMap_t;
        typedef std::unordered_map<std::string, std::function<ConstantNode::PConstantNode(ConstantNode::PConstantNode)>> unaryOpMap_t;
        
        static const binaryOpMap_t binaryOpMap;
        static const unaryOpMap_t unaryOpMap;

        template<typename F>
        static ConstantNode::PConstantNode calc(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right, bool real_only = false) {
            F f;
            if (left->ScalarType == ScalarNode::ScalarType::Integer && right->ScalarType == ScalarNode::ScalarType::Integer && !real_only) {
                long long result = std::floor(f(left->GetValue<double>(), right->GetValue<double>()));
                return ConstantNode::PConstantNode(new ConstantNode(std::to_string(result), Tokenizer::Token::Value(result), ScalarNode::ScalarType::Integer));
            }
            double result = f(left->GetValue<double>(), right->GetValue<double>());
            return ConstantNode::PConstantNode(new ConstantNode(std::to_string(result), Tokenizer::Token::Value(result), ScalarNode::ScalarType::Real));
        }

        template<typename T>
        void requireUnique(const Tokenizer::PToken t, T& set) {
            if (set.count(t->GetValueString()) > 0)
                throw DudlicateIdentifierException(t);
        }

        static ConstantNode::PConstantNode calculateConstExpr(Node::PNode n);
        static void requireTypesCompatibility(TypeNode::PTypeNode left, TypeNode::PTypeNode right, bool allow_left_int);
        static void requireTypesCompatibility(TypeNode::PTypeNode left, ConstantNode::PConstantNode right);
        static void requireTypesCompatibility(TypeNode::PTypeNode left, TypeNode::TypeIdentifier t);
        static void requireTypesCompatibility(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right);
        static void requireInteger(ConstantNode::PConstantNode left, ConstantNode::PConstantNode right);
        static void requireArgsCompatibility(Node::PNode f, Node::PNode n);
        static TypeNode::PTypeNode getBaseType(TypeNode::PTypeNode type);

        Node::PNode findDeclaration(const My::Tokenizer::PToken t);
        static void requireNodeType(Node::PNode n, Node::Type type);
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
                auto type = deduceType(e, t);
                e = Node::PNode(new NNode(token->GetStringValue(), type, e, t));
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