#include "tree.hpp"
#include "boost/format.hpp"
#include <iterator>
#include "operations.hpp"

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

void tree_node::to_asm_code(asm_code& code, const bool is_left) {
    for (const auto& it : children_)
        it->to_asm_code(code, is_left);
}

const type_p& typed::type() const { return type_; }

type_p tree::get_type(const tree_node_p& node) {
    const auto type = std::dynamic_pointer_cast<typed>(node);
    if (!type)
        throw std::logic_error("This point should never be reached");
    return base_type(type->type());
}

//class variable_node
const tree_node_p& variable_node::value() const { return value_; }

void variable_node::to_asm_code(asm_code& code, const bool is_left) {
    const auto offset = code.get_offset(name());
    code.push_back({ asm_command::type::mov, asm_reg::reg_type::eax, {asm_reg::reg_type::ebp, asm_mem::mem_size::dword, offset.first} });
    code.push_back({ asm_command::type::sub, asm_reg::reg_type::eax, offset.second });
    if (type()->category() == type::type_category::modified) {
        const auto t = std::dynamic_pointer_cast<modified_type>(type());
        if (t->modificator() == modified_type::modificator_type::var)
            code.push_back({ asm_command::type::push, {asm_reg::reg_type::eax, asm_mem::mem_size::dword} });
        else
            code.push_back({ asm_command::type::push, asm_reg::reg_type::eax });
    }
    else
        code.push_back({ asm_command::type::push, asm_reg::reg_type::eax });
    if (!is_left)
        put_value_on_stack(code, type(), position());
}

std::string constant_node::value_string() const {
    switch (type()->category()) {
    case type::type_category::integer:  
        return std::to_string(static_cast<long long>(value_));
    case type::type_category::real:
        return std::to_string(static_cast<double>(value_));
    case type::type_category::character:
        return std::to_string(static_cast<char*>(value_)[0]);
    case type::type_category::string:
        return std::string(static_cast<char*>(value_));
    default:
        throw std::logic_error("This point should be unreachable");
    }
}

