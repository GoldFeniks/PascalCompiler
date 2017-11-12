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
    get_type_for_operands(left->type(), right->type(), type);
    return binary_operations.at(type)(left, right);
}

constant_node_p operations::calculate(const tokenizer::token::sub_types type,
                                                         const constant_node_p& left) {
    const auto left_type = left->type()->category();
    if (left_type != type::type_category::integer && left_type != type::type_category::real)
        throw unsupported_operands_types(left, type);
    return unary_operations.at(type)(left);
}

bool operations::is_relational(const tokenizer::token::sub_types type) {
    return type == tokenizer::token::sub_types::equal         ||
           type == tokenizer::token::sub_types::not_equal     ||
           type == tokenizer::token::sub_types::less          ||
           type == tokenizer::token::sub_types::less_equal    ||
           type == tokenizer::token::sub_types::greater       ||
           type == tokenizer::token::sub_types::greater_equal;
}

bool operations::is_int_only(const tokenizer::token::sub_types type) {
    return type == tokenizer::token::sub_types::shift_left  ||
           type == tokenizer::token::sub_types::shift_right ||
           type == tokenizer::token::sub_types::div         ||
           type == tokenizer::token::sub_types::mod         ||
           type == tokenizer::token::sub_types::or          ||
           type == tokenizer::token::sub_types::and         ||
           type == tokenizer::token::sub_types::xor;
}

type_p operations::get_type_for_operands(type_p left, type_p right, const tokenizer::token::sub_types op) {
    left = base_type(left);
    right = base_type(right);
    if (!left->is_scalar() || !right->is_scalar())
        throw unsupported_operands_types(left, right, op);
    auto result_type = left;
    if (left->category() == type::type_category::integer && right->category() == type::type_category::real ||
        left->category() == type::type_category::real && right->category() == type::type_category::integer)
        result_type = real();
    else if (left != right)
        throw unsupported_operands_types(left, right, op);
    if (is_relational(op))
        return result_type;
    if (is_int_only(op)) {
        require_integer(left, right, op);
        return result_type;
    }
    if (left == character() || right == character())
        throw unsupported_operands_types(left, right, op);
    if (op == tokenizer::token::sub_types::divide)
        return real();
    return result_type;
}
