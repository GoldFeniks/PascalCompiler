#include "tree.hpp"
#include "boost/format.hpp"

using namespace pascal_compiler;
using namespace syntax_analyzer;
using namespace tree;
using namespace operations;
using namespace types;
using namespace code;

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

void tree_node::to_asm_code(asm_code& code) {
    for (const auto& it : children_)
        it->to_asm_code(code);
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

void variable_node::to_asm_code(asm_code& code) {
    switch (type()->category()) { 
    case type::type_category::character: 
        code.push_back({ asm_command::type::push,{ asm_mem::mem_size::word, code.get_offset(name()) } });
        break;
    case type::type_category::integer: 
        code.push_back({ asm_command::type::push,{ asm_mem::mem_size::dword, code.get_offset(name()) } });
        break;
    case type::type_category::real: 
        code.push_back({ asm_command::type::push,{ asm_mem::mem_size::dword, code.get_offset(name()) - 4 } });
        code.push_back({ asm_command::type::push,{ asm_mem::mem_size::dword, code.get_offset(name()) } });
        break;
    case type::type_category::array:
    case type::type_category::record:
    case type::type_category::function:
    case type::type_category::modified: 
        throw std::logic_error("Not implemented");
    default: 
        throw std::logic_error("This point should never be reached");
    }
}

std::string constant_node::value_string() const {
    switch (type()->category()) {
    case type::type_category::integer:  
        return std::to_string(static_cast<long long>(value_));
    case type::type_category::real:
        return std::to_string(static_cast<double>(value_));
    case type::type_category::character:
        return std::to_string(static_cast<char*>(value_)[0]);
    default:
        throw std::logic_error("This point should be unreachable");
    }
}

void constant_node::to_asm_code(asm_code& code) {
    switch (type()->category()) {
    case type::type_category::character:
    case type::type_category::integer:
        code.push_back({ asm_command::type::push, { std::to_string(static_cast<long long>(value_)) } });
        break;
    case type::type_category::real:
    {
        const auto s = code.add_double_constant(static_cast<double>(value_));
        code.push_back({ asm_command::type::push, { asm_mem::mem_size::dword, s, 4 } });
        code.push_back({ asm_command::type::push, { asm_mem::mem_size::dword, s, 0 } });
        break;
    }
    default:
        throw std::logic_error("This point should be unreachable");
    }    
}

//class operation_node

const std::unordered_map<tokenizer::token::sub_types, asm_command::type> operation_node::ops = {
    { tokenizer::token::sub_types::plus, asm_command::type::add },
    { tokenizer::token::sub_types::minus, asm_command::type::sub },
    { tokenizer::token::sub_types::mult, asm_command::type::mul },
    { tokenizer::token::sub_types::divide, asm_command::type::div },
    { tokenizer::token::sub_types::and, asm_command::type::and },
    { tokenizer::token::sub_types::or, asm_command::type::or },
    { tokenizer::token::sub_types::xor, asm_command::type::xor },
    { tokenizer::token::sub_types::div, asm_command::type::div },
    { tokenizer::token::sub_types::mod, asm_command::type::div },
    { tokenizer::token::sub_types::plus_assign, asm_command::type::add },
    { tokenizer::token::sub_types::minus_assign, asm_command::type::sub },
    { tokenizer::token::sub_types::mult_assign, asm_command::type::mul },
    { tokenizer::token::sub_types::divide_assign, asm_command::type::div },
    { tokenizer::token::sub_types::assign, asm_command::type::mov },
};

const std::unordered_map<tokenizer::token::sub_types, asm_command::type> operation_node::f_ops = {
    { tokenizer::token::sub_types::plus, asm_command::type::addsd },
    { tokenizer::token::sub_types::minus, asm_command::type::subsd },
    { tokenizer::token::sub_types::mult, asm_command::type::mulsd },
    { tokenizer::token::sub_types::divide, asm_command::type::divsd },
    { tokenizer::token::sub_types::plus_assign, asm_command::type::addsd },
    { tokenizer::token::sub_types::minus_assign, asm_command::type::subsd },
    { tokenizer::token::sub_types::mult_assign, asm_command::type::mulsd },
    { tokenizer::token::sub_types::divide_assign, asm_command::type::divsd },
    { tokenizer::token::sub_types::assign, asm_command::type::movsd },
};

tokenizer::token::sub_types operation_node::operation_type() const { return operation_type_; }

const tree_node_p& operation_node::left() const { return left_; }

const tree_node_p& operation_node::right() const { return right_; }

void operation_node::to_asm_code(asm_code& code) {
    //TODO ADD UNARY
    //TODO ADD CHAR
    const auto b = is_assign();  
    if (!b)
        left_->to_asm_code(code);
    right_->to_asm_code(code);
    asm_mem::mem_size mem_size;
    asm_command::type com_type;
    asm_reg::reg_type reg1, reg2;
    if (type() == real()) {
        reg1 = asm_reg::reg_type::xmm0;
        reg2 = asm_reg::reg_type::xmm1;
        if (!b) {
            code.push_back({ asm_command::type::movsd, reg2,{ asm_reg::reg_type::esp, asm_mem::mem_size::qword } });
            code.push_back({ asm_command::type::add,{ asm_reg::reg_type::esp },{ "8" } });
        }
        code.push_back({ asm_command::type::movsd, reg1, { asm_reg::reg_type::esp, asm_mem::mem_size::qword } });
        code.push_back({ asm_command::type::add,{asm_reg::reg_type::esp },{ "8" } });
        com_type = f_ops.at(operation_type_);
        mem_size = asm_mem::mem_size::qword;
    }
    else {
        reg1 = asm_reg::reg_type::eax;
        reg2 = asm_reg::reg_type::ebx;
        if (!b) {
            code.push_back({ asm_command::type::pop, reg2 });
        }
        code.push_back({ asm_command::type::pop, reg1 });
        mem_size = asm_mem::mem_size::dword;
        com_type = ops.at(operation_type_);
    }
    if (b)
        code.push_back({ com_type,{ mem_size, code.get_offset(left_->name()) }, reg1 });
    else {
        if (com_type == asm_command::type::mul ||
            com_type == asm_command::type::div)
            code.push_back({ com_type, reg2 });
        else
            code.push_back({ com_type, reg1, reg2 });
        if (type() == real()) {
            code.push_back({ asm_command::type::sub,{ asm_reg::reg_type::esp },{ "8" } });
            code.push_back({ asm_command::type::movsd, { asm_reg::reg_type::esp, asm_mem::mem_size::qword }, reg1 });
        }
        else
            code.push_back({ asm_command::type::push, reg1 });
    }
}

bool operation_node::is_assign() const {
    return operation_type_ == tokenizer::token::sub_types::plus_assign   ||
           operation_type_ == tokenizer::token::sub_types::minus_assign  ||
           operation_type_ == tokenizer::token::sub_types::mult_assign   ||
           operation_type_ == tokenizer::token::sub_types::divide_assign ||
           operation_type_ == tokenizer::token::sub_types::assign;
}

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

void write_node::to_asm_code(asm_code& code) {
    std::vector<std::shared_ptr<asm_arg>> args;
    std::string format = "\"";
    size_t offset = 0;
    for (const auto& it : children()) {
        const auto type = std::dynamic_pointer_cast<typed>(it);
        asm_mem::mem_size size;
        switch (type->type()->category()) { 
        case type::type_category::character: 
            format += "%c";
            size = asm_mem::mem_size::word;
            break;
        case type::type_category::integer:
            format += "%d";
            size = asm_mem::mem_size::dword;
            break;
        case type::type_category::real: 
            format += "%f";
            size = asm_mem::mem_size::qword;
            break;
        default: 
            throw std::logic_error("This point should never be reached");
        }
        it->to_asm_code(code);
        args.push_back(std::make_shared<asm_reg>(asm_reg::reg_type::eax, size, offset));
        offset += type->type()->data_size();
    }
    code.push_back({ asm_command::type::mov, asm_reg::reg_type::eax, asm_reg::reg_type::esp });
    reverse(args.begin(), args.end());
    args.insert(args.begin(), std::make_shared<asm_imm>( format + "\\n\"" ));
    code.push_back({ asm_command::type::printf, args });
    code.push_back({ asm_command::type::add, asm_reg::reg_type::esp, std::to_string(offset) });
}
