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

            SyntaxErrorException(const std::string& message) : message(message) {};

            virtual const char* what() const override;

        private:

            std::string message;

        };

        class ExpectedException : public SyntaxErrorException {

        public:

            ExpectedException(const My::Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type) : SyntaxErrorException(getMessage(token, type)) {};

        private:

            std::string getMessage(const My::Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type);

        };

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
                Add(children...);
            };

            Node(const std::string message, Type type) : name(message), MyType(type) {};

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
                Char
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

        class VariableNode : public Node {

        public:

            VariableNode(std::string name, TypeNode::PTypeNode type, bool is_const, PNode value = nullptr) : Node(name, Type::Variable, type, value), IsConst(is_const), type(type) {};

            TypeNode::PTypeNode type;

            bool IsConst;

        };//class VariableNode

        enum class ConstantType {
            Integer, 
            Real,
            String,
            Char
        };

        template <typename T, ConstantType NConstantType>
        class ConstantNode : public Node {

        public:

            ConstantNode(const Tokenizer::PToken token) : Node(token->GetString(), Type::Const), Value(token->GetValue<T>()) {};

            ConstantType MyConstantType = NConstantType;
            T Value;

        };//class ConstantNode

        using IntNode = ConstantNode<unsigned long long, ConstantType::Integer>;
        using FloatNode = ConstantNode<double, ConstantType::Real>;
        using StringNode = ConstantNode<char*, ConstantType::String>;
        using CharNode = ConstantNode<char, ConstantType::Char>;
        //ConstantNode specifications

        class TypedConstantNode : public Node {

        public:

            template<typename... C>
            TypedConstantNode(PNode node, C... children) : Node("typed_const", Type::Const, node, children...) {};

        };

        class OperationNode : public Node {

        public:

            template<typename... C>
            OperationNode(const My::Tokenizer::PToken token, C... children) : 
                Node(token->GetStringValue(), Type::Operation, children...) {};

        };//class BinaryOperation

        class CallNode : public Node {

        public:

            CallNode(std::string name, PNode args) : Node(name, Type::Call, args) {};

        };//class CallNode

        class IndexNode : public Node {

        public:

            IndexNode(std::string name, PNode args) : Node(name, Type::Index, args) {};

        };//class IndexNode

        class ProcedureNode : public Node {

        public:

            ProcedureNode(std::string name, PNode args, PNode block) : Node(name, Type::Procedure, args, block) {};

        };//class ProcedureNode

        class FunctionNode : public Node {

        public:

            FunctionNode(std::string name, PNode args, PNode return_type, PNode block) : Node(name, Type::Function, args, return_type, block) {};

        };//class FunctionNode

        struct Declaration {

            std::unordered_map<std::string, Node::PNode> Types, Vars, Procedures;

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
        Node::PNode ParseField(Node::PNode var);

        template<typename T, typename... Args>
        void ParseIdentifierList(Node::PNode p, T& set, Args... args) {
            auto t = tokenizer.Current();
            require(t, Tokenizer::Token::SubTypes::Identifier);
            requireUnique(t->GetValueString(), set);
            set.emplace(t->GetValueString(), args...);
            p->Add(Node::PNode(new Node(t->GetValueString(), Node::Type::Block)));
            while (tokenizer.Next()->GetSubType() == Tokenizer::Token::SubTypes::Comma) {
                t = tokenizer.Next();
                require(t, Tokenizer::Token::SubTypes::Identifier);
                requireUnique(t->GetValueString(), set);
                set.emplace(t->GetValueString(), args...);
                p->Add(Node::PNode(new Node(t->GetValueString(), Node::Type::Block)));
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
                //Add more info
                throw SyntaxErrorException("Dublicate identifier");
        }

        TypeNode::PTypeNode findTypeDeclaration(const std::string& name);
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