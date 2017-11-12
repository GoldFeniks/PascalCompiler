#pragma once
#include <functional>
#include <exception>
#include "tree.hpp"
#include "type.hpp"
#include "tokenizer.hpp"
#include "unordered_map"
#include "exceptions.hpp"

namespace pascal_compiler {

    namespace syntax_analyzer {

        using namespace tree;
        using namespace types;

        namespace operations {

            class unsupported_operands_types : public exception {
                
            public:

                unsupported_operands_types(const typed_p& left, const typed_p& right, const tokenizer::token::sub_types type) {
                    message_ = str(boost::format("Unsupported operands types \"%1%\", \"%2%\" for %3%")
                        % type::type_strings[static_cast<unsigned int>(left->type()->category())]
                        % type::type_strings[static_cast<unsigned int>(right->type()->category())]
                        % tokenizer::token::sub_types_strings[static_cast<unsigned int>(type)]);
                }

                unsupported_operands_types(const typed_p& left, const tokenizer::token::sub_types type) {
                    message_ = str(boost::format("Unsupported operand types \"%1%\" for %2%")
                        % type::type_strings[static_cast<unsigned int>(left->type()->category())]
                        % tokenizer::token::sub_types_strings[static_cast<unsigned int>(type)]);
                }

                unsupported_operands_types(const type_p& left, const type_p& right, const tokenizer::token::sub_types type) {
                    message_ = str(boost::format("Unsupported operands types \"%1%\", \"%2%\" for %3%")
                        % type::type_strings[static_cast<unsigned int>(left->category())]
                        % type::type_strings[static_cast<unsigned int>(right->category())]
                        % tokenizer::token::sub_types_strings[static_cast<unsigned int>(type)]);
                }

            };

            class incompatible_types : public exception {

            public:

                incompatible_types(const type_p& left, const type_p& right);
                
            };
            
            inline void require_integer(const constant_node_p& left, const constant_node_p& right,
                const tokenizer::token::sub_types type) {
                if (left->type()->category() != type::type_category::integer ||
                    right->type()->category() != type::type_category::integer)
                    throw unsupported_operands_types(left, right, type);
            }

            inline void require_integer(const type_p& left, const type_p& right,
                const tokenizer::token::sub_types type) {
                if (left != integer() || right != integer())
                    throw unsupported_operands_types(left, right, type);
            }

            template<typename T, typename Left = typename T::first_argument_type, typename Right = typename T::second_argument_type>
            constant_node_p calculate(const constant_node_p& left, const constant_node_p& right, const type_p& type) {
                T op;
                const typename T::result_type result = op(left->get_value<Left>(), right->get_value<Right>());
                return std::make_shared<constant_node>(std::to_string(result), type, tokenizer::token::value(result), left->position());                
            }

            template<template<typename T>typename T>
            constant_node_p calculate(const constant_node_p& left, const constant_node_p& right, const bool forse_int = false) {
                if (left->type()->category() == type::type_category::integer && 
                    right->type()->category() == type::type_category::integer)
                    return calculate<T<long long>>(left, right, integer());
                return calculate<T<double>>(left, right, forse_int ? integer() : real());
            }

            template<typename T, typename Left = typename T::argument_type>
            constant_node_p calculate(const constant_node_p& left, const type_p& type) {
                T op;
                const typename T::result_type result = op(left->get_value<Left>());
                return std::make_shared<constant_node>(std::to_string(result), type, tokenizer::token::value(result), left->position());
            }

            template<template<typename T>typename T>
            constant_node_p calculate(const constant_node_p& left, const bool forse_int = false) {
                if (left->type()->category() == type::type_category::integer)
                    return calculate<T<long long>>(left, integer());
                return calculate<T<double>>(left, forse_int ? integer() : real());
            }

            template<typename T>
            struct shift_left {
                T operator()(const T& x, const T& y) const { return x << y; }
                typedef T result_type;
                typedef T first_argument_type;
                typedef T second_argument_type;
            };

