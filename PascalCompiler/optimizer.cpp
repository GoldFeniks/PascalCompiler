#include "optimizer.hpp"
#include <vector>

using namespace pascal_compiler;
using namespace optimizer;

void unreachable_code_optimizer::optimize(const tree_node_p node) {
    if (!node) return;
    for (size_t i = 0; i < node->children_.size(); ++i) {
        if (!node->children_[i])
            continue;
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
        case tree_node::node_category::continue_op:
        case tree_node::node_category::break_op:
        case tree_node::node_category::exit:
            node->children_.resize(i + 1);
            break;
        case tree_node::node_category::variable:
            used_symbols_.insert(node->children_[i]->name());
            break;
        case tree_node::node_category::index:
        case tree_node::node_category::function:
        case tree_node::node_category::field_access:
            used_symbols_.insert(std::dynamic_pointer_cast<applied>(node->children_[i])->variable()->name());
            break;
        default:
            optimize(node->children_[i]);
        }
    }
}

void unreachable_code_optimizer::optimize(symbols_table& table) {
    for (const auto it : table.vector())
        if (it.second.first->is_category(type::type_category::function)) {
            used_symbols_.clear();
            auto t = std::dynamic_pointer_cast<function_type>(it.second.first);
            optimize(it.second.second);
            for (auto& v : t->table_.vector_)
                if (used_symbols_.find(v.first) == used_symbols_.end() && v.first != "result") {
                    v.second = make_pair(nil(), nullptr);
                    t->table_.table_[v.first] = v.second;
                }
            t->table_.calculate_offsets();
            optimize(t->table_);
        }
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
        return optimize_loop_body(node, node->body());
    if (node->is_downto() && to < from || !node->is_downto() && from < to)
        return optimize_loop_body(node, node->body());
    if (from == to)
        return remove_break_continue(node->body());
    return nullptr;
}

tree_node_p unreachable_code_optimizer::optimize_repeat(const repeat_node_p node) {
    if (node->children().size() <= 1)
        return nullptr;
    optimize(node);
    long long cond;
    if (!get_int_value(node->condition(), cond) || cond == 0)
        return optimize_loop_body(node, node->body());
    return remove_break_continue(node->children().front());
}

tree_node_p unreachable_code_optimizer::optimize_while(const while_node_p node) {
    if (node->children().size() <= 1)
        return nullptr;
    optimize(node);
    long long cond;
    if (!get_int_value(node->condition(), cond) || cond != 0)
        return optimize_loop_body(node, node->body());
    return nullptr;
}

tree_node_p unreachable_code_optimizer::optimize_if(const if_node_p node) {
    optimize(node);
    long long cond;
    if (!get_int_value(node->condition(), cond))
        return node;
    if (cond == 0)
        return node->else_branch(); 
    return node->then_branch();
}

tree_node_p unreachable_code_optimizer::optimize_loop_body(const tree_node_p loop_node, const tree_node_p body) {
    if (body->children_.empty()) {
        if (body->category() == tree_node::node_category::continue_op ||
            body->category() == tree_node::node_category::break_op)
            return nullptr;
        if (body->category() == tree_node::node_category::exit) {
            if (loop_node->category() == tree_node::node_category::repeat)
                return body;
            return optimize_if(make_if_from_loop(loop_node));
        }
        return loop_node;
    }
    if (body->children_.front()->category() == tree_node::node_category::continue_op ||
        body->children_.front()->category() == tree_node::node_category::break_op)
        return nullptr;
    if (body->children_.back()->category() == tree_node::node_category::continue_op) {
        body->children_.pop_back();
        return loop_node;
    }
    switch (body->children_.back()->category()) {
    case tree_node::node_category::break_op:
        body->children_.pop_back();
        return body;
    case tree_node::node_category::exit:
        if(loop_node->category() == tree_node::node_category::repeat)
            return body;
        return optimize_if(make_if_from_loop(loop_node));
    default:
        return loop_node;
    }
}

tree_node_p unreachable_code_optimizer::remove_break_continue(const tree_node_p node) const {
    for (const auto it : node->children_) {
        if (node->category() == tree_node::node_category::if_op) {
            const auto n = std::dynamic_pointer_cast<if_node>(it);
            if (n->then_branch())
                remove_break_continue(n->then_branch());
            if (n->else_branch())
                remove_break_continue(n->else_branch());
        }
    }
    if (node->children_.back()->category() == tree_node::node_category::continue_op ||
        node->children_.back()->category() == tree_node::node_category::break_op)
        node->children_.pop_back();
    return node;
}

if_node_p unreachable_code_optimizer::make_if_from_loop(const tree_node_p loop_node) const {
    auto if_n = std::make_shared<if_node>(loop_node->position_);
    switch (loop_node->category()) {
    case tree_node::node_category::for_op:
    {
        const auto for_n = std::dynamic_pointer_cast<for_node>(loop_node);
        if_n->push_back(std::make_shared<operation_node>(
            for_n->is_downto() ? tokenizer::token::sub_types::greater_equal : tokenizer::token::sub_types::less_equal,
            for_n->from()->position(), integer(), for_n->from(), for_n->to(), for_n->is_downto() ? ">=" : "<="));
        if_n->set_then_branch(for_n->body());
        break;
    }
    case tree_node::node_category::while_op:
        if_n->push_back(loop_node->children_.front());
        if_n->set_then_branch(loop_node->children_.back());
        break;
    default:
        throw std::logic_error("This point should never be reached!");
    }
    return if_n;
}
