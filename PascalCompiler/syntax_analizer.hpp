#pragma once
#include <memory>
#include "tokenizer.hpp"

namespace My {

    class SyntaxAnalizer {

    public:

        class SyntaxErrorException : std::exception {

        public:

            SyntaxErrorException(const My::Tokenizer::Token& token, My::Tokenizer::Token::SubTypes type);

            virtual const char* what() const override;

        };

        class Node {

        public:

            Node(const Tokenizer::Token& token, Node* parent) : position(token.GetPosition()), Parent(parent) {};

            const std::pair<int, int>& GetPosition() {
                return position;
            }

            Node* Parent;

        private:

            const std::pair<int, int> position;

        };//class Node

        class ExpressionNode : public Node {

        public:

            ExpressionNode(const Tokenizer::Token& token, Node* parent) : Node(token, parent) {};

        };//class ExpressionNode

        class VariableNode : public ExpressionNode {

        public:

            VariableNode(const Tokenizer::Token& token, Node* parent) : 
                ExpressionNode(token, parent), name(token.GetStringValue()) {};

            const std::string& GetName() {
                return name;
            }

        private:

            std::string name;

        };//class VariableNode

        template <typename T>
        class ConstantNode : public ExpressionNode {

        public:

            ConstantNode(const Tokenizer::Token& token, Node* parent) : 
                ExpressionNode(token, parent), value(token.GetValueCopy<T>()) {};

            const T& GetValue() {
                return value;
            }

        private:

            T value;

        };//class ConstantNode

        using IntNode = ConstantNode<unsigned long long>;
        using FloatNode = ConstantNode<double>;
        using StringNode = ConstantNode<std::string>;
        using CharNode = ConstantNode<char>;
        //ConstantNode specifications

        typedef std::shared_ptr<ExpressionNode> PExpressionNode;

        class UnaryOperation : ExpressionNode {

        public:

            UnaryOperation(const My::Tokenizer::Token& token, std::shared_ptr<ExpressionNode> operand, Node* parent) :
                ExpressionNode(token, parent), Operand(operand) {};

            PExpressionNode Operand;

        };

        class BinaryOperation : ExpressionNode {

        public:

            BinaryOperation(const My::Tokenizer::Token& token, std::shared_ptr<ExpressionNode> left, 
                std::shared_ptr<ExpressionNode> right, Node* parent) : 
                ExpressionNode(token, parent), Left(left), Right(right) {};

            PExpressionNode Left, Right;

        };

        SyntaxAnalizer(Tokenizer&& tokenizer) : tokenizer(std::move(tokenizer)) {};
        SyntaxAnalizer(const std::string file) : tokenizer(Tokenizer(file)) {};
        SyntaxAnalizer(std::ifstream&& file) : tokenizer(std::move(file)) {};

        PExpressionNode ParseExpression();
        PExpressionNode ParseTerm();
        PExpressionNode ParseFactor();

    private:

        Tokenizer tokenizer;

        void require(const Tokenizer::Token& token, My::Tokenizer::Token::SubTypes type);

    };//class SyntaxAnalizer

}// namespace My