            template<typename T>
            struct shift_right {
                T operator()(const T& x, const T& y) const { return x >> y; }
                typedef T result_type;
                typedef T first_argument_type;
                typedef T second_argument_type;
            };

            const std::unordered_map<tokenizer::token::sub_types, 
                std::function<constant_node_p(const constant_node_p&, const constant_node_p&)>> binary_operations = {
                    { 
                        tokenizer::token::sub_types::plus, 
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::plus>(left, right);
                        }
                    },
                    {
                        tokenizer::token::sub_types::minus,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::minus>(left, right);
                        }
                    },
                    {
                        tokenizer::token::sub_types::mult,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::multiplies>(left, right);
                        }
                    },
                    {
                        tokenizer::token::sub_types::divide,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::divides<double>>(left, right, real());
                        }
                    },
                    {
                        tokenizer::token::sub_types::mod,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::modulus<long long>>(left, right, integer());
                        }
                    },
                    {
                        tokenizer::token::sub_types::div,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::divides<long long>>(left, right, integer());
                        }
                    },
                    {
                        tokenizer::token::sub_types::shift_left,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<shift_left<long long>>(left, right, integer());
                        }
                    },
                    {
                        tokenizer::token::sub_types::shift_right,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<shift_right<long long>>(left, right, integer());
                        }
                    },
                    {
                        tokenizer::token::sub_types::and,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::bit_and<long long>>(left, right, integer());
                        }
                    },
                    {
                        tokenizer::token::sub_types::or,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::bit_or<long long>>(left, right, integer());
                        }
                    },
                    {
                        tokenizer::token::sub_types::xor,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::bit_xor<long long>>(left, right, integer());
                        }
                    },
                    {
                        tokenizer::token::sub_types::less,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::less>(left, right, true);
                        }
                    },
                    {
                        tokenizer::token::sub_types::less_equal,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::less_equal>(left, right, true);
                        }
                    },
                    {
                        tokenizer::token::sub_types::greater,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::greater>(left, right, true);
                        }
                    },
                    {
                        tokenizer::token::sub_types::greater_equal,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::greater_equal>(left, right, true);
                        }
                    },
                    {
                        tokenizer::token::sub_types::greater_equal,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::greater_equal>(left, right, true);
                        }
                    },
                    {
                        tokenizer::token::sub_types::equal,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::equal_to>(left, right, true);
                        }
                    },
                    {
                        tokenizer::token::sub_types::not_equal,
                        [](const constant_node_p& left, const constant_node_p& right) -> constant_node_p {
                            return calculate<std::not_equal_to>(left, right, true);
                        }
                    }
                };

            const std::unordered_map<tokenizer::token::sub_types,
                std::function<constant_node_p(const constant_node_p&)>> unary_operations = {
                    {
                        tokenizer::token::sub_types::minus,
                        [](const constant_node_p& left) -> constant_node_p {
                            return calculate<std::negate>(left);
                        }
                    },
                    {
                        tokenizer::token::sub_types::plus,
                        [](const constant_node_p& left) -> constant_node_p {
                            return left;
                        }
                    },
                    {
                        tokenizer::token::sub_types::not,
                        [](const constant_node_p& left) -> constant_node_p {
                            if (left->type()->category() != type::type_category::integer)
                                throw unsupported_operands_types(left, tokenizer::token::sub_types::not);
                            return calculate<std::bit_not<long long>>(left, integer());
                        }
                    },
                };

            extern inline constant_node_p calculate(const tokenizer::token::sub_types type,
                                               const constant_node_p& left, const constant_node_p& right);

            extern inline constant_node_p calculate(const tokenizer::token::sub_types type, const constant_node_p& left);

            bool inline is_relational(const tokenizer::token::sub_types type);
            bool inline is_int_only(const tokenizer::token::sub_types type);

            inline type_p get_type_for_operands(type_p left, type_p right, const tokenizer::token::sub_types op);
            
        }// namespace operations
       
    }// namespace syntax_analyzer

}// namespace pascal_compiler
