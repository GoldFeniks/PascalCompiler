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

    namespace optimizer {

        class basic_optimizer;
        class unreachable_code_optimizer;

    }// namespace optimizer

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
                    function, call, index, field_access, null, cast, write,
                    repeat, if_op, while_op, for_op, break_op, continue_op, exit
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
                virtual void to_asm_code(asm_code& code, bool is_left = false);

            private:

                friend class optimizer::basic_optimizer;
                friend class optimizer::unreachable_code_optimizer;
                friend class if_node;

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
                void to_asm_code(asm_code& code, const bool is_left = false) override;

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
                void to_asm_code(asm_code& code, const bool is_left = false) override;

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

                operation_node(const tokenizer::token::sub_types type, const position_type& position, const type_p& result_type,
                    const tree_node_p& left, const tree_node_p& right, const std::string& name) :
                    tree_node(name, node_category::operation, position, left, right),
                    typed(result_type), operation_type_(type), left_(left), right_(right) {}

                tokenizer::token::sub_types operation_type() const;
                const tree_node_p& left() const;
                const tree_node_p& right() const;
                bool is_assign() const;

                void to_asm_code(asm_code& code, const bool is_left = false) override;

            private:

                void to_asm_assign(asm_code& code) const;
                void to_asm(asm_code& code) const;
                void to_asm_compare(asm_code& code) const;
                tokenizer::token::sub_types operation_type_;
                tree_node_p left_ = nullptr, right_ = nullptr;
                static const std::unordered_map<tokenizer::token::sub_types, asm_command::type> ops, f_ops;

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

                void to_asm_code(asm_code& code, bool is_left) override;

            };// class call_node

            void put_value_on_stack(asm_code& code, type_p type, const tree_node::position_type position);

            class index_node;
            typedef std::shared_ptr<index_node> index_node_p;

            class index_node : public tree_node, public typed, public applied {
                
            public:

                index_node(const position_type& position, const tree_node_p& variable, 
                    const type_p& result_type, const tree_node_p& index) :
                    tree_node("[]", node_category::index, position, variable, index), typed(result_type), 
                    applied(variable), index_(index) {}

                const tree_node_p& index() const;
                void to_asm_code(asm_code& code, const bool is_left = false) override;

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
                void to_asm_code(asm_code& code, bool is_left) override;

            private:

                variable_node_p field_;

            };// class field_access_node

            class cast_node;
            typedef std::shared_ptr<cast_node> cast_node_p;

            class cast_node : public tree_node, public typed, public applied {
                
            public:

                cast_node(const type_p& type, const tree_node_p& node, const position_type& position);
                void to_asm_code(asm_code& code, const bool is_left = false) override;

            };// class cast_node

            class write_node;
            typedef std::shared_ptr<write_node> write_node_p;

            class write_node : public tree_node {

            public:

                template<typename... T>
                explicit write_node(const position_type& position, T... children) 
                   : tree_node("write", node_category::write, position, children...) {}

                void to_asm_code(asm_code& code, const bool is_left = false) override;

            };//class write node

            class repeat_node;
            typedef std::shared_ptr<repeat_node> repeat_node_p;

            class repeat_node : public tree_node {

            public:

                repeat_node(const position_type& position, const tree_node_p statements) 
                    : tree_node("repeat", node_category::repeat, position, statements) {}

                void set_condition(const tree_node_p condition);
                tree_node_p condition() const;
                tree_node_p body() const;
                void to_asm_code(asm_code& code, const bool is_left = false) override;

            };//class repeat_node

            class for_node;
            typedef std::shared_ptr<for_node> for_node_p;

            class for_node : public tree_node {
                
            public:

                explicit for_node(const position_type& position)
                    : tree_node("for", node_category::for_op, position) {}

                void set_downto(const bool value);
                bool is_downto() const;
                tree_node_p from() const;
                tree_node_p to() const;
                tree_node_p body() const;
                void to_asm_code(asm_code& code, bool is_left) override;

            private:

                bool is_downto_ = false;

            };//class for_node

            class while_node;
            typedef std::shared_ptr<while_node> while_node_p;

            class while_node : public tree_node {
                
            public:

                explicit while_node(const position_type& position)
                    : tree_node("while", node_category::while_op, position) {}

                void to_asm_code(asm_code& code, bool is_left) override;
                tree_node_p condition() const;
                tree_node_p body() const;

            };//class while_node

            class break_node;
            typedef std::shared_ptr<break_node> break_node_p;

            class break_node : public tree_node {
                
            public:

                explicit break_node(const position_type& position) 
                    : tree_node("break", node_category::break_op, position) {}

                void to_asm_code(asm_code& code, bool is_left) override;

            };// class break_node

            class continue_node;
            typedef std::shared_ptr<continue_node> continue_node_p;

            class continue_node : public tree_node {

            public:

                explicit continue_node(const position_type& position)
                    : tree_node("break", node_category::break_op, position) {}

                void to_asm_code(asm_code& code, bool is_left) override;

            };// class continue_node

            class if_node;
            typedef std::shared_ptr<if_node> if_node_p;

            class if_node : public tree_node {
                
            public:

                explicit if_node(const position_type& position)
                    : tree_node("if", node_category::if_op, position) {}

                void to_asm_code(asm_code& code, bool is_left) override;
                tree_node_p condition() const;
                void set_then_branch(tree_node_p node);
                void set_else_branch(tree_node_p node);
                tree_node_p then_branch() const;
                tree_node_p else_branch() const;

            };// class if_node

            class exit_node;
            typedef std::shared_ptr<exit_node> exit_node_p;

            class exit_node : public tree_node {

            public:

                explicit exit_node(const position_type& position)
                    : tree_node("exit", node_category::exit, position) {}

                void to_asm_code(asm_code& code, bool is_left) override;

            };

        }// namespace tree

    }// namespace SyntaxAnalyzer

}// namespace pascal_compiler
