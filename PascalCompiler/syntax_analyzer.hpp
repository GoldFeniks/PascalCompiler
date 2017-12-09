#pragma once
#include <utility>
#include <vector>
#include "tokenizer.hpp"
#include "tree.hpp"
#include "type.hpp"
#include "operations.hpp"
#include "symbols_table.hpp"
#include "exceptions.hpp"
#include "boost/format.hpp"
#include "optimizer.hpp"

namespace pascal_compiler {

    namespace syntax_analyzer {

        using namespace tree;
        using namespace operations;
        using namespace code;
        using namespace optimizer;

        class syntax_error : public exception {
            
        public:

            syntax_error(const std::string& message, const tree_node::position_type position);

        };

        class declaration_not_found : public syntax_error {
            
        public:

            explicit declaration_not_found(const tokenizer::token_p& token) : 
                syntax_error(str(boost::format("Declaration not found \"%1%\"") 
                    % token->get_string_value()), token->get_position()) {};

        };

        class unexpected_token : public syntax_error {

        public:

            unexpected_token(const tokenizer::token_p& token, const tokenizer::token::sub_types type) :
                syntax_error(str(boost::format("%1% expected but %2% \"%3%\" found") %
                    tokenizer::token::sub_types_strings[static_cast<unsigned int>(type)] %
                    tokenizer::token::types_strings[static_cast<unsigned int>(token->get_type())] %
                    token->get_value_string()), token->get_position()) {}

        };

        class unexpected_type : public syntax_error {

        public:

            unexpected_type(const type_p& type, const type::type_category category,
                const tree_node::position_type position) :
                syntax_error(str(boost::format("%1% expected but %2% found") %
                    type::type_strings[static_cast<unsigned int>(category)] %
                    type::type_strings[static_cast<unsigned int>(type->category())]), position) {}
           
        };

        class field_not_found : public syntax_error {
            
        public:

            explicit field_not_found(const tokenizer::token_p& token) :
                syntax_error(str(boost::format("field not found \"%1%\"") % token->get_string_value()), 
                    token->get_position()) {}

        };

        class syntax_analyzer {
            
        public:

            explicit syntax_analyzer(tokenizer&& tokenizer) : tokenizer_(std::move(tokenizer)) {}
            explicit syntax_analyzer(const std::string file) : tokenizer_(tokenizer(file)) {}
            explicit syntax_analyzer(std::ifstream&& file) : tokenizer_(move(file)) {}

            syntax_analyzer(const syntax_analyzer&) = delete;
            syntax_analyzer(syntax_analyzer&& other) noexcept : tokenizer_(std::move(other.tokenizer_)) 
                { swap(root_, other.root_); };

            syntax_analyzer& operator=(const syntax_analyzer&) = delete;
            syntax_analyzer& operator=(syntax_analyzer&& other) noexcept;
            void to_asm_code(asm_code& code);
            void set_optimizer(std::shared_ptr<basic_optimizer> optimizer);
            void parse();
            const std::vector<symbols_table>& tables() const;

        private:

            tokenizer tokenizer_;
            tree_node_p root_;
            std::vector<symbols_table> tables_;
            size_t loops_count_ = 0;
            std::shared_ptr<basic_optimizer> optimizer_ = std::make_shared<basic_optimizer>();

            void parse_program();

            tree_node_p parse_simple_expression();
            tree_node_p parse_expression();
            tree_node_p parse_term();
            tree_node_p parse_factor();
            tree_node_p parse_function_call(tree_node_p node);
            tree_node_p parse_index(tree_node_p node);
            tree_node_p parse_field_access(tree_node_p node);
            tree_node_p parse_actual_parameter_list(const function_type_p& type);
            tree_node_p parse_block();
            tree_node_p parse_typed_const(const type_p& type);
            tree_node_p parse_statements();
            tree_node_p parse_exit_statement();
            tree_node_p parse_statement();
            tree_node_p parse_compound_statement();
            tree_node_p parse_if_statement();
            tree_node_p parse_while_statement();
            tree_node_p parse_for_statement();
            tree_node_p parse_repeat_statement();
            tree_node_p parse_write_statement();
            tree_node_p parse_read_statement();
            tree_node_p parse_assignment_statement();
            tree_node_p parse_condition();

