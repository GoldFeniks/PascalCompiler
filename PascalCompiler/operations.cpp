#include "operations.hpp"
#include "boost/format.hpp"

using namespace pascal_compiler;
using namespace syntax_analyzer;
using namespace tree;
using namespace types;

incompatible_types::incompatible_types(const type_p& left, const type_p& right) {
    message_ = str(boost::format("Incompatible types \"%1%\", \"%2%\"") % left->name() % right->name());
}

constant_node_p operations::calculate(const tokenizer::token::sub_types type,
                                                         const constant_node_p& left, const constant_node_p& right) {
    const auto left_type = left->type()->category(), right_type = right->type()->category();
    if (left_type != type::type_category::integer && left_type != type::type_category::real &&
        right_type != type::type_category::integer && right_type != type::type_category::real &&
        type != tokenizer::token::sub_types::equal && type != tokenizer::token::sub_types::not_equal)
        throw unsupported_operands_types(left, right, type);
    return binary_operations.at(type)(left, right);
}

constant_node_p operations::calculate(const tokenizer::token::sub_types type,
                                                         const constant_node_p& left) {
    const auto left_type = left->type()->category();
    if (left_type != type::type_category::integer && left_type != type::type_category::real)
        throw unsupported_operands_types(left, type);
    return unary_operations.at(type)(left);
}

type_p operations::get_type_for_operands(type_p left, type_p right, const tokenizer::token::sub_types op) {
    left = base_type(left);
    right = base_type(right);
    if (!left->is_scalar() || !right->is_scalar())
        throw unsupported_operands_types(left, right, op);
    if (left->category() == type::type_category::integer && right->category() == type::type_category::real ||
        left->category() == type::type_category::real && right->category() == type::type_category::integer ||
        op == tokenizer::token::sub_types::divide)
        return real();
    if (left->category() == right->category() && 
        (op == tokenizer::token::sub_types::equal || op == tokenizer::token::sub_types::not_equal))
        return left;
    if (left->category() == type::type_category::symbol || right->category() == type::type_category::symbol ||
        left->category() != right->category())
        throw incompatible_types(left, right);
    return left;
}
