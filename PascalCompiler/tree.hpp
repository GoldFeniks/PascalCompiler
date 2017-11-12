#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <memory>
#include "type.hpp"
#include "tokenizer.hpp"
#include "exceptions.hpp"
#include "asm_code.hpp"

namespace pascal_compiler {

    namespace syntax_analyzer {

        using namespace types;

        namespace tree {

            class constant_node;
            typedef std::shared_ptr<constant_node> constant_node_p;
            
        }// namespace tree

        namespace operations {

            extern inline type_p get_type_for_operands(type_p left, type_p right, const tokenizer::token::sub_types op);

        }// namespace operations

        using namespace operations;
        using namespace code;

        namespace tree {

            class convertion_error : public exception {
                
            public:

                convertion_error(const type_p& type_l, const type_p& type_r);

            };

            class tree_node;
            typedef std::shared_ptr<tree_node> tree_node_p;
            typedef std::vector<tree_node_p> nodes_vector;

            class tree_node {

            public:

                typedef std::pair<int, int> position_type;

                enum class node_category {
                    variable, constant, typed_constant, operation, procedure,
                    function, call, index, field_access, null, cast, write
                };

                template<typename... C>
                tree_node(const std::string& name, const node_category type, const position_type& position, C... children) :
                    name_(name), type_(type), position_(position) {
                    push_back(children...);
                }

                virtual ~tree_node() {}

                static void push_back() {}

                void push_back(const tree_node_p& node) {
                    if (node)
                        children_.push_back(node);
                }

                template<typename... C>
                void push_back(const tree_node_p& node, C... children) {
                    children_.push_back(node);
                    push_back(children...);
                }

                const nodes_vector& children() const;
                const std::string& name() const;
                node_category category() const;
                const position_type& position() const;
                std::string to_string(const std::string& prefix = "", const bool last = true) const;
                virtual void to_asm_code(code::asm_code& code);

            private:

                nodes_vector children_;
                std::string name_;
                node_category type_;
                position_type position_;
                
            };// class TreeNode

            class typed {

            public:

                virtual ~typed() = default;

                explicit typed(const type_p& type) : type_(type) {}

                virtual const type_p& type() const;
                
            private:

                type_p type_;
                
            };// class typed

            typedef std::shared_ptr<typed> typed_p;

            type_p get_type(const tree_node_p& node);

            class variable_node;
            typedef std::shared_ptr<variable_node> variable_node_p;

            class variable_node : public tree_node, public typed {
                
            public:

                variable_node(const std::string& name, const position_type& position, const type_p& type,
                    const tree_node_p& value = nullptr) :
                    tree_node(name, node_category::variable, position), typed(type), value_(value) {}

                virtual ~variable_node() {}

                const tree_node_p& value() const;
                void to_asm_code(asm_code& code) override;

            private:

                tree_node_p value_;

            };// class variable_node

            class constant_node : public tree_node, public typed {
                
            public:

                constant_node(const std::string& name, const type_p& type,
                    const tokenizer::token::value& value, const position_type& position) :
                    tree_node(name, node_category::constant, position), typed(type), value_(value) {}

                virtual ~constant_node() {}

                template<typename T>
                T get_value() const {
                    switch (type()->category()) {
                    case type::type_category::integer:
                        return T(static_cast<long long>(value_));
                    case type::type_category::real:
                        return T(static_cast<double>(value_));
                    case type::type_category::character:
                        return T(static_cast<char*>(value_)[0]);
                    default: 
                        throw std::logic_error("This point should be unreachable");
                    }
                }

                std::string value_string() const;
                void to_asm_code(asm_code& code) override;

            private:

                tokenizer::token::value value_;

            };// class constant_node

            class operation_node;
            typedef std::shared_ptr<operation_node> operation_node_p;

            class operation_node : public tree_node, public typed {
                
            public:

                operation_node(const tokenizer::token_p& token, const tree_node_p& left, const tree_node_p& right,
                    const type_p& result_type) :
                    tree_node(token->get_value_string(), node_category::operation, token->get_position(), left, right),
                    typed(result_type), operation_type_(token->get_sub_type()),
                    left_(left), right_(right) {}

                operation_node(const tokenizer::token_p& token, const tree_node_p& left) :
                    tree_node(token->get_value_string(), node_category::operation, token->get_position(), left),
                    typed(get_type(left)), operation_type_(token->get_sub_type()),
                    left_(left) {}

                tokenizer::token::sub_types operation_type() const;
                const tree_node_p& left() const;
                const tree_node_p& right() const;

                void to_asm_code(asm_code& code) override;

            private:

                tokenizer::token::sub_types operation_type_;
                tree_node_p left_ = nullptr, right_ = nullptr;
                static const std::unordered_map<tokenizer::token::sub_types, asm_command::type> ops, f_ops;
                bool is_assign() const;

            };// class operation_node

            class typed_constant_node;
            typedef std::shared_ptr<typed_constant_node> typed_constant_node_p;

            class typed_constant_node : public tree_node, public typed {
                
            public:

                typed_constant_node(const position_type& position, const type_p& result_type) :
                    tree_node("typed_constant", node_category::typed_constant, position), typed(result_type) {}

                virtual ~typed_constant_node() {}

            };// class typed_constant_node

            class applied {
                
            public:

                explicit applied(const tree_node_p& variable) : variable_(variable) {}

                const tree_node_p& variable() const;

            private:

                tree_node_p variable_;

            };// class applied

            class call_node;
            typedef std::shared_ptr<call_node> call_node_p;

            class call_node : public tree_node, public typed, public applied {
                
            public:

                call_node(const position_type& position, const tree_node_p& function,
                    const tree_node_p& args) :
                    tree_node("()", node_category::call, position, function, args),
                    typed(std::dynamic_pointer_cast<function_type>(get_type(function))->return_type()),
                    applied(function) {}

            };// class call_node

            class index_node;
            typedef std::shared_ptr<index_node> index_node_p;

            class index_node : public tree_node, public typed, public applied {
                
            public:

                index_node(const position_type& position, const tree_node_p& variable, 
                    const type_p& result_type, const tree_node_p& index) :
                    tree_node("[]", node_category::index, position, variable, index), typed(result_type), 
                    applied(variable), index_(index) {}

                const tree_node_p& index() const;

            private:

                tree_node_p index_;

            };// class index_node

            class field_access_node;
            typedef std::shared_ptr<field_access_node> field_access_node_p;

            class field_access_node : public tree_node, public typed, public applied {
                
            public:

                field_access_node(const position_type& position, const tree_node_p& variable, 
                    const variable_node_p& field) :
                    tree_node(".", node_category::field_access, position, variable, field), 
                    typed(get_type(field)), applied(variable), field_(field) {}

                const variable_node_p& field() const;

            private:

                variable_node_p field_;

            };// class field_access_node

            class cast_node;
            typedef std::shared_ptr<cast_node> cast_node_p;

            class cast_node : public tree_node, public typed, public applied {
                
            public:

                cast_node(const type_p& type, const tree_node_p& node, const position_type& position);

            };// class cast_node

            class write_node;
            typedef std::shared_ptr<write_node> write_node_p;

            class write_node : public tree_node {

            public:

                template<typename... T>
                explicit write_node(const position_type& position, T... children) 
                   : tree_node("write", node_category::write, position, children...) {}

                void to_asm_code(asm_code& code) override;

            };

        }// namespace tree

    }// namespace SyntaxAnalyzer

}// namespace pascal_compiler