            type_p parse_type(const std::string& name = "");
            
            void parse_expressions_list(std::vector<tree_node_p>& list);
            void parse_identifier_list(std::vector<std::string>& names, symbols_table& table);

            void parse_declaration_part();
            void parse_type_declaration();
            void parse_var_declaration();
            void parse_const_declaration();
            void parse_function_declaration();
            void parse_procedure_declaration();
            void parse_formal_parameter_list();

            const symbols_table::symbol_t& find_declaration(const tokenizer::token_p& token);

            static void require(const tokenizer::token_p& token, const tokenizer::token::sub_types type);
            void require(const tokenizer::token::sub_types type) const;
            static void require(const type_p type, const type::type_category category, 
                const tree_node::position_type& position);
            static void require_types_compatibility(const type_p& left, const type_p& right,
                const tree_node::position_type& position);
            static void require_constant(const tree_node_p& node);
            void require_loop(const tokenizer::token::sub_types type) const;

            static bool simple_expression_operators(const tokenizer::token::sub_types type) {
                return type == tokenizer::token::sub_types::plus ||
                       type == tokenizer::token::sub_types::minus;
            }

            static bool term_operators(const tokenizer::token::sub_types type) {
                return type == tokenizer::token::sub_types::divide     ||
                       type == tokenizer::token::sub_types::mult       ||
                       type == tokenizer::token::sub_types::mod        ||
                       type == tokenizer::token::sub_types::div        ||
                       type == tokenizer::token::sub_types::shift_left ||
                       type == tokenizer::token::sub_types::shift_right;
            }

            static bool expression_operators(const tokenizer::token::sub_types type) {
                return type == tokenizer::token::sub_types::less          ||
                       type == tokenizer::token::sub_types::less_equal    ||
                       type == tokenizer::token::sub_types::equal         ||
                       type == tokenizer::token::sub_types::greater       ||
                       type == tokenizer::token::sub_types::greater_equal ||
                       type == tokenizer::token::sub_types::not_equal     ||
                       type == tokenizer::token::sub_types::and           ||
                       type == tokenizer::token::sub_types::or            ||
                       type == tokenizer::token::sub_types::xor;
            }

            static tree_node_p get_constant(const tree_node_p& node);

            template<tree_node_p(syntax_analyzer::*Parse)(), bool(*Cond)(const tokenizer::token::sub_types)>
            tree_node_p parse_operation(const type_p& d_type = nullptr) {
                auto result = (this->*Parse)();
                auto token = tokenizer_.current();
                while (Cond(token->get_sub_type())) {
                    tokenizer_.next();
                    auto node = (this->*Parse)();
                    const auto left = get_constant(result);
                    const auto right = get_constant(node);
                    if (left->category() == tree_node::node_category::constant &&
                        right->category() == tree_node::node_category::constant)
                        result = calculate(token->get_sub_type(), std::static_pointer_cast<constant_node>(left), 
                            std::static_pointer_cast<constant_node>(right));
                    else {
                        const auto type = get_type_for_operands(get_type(result), get_type(node), token->get_sub_type());
                        if (base_type(get_type(result)) != type)
                            result = std::make_shared<cast_node>(type, result, result->position());
                        if (base_type(get_type(node)) != type)
                            node = std::make_shared<cast_node>(type, node, node->position());
                        result = std::make_shared<operation_node>(token, result, node, d_type ? d_type : type);
                    }
                    token = tokenizer_.current();
                }
                return result;
            }

        };// class syntax_analyzer

    }// namespace syntax_analyzer

}// namespace pascal_compiler