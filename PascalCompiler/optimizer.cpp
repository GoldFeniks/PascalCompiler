#include "optimizer.hpp"
#include <vector>

using namespace pascal_compiler;
using namespace optimizer;

void unreachable_code_optimizer::optimize(const tree_node_p node) {
    if (!node) return;
    for (size_t i = 0; i < node->children_.size(); ++i) {
        switch (node->children_[i]->category()) { 
        case tree_node::node_category::if_op: 
            node->children_[i] = optimize_if(std::dynamic_pointer_cast<if_node>(node->children_[i]));
            continue;
        case tree_node::node_category::while_op: 
            node->children_[i] = optimize_while(std::dynamic_pointer_cast<while_node>(node->children_[i])); 
            continue;
        case tree_node::node_category::for_op:
            node->children_[i] = optimize_for(std::dynamic_pointer_cast<for_node>(node->children_[i]));
            continue;
        case tree_node::node_category::repeat:
            node->children_[i] = optimize_repeat(std::dynamic_pointer_cast<repeat_node>(node->children_[i]));
            continue;
        default:;
        }
    }
}

void unreachable_code_optimizer::optimize(symbols_table& table) {
    for (const auto it : table.vector())
        if (it.second.first->is_category(type::type_category::function))
            optimize(it.second.second);
}

bool unreachable_code_optimizer::get_int_value(tree_node_p node, long long& value) {
    if (node->category() == tree_node::node_category::variable) {
        const auto t = std::dynamic_pointer_cast<typed>(node)->type();
        node = std::dynamic_pointer_cast<variable_node>(node)->value();
        if (!t->is_category(type::type_category::modified) ||
            std::dynamic_pointer_cast<modified_type>(t)->modificator() == modified_type::modificator_type::var)
            return false;
    }
    if (node->category() == tree_node::node_category::constant) {
        value = std::dynamic_pointer_cast<constant_node>(node)->get_value<long long>();
        return true;
    }
    return false;
}

tree_node_p unreachable_code_optimizer::optimize_for(const for_node_p node) {
    if (node->children().size() <= 3)
        return nullptr;
    optimize(node);
    long long from, to;
    if (!get_int_value(node->from(), from) || !get_int_value(node->to(), to))
        return node;
    if (node->is_downto() && to < from || !node->is_downto() && from < to)
        return node;
    if (from == to)
        return node->body();
    return nullptr;
}

tree_node_p unreachable_code_optimizer::optimize_repeat(const repeat_node_p node) {
    if (node->children().size() <= 1)
        return nullptr;
    optimize(node);
    long long cond;
    if (!get_int_value(node->condition(), cond) || cond == 0)
        return node;
    return node->children().front();
}

tree_node_p unreachable_code_optimizer::optimize_while(const while_node_p node) {
    if (node->children().size() <= 1)
        return nullptr;
    optimize(node);
    long long cond;
    if (!get_int_value(node->condition(), cond) || cond != 0)
        return node;
    return nullptr;
}

tree_node_p unreachable_code_optimizer::optimize_if(const if_node_p node) {
    optimize(node);
    long long cond;
    if (!node->then_branch())
        return node->else_branch();
    if (!get_int_value(node->condition(), cond))
        return node;
    if (cond == 0)
        return node->else_branch();
    return node->then_branch();
}
