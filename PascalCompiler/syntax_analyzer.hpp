#pragma once
#include <memory>
#include <vector>
#include "tokenizer.hpp"
#include "boost/format.hpp"

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

            virtual bool Add(PNode node) {
                Children.push_back(node);
                return true;
            }

            const std::string& ToString();

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

        class FinalExpressionNode : public ExpressionNode {

        public:

            template<typename... C>
            FinalExpressionNode(const Tokenizer::PToken token, C... children) :
                ExpressionNode(token, children...) {};

            virtual bool Add(PNode) override { return false; }

        };//class FinalExpressionNode

        class VariableNode : public FinalExpressionNode {

        public:

            VariableNode(const Tokenizer::PToken token) : FinalExpressionNode(token) {};

        private:

            std::string name;

        };//class VariableNode

        template <typename T>
        class ConstantNode : public FinalExpressionNode {

        public:

            ConstantNode(const Tokenizer::PToken token) : FinalExpressionNode(token) {};

            const T GetValue() {
                return Token->GetValue<T>();
            }

        };//class ConstantNode

        using IntNode = ConstantNode<unsigned long long>;
        using FloatNode = ConstantNode<double>;
        using StringNode = ConstantNode<char*>;
        using CharNode = ConstantNode<char>;
        //ConstantNode specifications

        class UnaryOperation : public FinalExpressionNode {

        public:

            UnaryOperation(const My::Tokenizer::PToken token, PNode operand) :
                FinalExpressionNode(token, operand) {};

            virtual bool Add(PNode node) override { return false; }

        };//class UnaryOperation

        class Operation : public ExpressionNode {

        public:

            Operation(const My::Tokenizer::PToken token, PNode left, PNode right) : 
                ExpressionNode(token, left, right) {};

        };//class BinaryOperation

        SyntaxAnalyzer(Tokenizer&& tokenizer) : tokenizer(std::move(tokenizer)) {};
        SyntaxAnalyzer(const std::string file) : tokenizer(Tokenizer(file)) {};
        SyntaxAnalyzer(std::ifstream&& file) : tokenizer(std::move(file)) {};

        SyntaxAnalyzer(const SyntaxAnalyzer&) = delete;
        SyntaxAnalyzer(SyntaxAnalyzer&& other) : tokenizer(std::move(other.tokenizer)) { std::swap(root, other.root); };

        SyntaxAnalyzer& operator=(const SyntaxAnalyzer&) = delete;
        SyntaxAnalyzer& operator=(SyntaxAnalyzer&& other);

        ExpressionNode::PExpressionNode ParseExpression();
        ExpressionNode::PExpressionNode ParseTerm();
        ExpressionNode::PExpressionNode ParseFactor();
        void Parse();
        std::string ToString();

    private:

        Tokenizer tokenizer;
        Node::PNode root = nullptr;

        void require(const Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type);
        std::string walk(const Node::PNode node, std::string prefix, bool last);

        inline static bool expressionOperators(const Tokenizer::Token::SubTypes type) {
            return type == Tokenizer::Token::SubTypes::Plus ||
                type == Tokenizer::Token::SubTypes::Minus;
        }

        inline static bool termOperators(const Tokenizer::Token::SubTypes type) {
            return type == Tokenizer::Token::SubTypes::Divide ||
                type == Tokenizer::Token::SubTypes::Mult;
        }

        template<typename NNode, ExpressionNode::PExpressionNode(SyntaxAnalyzer::*NParse)(void), bool(*Cond)(const Tokenizer::Token::SubTypes)>
        ExpressionNode::PExpressionNode parse(ExpressionNode::PExpressionNode e) {
            auto token = tokenizer.Current();
            while (Cond(token->GetSubType())) {
                tokenizer.Next();
                auto t = (this->*NParse)();
                if (e->Token->GetSubType() != token->GetSubType() || !e->Add(t))
                    e = ExpressionNode::PExpressionNode(new NNode(token, e, t));
                token = tokenizer.Current();
            }
            return e;
        }

    };//class SyntaxAnalyzer

}// namespace My