void constant_node::to_asm_code(asm_code& code, const bool is_left) {
    switch (type()->category()) {
    case type::type_category::character:
        code.push_back({ asm_command::type::sub, asm_reg::reg_type::esp, {"1"} });
        code.push_back({ asm_command::type::mov,{ asm_reg::reg_type::esp, asm_mem::mem_size::byte },
            { std::to_string(static_cast<char>(value_)) } });
        break;
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
    { tokenizer::token::sub_types::mult, asm_command::type::imul },
    { tokenizer::token::sub_types::and, asm_command::type::and },
    { tokenizer::token::sub_types::or, asm_command::type::or },
    { tokenizer::token::sub_types::xor, asm_command::type::xor },
    { tokenizer::token::sub_types::div, asm_command::type::idiv },
    { tokenizer::token::sub_types::mod, asm_command::type::idiv },
    { tokenizer::token::sub_types::plus_assign, asm_command::type::add },
    { tokenizer::token::sub_types::minus_assign, asm_command::type::sub },
    { tokenizer::token::sub_types::mult_assign, asm_command::type::imul },
    { tokenizer::token::sub_types::divide_assign, asm_command::type::idiv },
    { tokenizer::token::sub_types::assign, asm_command::type::mov },
    { tokenizer::token::sub_types::shift_left, asm_command::type::shl},
    { tokenizer::token::sub_types::shift_right, asm_command::type::shr},
    { tokenizer::token::sub_types::greater_equal, asm_command::type::setl },
    { tokenizer::token::sub_types::greater, asm_command::type::setle },
    { tokenizer::token::sub_types::less_equal, asm_command::type::setg },
    { tokenizer::token::sub_types::less, asm_command::type::setge },
    { tokenizer::token::sub_types::equal, asm_command::type::setne },
    { tokenizer::token::sub_types::not_equal, asm_command::type::sete }
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
    { tokenizer::token::sub_types::greater_equal, asm_command::type::setb },
    { tokenizer::token::sub_types::greater, asm_command::type::setbe },
    { tokenizer::token::sub_types::less_equal, asm_command::type::seta },
    { tokenizer::token::sub_types::less, asm_command::type::setae },
    { tokenizer::token::sub_types::equal, asm_command::type::jp },
    { tokenizer::token::sub_types::not_equal, asm_command::type::jnp }
};

tokenizer::token::sub_types operation_node::operation_type() const { return operation_type_; }

const tree_node_p& operation_node::left() const { return left_; }

const tree_node_p& operation_node::right() const { return right_; }

void operation_node::to_asm_code(asm_code& code, const bool is_left) {
    if (is_assign())
        to_asm_assign(code);
    else if (is_relational(operation_type_))
        to_asm_compare(code);
    else
        to_asm(code);
}

void operation_node::to_asm_assign(asm_code& code) const {
    left_->to_asm_code(code, true);
    right_->to_asm_code(code, !type()->is_scalar());
    asm_mem::mem_size mem_size;
    asm_command::type com_type;
    asm_reg::reg_type reg1;
    const auto t = base_type(type());
    if (t == real()) {
        reg1 = asm_reg::reg_type::xmm0;
        code.push_back({ asm_command::type::movsd, reg1, { asm_reg::reg_type::esp, asm_mem::mem_size::qword } });
        code.push_back({ asm_command::type::add, asm_reg::reg_type::esp, {"8"} });
        com_type = f_ops.at(operation_type_);
        mem_size = asm_mem::mem_size::qword;
    }
    else if (t->is_scalar()) {
        if (t == integer()) {
            reg1 = asm_reg::reg_type::eax;
            code.push_back({ asm_command::type::pop, reg1 });
        }
        else {
            reg1 = asm_reg::reg_type::al;
            code.push_back({ asm_command::type::mov, reg1, { asm_reg::reg_type::esp, asm_mem::mem_size::byte } });
            code.push_back({ asm_command::type::add, asm_reg::reg_type::esp, {"1"} });
        }
        mem_size = type() == integer() ? asm_mem::mem_size::dword : asm_mem::mem_size::byte;
        com_type = ops.at(operation_type_);
    }
    else {
        code.push_back({ asm_command::type::pop, asm_reg::reg_type::eax });
        code.push_back({ asm_command::type::pop, asm_reg::reg_type::ebx });
        code.push_back({ asm_command::type::mov, asm_reg::reg_type::ecx, t->data_size() / 4 });
        const auto label = code.get_label_name(position().first, position().second, "COPYSTRUCT");
        code.push_back({ asm_command::type::label, label });
        code.push_back({ asm_command::type::mov,asm_reg::reg_type::edx, { asm_reg::reg_type::eax, asm_mem::mem_size::dword } });
        code.push_back({ asm_command::type::mov,{ asm_reg::reg_type::ebx, asm_mem::mem_size::dword }, asm_reg::reg_type::edx });
        code.push_back({ asm_command::type::add, asm_reg::reg_type::eax, 4 });
        code.push_back({ asm_command::type::add, asm_reg::reg_type::ebx, 4 });
        code.push_back({ asm_command::type::loop, label });
        return;
    }
    if (com_type == asm_command::type::idiv)
        code.push_back({ asm_command::type::cdq });
    code.push_back({ asm_command::type::pop, asm_reg::reg_type::ebx });
    code.push_back({ com_type,{ asm_reg::reg_type::ebx, mem_size }, reg1 });
}

void operation_node::to_asm(asm_code& code) const {
    left_->to_asm_code(code);
    if (right_ == nullptr) {
        if (operation_type_ == tokenizer::token::sub_types::minus)
            if (type() == real()) 
                code.push_back({ asm_command::type::xor, {asm_reg::reg_type::esp, asm_mem::mem_size::byte, 7},{"128"} });
            else
                code.push_back({ asm_command::type::neg,{ asm_reg::reg_type::esp, asm_mem::mem_size::dword} });
        else if (operation_type_ == tokenizer::token::sub_types::not)
            code.push_back({ asm_command::type::not,{ asm_reg::reg_type::esp, asm_mem::mem_size::dword } });       
        return;
    }
    right_->to_asm_code(code);
    asm_command::type com_type;
    asm_reg::reg_type reg1, reg2;
    if (type() == real()) {
        reg1 = asm_reg::reg_type::xmm0;
        reg2 = asm_reg::reg_type::xmm1;
        code.push_back({ asm_command::type::movsd, reg2,{ asm_reg::reg_type::esp, asm_mem::mem_size::qword } });
        code.push_back({ asm_command::type::add,{ asm_reg::reg_type::esp },{ "8" } });
        code.push_back({ asm_command::type::movsd, reg1,{ asm_reg::reg_type::esp, asm_mem::mem_size::qword } });
        com_type = f_ops.at(operation_type_);
    }
    else {
        reg1 = asm_reg::reg_type::eax;
        reg2 = asm_reg::reg_type::ebx;
        com_type = ops.at(operation_type_);
        if (com_type == asm_command::type::shl ||
            com_type == asm_command::type::shr) {
            reg2 = asm_reg::reg_type::cl;
            code.push_back({ asm_command::type::mov, asm_reg::reg_type::ebx, asm_reg::reg_type::ecx });
            code.push_back({ asm_command::type::pop, asm_reg::reg_type::ecx });
        }
        else
            code.push_back({ asm_command::type::pop, reg2 });
        code.push_back({ asm_command::type::pop, reg1 });
    }
    if (com_type == asm_command::type::idiv)
        code.push_back({ asm_command::type::cdq });
    if (com_type == asm_command::type::imul ||
        com_type == asm_command::type::idiv)
        code.push_back({ com_type, reg2 });
    else
        code.push_back({ com_type, reg1, reg2 });
    if (type() == real()) {
        code.push_back({ asm_command::type::movsd,{ asm_reg::reg_type::esp, asm_mem::mem_size::qword }, reg1 });
    }
    else
        code.push_back({ asm_command::type::push,
            operation_type_ == tokenizer::token::sub_types::mod
            ? asm_reg::reg_type::edx
            : reg1 });
    if (com_type == asm_command::type::shl ||
        com_type == asm_command::type::shr)
        code.push_back({ asm_command::type::mov, asm_reg::reg_type::ecx, asm_reg::reg_type::ebx });
}

void operation_node::to_asm_compare(asm_code& code) const {
    left_->to_asm_code(code);
    right_->to_asm_code(code);
    const auto t = std::dynamic_pointer_cast<typed>(left_)->type();
    asm_command::type comm;
    switch(t->category()) { 
    case type::type_category::character: 
        code.push_back({ asm_command::type::movsx, asm_reg::reg_type::ebx, {asm_reg::reg_type::esp, asm_mem::mem_size::byte} });
        code.push_back({ asm_command::type::movsx, asm_reg::reg_type::eax, { asm_reg::reg_type::esp, asm_mem::mem_size::byte, 1 } });
        code.push_back({ asm_command::type::sub, asm_reg::reg_type::esp, {"2"} });
        code.push_back({ asm_command::type::cmp, asm_reg::reg_type::eax, asm_reg::reg_type::ebx });
        comm = ops.at(operation_type_);
        break;
    case type::type_category::integer: 
        code.push_back({ asm_command::type::pop, asm_reg::reg_type::ebx });
        code.push_back({ asm_command::type::cmp, {asm_reg::reg_type::esp, asm_mem::mem_size::dword},  asm_reg::reg_type::ebx });
        comm = ops.at(operation_type_);
        break;
    case type::type_category::real:
    {
        const auto is_equ = operation_type_ == tokenizer::token::sub_types::equal || operation_type_ == tokenizer::token::sub_types::not_equal;
        const auto comp_com = is_equ
            ? asm_command::type::ucomisd
            : asm_command::type::comisd;
        code.push_back({ asm_command::type::movsd, asm_reg::reg_type::xmm0,{ asm_reg::reg_type::esp, asm_mem::mem_size::qword, 8 } });
        code.push_back({ asm_command::type::movsd, asm_reg::reg_type::xmm1,{ asm_reg::reg_type::esp, asm_mem::mem_size::qword } });
        code.push_back({ asm_command::type::add, asm_reg::reg_type::esp,{ "12" } });
        code.push_back({ comp_com, asm_reg::reg_type::xmm0,asm_reg::reg_type::xmm1 });
        comm = f_ops.at(operation_type_);
        if (!is_equ)
            break;
        const auto label = code.get_label_name(position().first, position().second, "CONDFAIL");
        const auto end_l = code.get_label_name(position().first, position().second, "ENDCOND");
        code.push_back(asm_command::type::lahf);
        code.push_back({ asm_command::type::test, asm_reg::reg_type::ah,{ "68" } });
        code.push_back({ comm,{ label } });
        code.push_back({ asm_command::type::mov,{ asm_reg::reg_type::esp, asm_mem::mem_size::dword},{ "-1" } });
        code.push_back({ asm_command::type::jmp,{ end_l } });
        code.push_back({ asm_command::type::label,{ label } });
        code.push_back({ asm_command::type::mov,{ asm_reg::reg_type::esp, asm_mem::mem_size::dword},{ "0" } });
        code.push_back({ asm_command::type::label,{ end_l } });
        return;
    }
    default: 
        throw std::logic_error("This point should be unreachable");
    }
    code.push_back({ comm, asm_reg::reg_type::al });
    code.push_back({ asm_command::type::sub, asm_reg::reg_type::al, {"1"} });
    code.push_back({ asm_command::type::movsx, asm_reg::reg_type::eax, asm_reg::reg_type::al });
    code.push_back({ asm_command::type::mov,{ asm_reg::reg_type::esp, asm_mem::mem_size::dword }, asm_reg::reg_type::eax });
}

bool operation_node::is_assign() const {
    return operation_type_ == tokenizer::token::sub_types::plus_assign   ||
           operation_type_ == tokenizer::token::sub_types::minus_assign  ||
           operation_type_ == tokenizer::token::sub_types::mult_assign   ||
           operation_type_ == tokenizer::token::sub_types::divide_assign ||
           operation_type_ == tokenizer::token::sub_types::assign;
}

const tree_node_p& applied::variable() const { return variable_; }

void call_node::to_asm_code(asm_code& code, bool is_left) {
    const auto func = std::dynamic_pointer_cast<function_type>(std::dynamic_pointer_cast<typed>(variable())->type());
    size_t i = 0;
    for (; i < children()[1]->children().size(); ++i) {
        const auto t = func->parameters().vector()[i].second.first;
        if (t->category() == type::type_category::modified)
            children()[1]->children()[i]->to_asm_code(code,
                std::dynamic_pointer_cast<modified_type>(t)->modificator() == modified_type::modificator_type::var);
        else
            children()[1]->children()[i]->to_asm_code(code);
    }
    for (; i < func->parameters().size(); ++i)
        func->parameters().vector()[i].second.second->to_asm_code(code);
    if (func->parameters().get_data_size() % 4 != 0)
        code.push_back({ asm_command::type::sub, asm_reg::reg_type::esp, func->parameters().get_data_size() % 4 });
    code.push_back({ asm_command::type::call, code.get_function_label(variable()->name()) });
    switch (func->return_type()->category()) {
    case type::type_category::character:
        code.push_back({ asm_command::type::sub, asm_reg::reg_type::esp, 1 });
        code.push_back({ asm_command::type::mov,{ asm_reg::reg_type::esp, asm_mem::mem_size::byte }, asm_reg::reg_type::al });
        return;
    case type::type_category::integer: 
        code.push_back({ asm_command::type::push, asm_reg::reg_type::eax });
        return;
    case type::type_category::real: 
        code.push_back({ asm_command::type::sub, asm_reg::reg_type::esp, 8 });
        code.push_back({ asm_command::type::movsd, {asm_reg::reg_type::esp, asm_mem::mem_size::qword}, asm_reg::reg_type::xmm0 });
        return;
    case type::type_category::array:
    case type::type_category::record:
        code.push_back({ asm_command::type::push, {"offset", code.get_temp_var_name()} });
        return;
    case type::type_category::nil:
        return;
    default: 
        throw std::logic_error("This point should never be reached");
    }

}

void tree::put_value_on_stack(asm_code& code, const type_p type, const tree_node::position_type position) {
    code.push_back({ asm_command::type::pop, asm_reg::reg_type::eax });
    const auto t = type->category() == type::type_category::modified
        ? std::dynamic_pointer_cast<modified_type>(type)->base_type()
        : type;
    switch (t->category()) {
    case type::type_category::character:
        code.push_back({ asm_command::type::mov, asm_reg::reg_type::al,{ asm_reg::reg_type::eax, asm_mem::mem_size::byte } });
        code.push_back({ asm_command::type::sub, asm_reg::reg_type::esp,{ "1" } });
        code.push_back({ asm_command::type::mov,{ asm_reg::reg_type::esp, asm_mem::mem_size::byte },asm_reg::reg_type::al });
        return;
    case type::type_category::integer:
        code.push_back({ asm_command::type::push,{ asm_reg::reg_type::eax, asm_mem::mem_size::dword } });
        return;
    case type::type_category::real:
        code.push_back({ asm_command::type::push,{ asm_reg::reg_type::eax, asm_mem::mem_size::dword, 4 } });
        code.push_back({ asm_command::type::push,{ asm_reg::reg_type::eax, asm_mem::mem_size::dword } });
        return;
    case type::type_category::record:
    case type::type_category::array:
    {
        code.push_back({ asm_command::type::add, asm_reg::reg_type::eax, t->data_size() - 4 });
        code.push_back({ asm_command::type::mov, asm_reg::reg_type::ecx, t->data_size() / 4 });
        const auto label = code.get_label_name(position.first, position.second, "COPYSTRUCT");
        code.push_back({ asm_command::type::label, label });
        code.push_back({ asm_command::type::push,{ asm_reg::reg_type::eax, asm_mem::mem_size::dword } });
        code.push_back({ asm_command::type::sub, asm_reg::reg_type::eax, 4 });
        code.push_back({ asm_command::type::loop, label });
        return;
    }
    default:
        throw std::logic_error("This point should never be reached");
    }    
}

//class index_node
const tree_node_p& index_node::index() const { return index_; }

void index_node::to_asm_code(asm_code& code, const bool is_left) {
    index_->to_asm_code(code, false);
    variable()->to_asm_code(code, true);
    code.push_back({ asm_command::type::pop, asm_reg::reg_type::ecx });
    code.push_back({ asm_command::type::pop, asm_reg::reg_type::eax });
    const auto t = base_type(std::dynamic_pointer_cast<typed>(variable())->type());
    const auto min = std::dynamic_pointer_cast<array_type>(t)->min();
    if (min != 0)
        code.push_back({ asm_command::type::sub, asm_reg::reg_type::eax, min });
    code.push_back({ asm_command::type::mov, asm_reg::reg_type::ebx, type()->data_size() });
    code.push_back({ asm_command::type::imul, asm_reg::reg_type::ebx });
    code.push_back({ asm_command::type::add, asm_reg::reg_type::ecx, asm_reg::reg_type::eax });
    code.push_back({ asm_command::type::push, asm_reg::reg_type::ecx });
    if (is_left) return;
    put_value_on_stack(code, type(), position());
}

//class field_access_node
const variable_node_p& field_access_node::field() const { return field_; }

void field_access_node::to_asm_code(asm_code& code, const bool is_left) {
    variable()->to_asm_code(code, true);
    const auto t = base_type(std::dynamic_pointer_cast<typed>(variable())->type());
    const auto offset = std::dynamic_pointer_cast<record_type>(t)->get_field_offset(field_->name());
    if (offset != 0)
        code.push_back({ asm_command::type::add,{ asm_reg::reg_type::esp, asm_mem::mem_size::dword }, offset });
    if (is_left) return;
    put_value_on_stack(code, type(), position());
}

//class cast_node
cast_node::cast_node(const type_p& type, const tree_node_p& node, const position_type& position) : 
    tree_node(type->name(), node_category::cast, position, node), typed(type), applied(node) {
    const auto tl = base_type(type), tr = base_type(get_type(node));
    if (!tl->is_scalar() || !tr->is_scalar())
        throw convertion_error(tl, tr);
}

void cast_node::to_asm_code(asm_code& code, const bool is_left) {
    children()[0]->to_asm_code(code);
    const auto t = base_type(std::dynamic_pointer_cast<typed>(children()[0])->type());
    const auto result_type = type();
    switch (result_type->category()) { 
    case type::type_category::character:
        switch (t->category()) { 
        case type::type_category::character: 
            return;
        case type::type_category::integer: 
            code.push_back({ asm_command::type::pop, asm_reg::reg_type::eax });
            code.push_back({ asm_command::type::sub, asm_reg::reg_type::esp,{ "1" } });
            code.push_back({ asm_command::type::mov, {asm_reg::reg_type::esp, asm_mem::mem_size::byte}, asm_reg::reg_type::al});
            return;
        case type::type_category::real: 
            code.push_back({ asm_command::type::cvttsd2si, asm_reg::reg_type::eax, {asm_reg::reg_type::esp, asm_mem::mem_size::qword} });
            code.push_back({ asm_command::type::add, asm_reg::reg_type::esp,{ "7" } });
            code.push_back({ asm_command::type::mov, {asm_reg::reg_type::esp, asm_mem::mem_size::byte}, asm_reg::reg_type::al });
            return;
        default: 
            throw std::logic_error("This point should be unreachable");
        }
    case type::type_category::integer:
        switch (t->category()) { 
        case type::type_category::character: 
            code.push_back({ asm_command::type::movsx, asm_reg::reg_type::eax,{ asm_reg::reg_type::esp, asm_mem::mem_size::byte } });
            code.push_back({ asm_command::type::sub, asm_reg::reg_type::esp,{ "3" } });
            code.push_back({ asm_command::type::mov, {asm_reg::reg_type::esp, asm_mem::mem_size::dword}, asm_reg::reg_type::eax });
            return;
        case type::type_category::integer: 
            return;
        case type::type_category::real: 
            code.push_back({ asm_command::type::cvttsd2si, asm_reg::reg_type::eax,{ asm_reg::reg_type::esp, asm_mem::mem_size::qword } });
            code.push_back({ asm_command::type::add, asm_reg::reg_type::esp,{ "4" } });
            code.push_back({ asm_command::type::mov,{ asm_reg::reg_type::esp, asm_mem::mem_size::dword }, asm_reg::reg_type::eax });
            break;
        default: 
            throw std::logic_error("This point should be unreachable");
        }
        break;
    case type::type_category::real:
        switch (t->category()) { 
        case type::type_category::character: 
            code.push_back({ asm_command::type::movsx, asm_reg::reg_type::eax,{ asm_reg::reg_type::esp, asm_mem::mem_size::byte } });
            code.push_back({ asm_command::type::sub, asm_reg::reg_type::esp,{ "7" } });
            goto end;
        case type::type_category::integer: 
            code.push_back({ asm_command::type::mov, asm_reg::reg_type::eax,{ asm_reg::reg_type::esp, asm_mem::mem_size::dword } });
            code.push_back({ asm_command::type::sub, asm_reg::reg_type::esp,{ "4" } });
            goto end;
        case type::type_category::real: 
            return;
        default: 
            throw std::logic_error("This point should be unreachable");
        }
        end:
        code.push_back({ asm_command::type::cvtsi2sd, asm_reg::reg_type::xmm0, asm_reg::reg_type::eax });
        code.push_back({ asm_command::type::movsd, {asm_reg::reg_type::esp, asm_mem::mem_size::qword}, asm_reg::reg_type::xmm0 });
        return;
    default: 
        throw std::logic_error("This point should be unreachable");;
    }
}

void write_node::to_asm_code(asm_code& code, const bool is_left) {
    std::string f = "";
    long long size = 0;
    for (auto it = children().rbegin(); it != children().rend(); ++it) {
        auto type = std::dynamic_pointer_cast<typed>(*it)->type();
        type = type->category() == type::type_category::modified
            ? std::dynamic_pointer_cast<modified_type>(type)->base_type()
            : type;
        switch (type->category()) {
        case type::type_category::character:
            f += "c%";
            (*it)->to_asm_code(code);
            code.push_back({ asm_command::type::movsx, asm_reg::reg_type::eax, {asm_reg::reg_type::esp, asm_mem::mem_size::byte} });
            code.push_back({ asm_command::type::add, asm_reg::reg_type::esp, 1 });
            code.push_back({ asm_command::type::push, asm_reg::reg_type::eax });
            size += 4;
            break;
        case type::type_category::integer:
            f += "d%";
            (*it)->to_asm_code(code);
            size += 4;
            break;
        case type::type_category::real:
            f += "f%";
            (*it)->to_asm_code(code);
            size += 8;
            break;
        case type::type_category::string:
        {
            f += "s%";
            const auto s = code.add_string_constant(std::dynamic_pointer_cast<constant_node>(*it)->value_string());
            code.push_back({ asm_command::type::push, {"offset", s}});
            size += 4;
            break;
        }
        default:
            throw std::logic_error("This point should never be reached");
        }
    }
    reverse(f.begin(), f.end());
    const auto s = code.add_string_constant(f + '\n');
    code.push_back({ asm_command::type::push,{ "offset", s } });
    code.push_back({ asm_command::type::call, {"crt_printf"} });
    code.push_back({ asm_command::type::add, asm_reg::reg_type::esp, size + 4 });
}

void repeat_node::set_condition(const tree_node_p condition) { push_back(condition); }

void repeat_node::to_asm_code(asm_code& code, const bool is_left) {
    const auto body_label = code.get_label_name(position().first, position().second, "REPEATBODY");
    const auto cond_label = code.get_label_name(position().first, position().second, "REPEATCOND");
    const auto end_label = code.get_label_name(position().first, position().second, "REPEATEND");
    code.add_loop_start(cond_label);
    code.add_loop_end(end_label);
    code.push_back({ asm_command::type::label, body_label });
    children()[0]->to_asm_code(code);
    code.push_back({ asm_command::type::label, cond_label });
    children()[1]->to_asm_code(code);
    code.push_back({ asm_command::type::pop, asm_reg::reg_type::eax });
    code.push_back({ asm_command::type::test, asm_reg::reg_type::eax, -1 });
    code.push_back({ asm_command::type::jz, body_label });
    code.push_back({ asm_command::type::label, end_label });
    code.pop_loop_start();
    code.pop_loop_end();
}

void for_node::set_downto(const bool value) {
    is_downto_ = value;
}

void for_node::to_asm_code(asm_code& code, bool is_left) {
    const auto body_label = code.get_label_name(position().first, position().second, "LOOPBODY");
    const auto cond_label = code.get_label_name(position().first, position().second, "LOOPCOND");
    const auto end_label = code.get_label_name(position().first, position().second, "LOOPEND");
    code.add_loop_start(cond_label);
    code.add_loop_end(end_label);
    children()[2]->to_asm_code(code);
    children()[1]->to_asm_code(code);
    const auto offset = code.get_offset(children()[0]->name());
    code.push_back({ asm_command::type::pop, asm_reg::reg_type::eax });
    code.push_back({ asm_command::type::mov, asm_reg::reg_type::ebx, { asm_reg::reg_type::ebp, asm_mem::mem_size::dword, offset.first } });
    code.push_back({ asm_command::type::mov, {asm_reg::reg_type::ebx, asm_mem::mem_size::dword, -offset.second}, asm_reg::reg_type::eax });
    code.push_back({ is_downto_ ? asm_command::type::inc : asm_command::type::dec,{ asm_reg::reg_type::ebx, asm_mem::mem_size::dword, -offset.second } });
    code.push_back({ asm_command::type::jmp, cond_label });
    code.push_back({ asm_command::type::label, body_label });
    children()[3]->to_asm_code(code);
    code.push_back({ asm_command::type::label, cond_label });
    code.push_back({ asm_command::type::mov, asm_reg::reg_type::ebx,{ asm_reg::reg_type::ebp, asm_mem::mem_size::dword, offset.first } });
    code.push_back({ is_downto_ ? asm_command::type::dec : asm_command::type::inc,{ asm_reg::reg_type::ebx, asm_mem::mem_size::dword, -offset.second } });
    code.push_back({ asm_command::type::mov, asm_reg::reg_type::eax, {asm_reg::reg_type::esp, asm_mem::mem_size::dword} });
    code.push_back({ asm_command::type::cmp, {asm_reg::reg_type::ebx, asm_mem::mem_size::dword, -offset.second }, asm_reg::reg_type::eax });
    code.push_back({ is_downto_ ? asm_command::type::jge : asm_command::type::jle, body_label });
    code.push_back({ asm_command::type::label, end_label });
    code.push_back({ asm_command::type::add, asm_reg::reg_type::esp, 4 });
    code.pop_loop_start();
    code.pop_loop_end();
}

void while_node::to_asm_code(asm_code& code, bool is_left) {
    const auto body_label = code.get_label_name(position().first, position().second, "WHILEBODY");
    const auto cond_label = code.get_label_name(position().first, position().second, "WHILECOND");
    const auto end_label = code.get_label_name(position().first, position().second, "WHILEEND");
    code.add_loop_start(cond_label);
    code.add_loop_end(end_label);
    code.push_back({ asm_command::type::jmp, cond_label });
    code.push_back({ asm_command::type::label, body_label });
    children()[1]->to_asm_code(code);
    code.push_back({ asm_command::type::label, cond_label });
    children()[0]->to_asm_code(code);
    code.push_back({ asm_command::type::pop, asm_reg::reg_type::eax });
    code.push_back({ asm_command::type::test, asm_reg::reg_type::eax, -1 });
    code.push_back({ asm_command::type::jnz, body_label });
    code.push_back({ asm_command::type::label, end_label });
    code.pop_loop_start();
    code.pop_loop_end();
}

void break_node::to_asm_code(asm_code& code, bool is_left) {
    code.push_break();
}

void continue_node::to_asm_code(asm_code& code, bool is_left) {
    code.push_continue();
}

void if_node::to_asm_code(asm_code& code, bool is_left) {
    const auto else_label = code.get_label_name(position().first, position().second, "IFFAIL");
    const auto end_label = code.get_label_name(position().first, position().second, "IFEND");
    children()[0]->to_asm_code(code);
    code.push_back({ asm_command::type::pop, asm_reg::reg_type::eax });
    code.push_back({ asm_command::type::test, asm_reg::reg_type::eax, -1 });
    code.push_back({ asm_command::type::jz, else_label });
    children()[1]->to_asm_code(code);
    code.push_back({ asm_command::type::jmp, end_label });
    code.push_back({ asm_command::type::label, else_label });
    if (children().size() == 3)
        children()[2]->to_asm_code(code);
    code.push_back({ asm_command::type::label, end_label });
}

void exit_node::to_asm_code(asm_code& code, bool is_left) {
    const auto f = code.get_current_function_result_type();
    if (children().size()) {
        children()[0]->to_asm_code(code, !f->is_scalar());
        switch (f->category()) {
        case type::type_category::character:
            code.push_back({ asm_command::type::mov, asm_reg::reg_type::al,{ asm_reg::reg_type::esp, asm_mem::mem_size::byte} });
            code.push_back({ asm_command::type::add, asm_reg::reg_type::esp, 1 });
            break;
        case type::type_category::integer:
            code.push_back({ asm_command::type::pop, asm_reg::reg_type::eax });
            break;
        case type::type_category::real:
            code.push_back({ asm_command::type::movsd, asm_reg::reg_type::xmm0,{ asm_reg::reg_type::esp, asm_mem::mem_size::qword } });
            break;
        case type::type_category::nil:
            break;
        case type::type_category::array:
        case type::type_category::record:
        {
            code.push_back({ asm_command::type::lea, asm_reg::reg_type::ebx,code.get_temp_var_name() });
            const auto offset = code.get_offset("result");
            code.push_back({ asm_command::type::pop, asm_reg::reg_type::eax });
            code.push_back({ asm_command::type::mov, asm_reg::reg_type::ecx, f->data_size() / 4 });
            const auto label = code.get_label_name(position().first, position().second, "COPYSTRUCT");
            code.push_back({ asm_command::type::label, label });
            code.push_back({ asm_command::type::mov,asm_reg::reg_type::edx,{ asm_reg::reg_type::eax, asm_mem::mem_size::dword } });
            code.push_back({ asm_command::type::mov,{ asm_reg::reg_type::ebx, asm_mem::mem_size::dword }, asm_reg::reg_type::edx });
            code.push_back({ asm_command::type::add, asm_reg::reg_type::eax, 4 });
            code.push_back({ asm_command::type::add, asm_reg::reg_type::ebx, 4 });
            code.push_back({ asm_command::type::loop, label });
            break;
        }
        default:
            throw std::logic_error("This point should never be reached!");
        }
    }
    else {
        const auto offset = code.get_offset("result");
        switch (f->category()) {
        case type::type_category::character:
            code.push_back({ asm_command::type::mov, asm_reg::reg_type::al,{ asm_reg::reg_type::ebp, asm_mem::mem_size::byte, -offset.second } });
            break;
        case type::type_category::integer:
            code.push_back({ asm_command::type::mov, asm_reg::reg_type::eax,{ asm_reg::reg_type::ebp, asm_mem::mem_size::dword, -offset.second } });
            break;
        case type::type_category::real:
            code.push_back({ asm_command::type::movsd, asm_reg::reg_type::xmm0,{ asm_reg::reg_type::ebp, asm_mem::mem_size::qword, -offset.second } });
            break;
        case type::type_category::nil:
            break;
        case type::type_category::array:
        case type::type_category::record:
        {
            code.push_back({ asm_command::type::lea, asm_reg::reg_type::ebx,code.get_temp_var_name() });
            code.push_back({ asm_command::type::lea, asm_reg::reg_type::eax,{ asm_reg::reg_type::ebp, asm_mem::mem_size::dword, -offset.second } });
            code.push_back({ asm_command::type::mov, asm_reg::reg_type::ecx, f->data_size() / 4 });
            const auto label = code.get_label_name(position().first, position().second, "COPYSTRUCT");
            code.push_back({ asm_command::type::label, label });
            code.push_back({ asm_command::type::mov,asm_reg::reg_type::edx,{ asm_reg::reg_type::eax, asm_mem::mem_size::dword } });
            code.push_back({ asm_command::type::mov,{ asm_reg::reg_type::ebx, asm_mem::mem_size::dword }, asm_reg::reg_type::edx });
            code.push_back({ asm_command::type::add, asm_reg::reg_type::eax, 4 });
            code.push_back({ asm_command::type::add, asm_reg::reg_type::ebx, 4 });
            code.push_back({ asm_command::type::loop, label });
            break;
        }
        default:
            throw std::logic_error("This point should never be reached!");
        }
    }
    code.push_back({ asm_command::type::leave });
    code.push_back({ asm_command::type::ret, code.get_current_function_param_size() });
}
