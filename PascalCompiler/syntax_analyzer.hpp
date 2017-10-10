#pragma once
#include <memory>
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

            Node(const Tokenizer::PToken token, PNode left = nullptr, PNode right = nullptr) : 
                Token(token), Left(left), Right(right) {};

            PNode Left, Right;
            const Tokenizer::PToken Token;

        };//class Node

        class ExpressionNode : public Node {

        public:

            ExpressionNode(const Tokenizer::PToken token, PNode left = nullptr, PNode right = nullptr) : 
                Node(token, left, right) {};

        };//class ExpressionNode

        class VariableNode : public ExpressionNode {

        public:

            VariableNode(const Tokenizer::PToken token) : ExpressionNode(token) {};

        private:

            std::string name;

        };//class VariableNode

        template <typename T>
        class ConstantNode : public ExpressionNode {

        public:

            ConstantNode(const Tokenizer::PToken token) : 
                ExpressionNode(token) {};

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

        class BinaryOperation : public ExpressionNode {

        public:

            BinaryOperation(const My::Tokenizer::PToken token, PNode left, PNode right) : 
                ExpressionNode(token, left, right) {};

        };//class BinaryOperation

        SyntaxAnalyzer(Tokenizer&& tokenizer) : tokenizer(std::move(tokenizer)) {};
        SyntaxAnalyzer(const std::string file) : tokenizer(Tokenizer(file)) {};
        SyntaxAnalyzer(std::ifstream&& file) : tokenizer(std::move(file)) {};

        SyntaxAnalyzer(const SyntaxAnalyzer&) = delete;
        SyntaxAnalyzer(SyntaxAnalyzer&& other) : tokenizer(std::move(other.tokenizer)) { std::swap(root, other.root); };

        SyntaxAnalyzer& operator=(const SyntaxAnalyzer&) = delete;
        SyntaxAnalyzer& operator=(SyntaxAnalyzer&& other);

        Node::PNode ParseExpression();
        Node::PNode ParseTerm();
        Node::PNode ParseFactor();
        void Parse();
        std::string ToString();

    private:

        Tokenizer tokenizer;
        Node::PNode root = nullptr;

        void require(const Tokenizer::PToken token, My::Tokenizer::Token::SubTypes type);
        std::string walk(Node::PNode node, std::string prefix, bool last);

    };//class SyntaxAnalyzer

}// namespace My