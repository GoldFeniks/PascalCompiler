#include "tree.hpp"
#include "boost/format.hpp"

using namespace pascal_compiler;
using namespace syntax_analyzer;
using namespace tree;
using namespace operations;
using namespace types;

//class convertion_error
convertion_error::convertion_error(const type_p& type_l, const type_p& type_r) {
    message_ = str(boost::format("Can't convert from %1% to %2%") %
        type::type_strings[static_cast<unsigned int>(type_l->category())] %
        type::type_strings[static_cast<unsigned int>(type_r->category())]);
}

//class tree_node
const nodes_vector& tree_node::children() const { return children_; }

const std::string& tree_node::name() const { return name_; }

tree_node::node_category tree_node::category() const { return type_; }

const tree_node::position_type& tree_node::position() const { return position_; }

std::string tree_node::to_string(const std::string& prefix, const bool last) const {
    auto result = str(boost::format("%1%%2%%3%\n") % prefix %
        (last ? "\xE2\x94\x94\xE2\x94\x80" : "\xE2\x94\x9C\xE2\x94\x80") % name_);
    const auto spaces = std::string(name_.length() - 1, ' ');
    const auto n_prefix = str(boost::format("%1%%2%%3%") % prefix %
        (last ? "  " : "\xE2\x94\x82 ") % spaces);
    if (!children_.size())
        return result;
    for (auto i = 0; i < children_.size() - 1; ++i)
        result += children_[i]->to_string(n_prefix, false);
    return result + children_.back()->to_string(n_prefix, true);
}

const type_p& typed::type() const { return type_; }

type_p tree::get_type(const tree_node_p& node) {
    const auto type = std::dynamic_pointer_cast<typed>(node);
    if (!type)
        throw std::logic_error("This point should never be reached");
    return type->type();
}

//class variable_node
const tree_node_p& variable_node::value() const { return value_; }

//class operation_node
tokenizer::token::sub_types operation_node::operation_type() const { return operation_type_; }

const tree_node_p& operation_node::left() const { return left_; }

const tree_node_p& operation_node::right() const { return right_; }

const tree_node_p& applied::variable() const { return variable_; }

//class index_node
const tree_node_p& index_node::index() const { return index_; }

//class field_access_node
const variable_node_p& field_access_node::field() const { return field_; }

//class cast_node
cast_node::cast_node(const type_p& type, const tree_node_p& node, const position_type& position) : 
    tree_node(type->name(), node_category::cast, position, node), typed(type), applied(node) {
    const auto tl = base_type(type), tr = base_type(get_type(node));
    if (!tl->is_scalar() || !tr->is_scalar())
        throw convertion_error(tl, tr);
}
