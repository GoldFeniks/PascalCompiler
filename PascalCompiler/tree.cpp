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
        code.push_back({ asm_command::type::mov, asm_reg::reg_type::al,{ asm_mem::mem_size::byte, code.get_offset(name()) } });
        code.push_back({ asm_command::type::sub, asm_reg::reg_type::esp, {"1"} });
        code.push_back({ asm_command::type::mov,{asm_reg::reg_type::esp, asm_mem::mem_size::byte},asm_reg::reg_type::al });
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

void operation_node::to_asm_code(asm_code& code) {
    if (is_assign())
        to_asm_assign(code);
    else if (is_relational(operation_type_))
        to_asm_compare(code);
    else
        to_asm(code);
}

void operation_node::to_asm_assign(asm_code& code) const {
    right_->to_asm_code(code);
    asm_mem::mem_size mem_size;
    asm_command::type com_type;
    asm_reg::reg_type reg1;
    if (type() == real()) {
        reg1 = asm_reg::reg_type::xmm0;
        code.push_back({ asm_command::type::movsd, reg1,{ asm_reg::reg_type::esp, asm_mem::mem_size::qword } });
        com_type = f_ops.at(operation_type_);
        mem_size = asm_mem::mem_size::qword;
    }
    else {
        if (type() == integer()) {
            reg1 = asm_reg::reg_type::eax;
            code.push_back({ asm_command::type::pop, reg1 });
        }
        else {
            reg1 = asm_reg::reg_type::al;
            code.push_back({ asm_command::type::mov, reg1, {asm_reg::reg_type::esp, asm_mem::mem_size::byte} });
            code.push_back({ asm_command::type::add, asm_reg::reg_type::esp, {"1"} });
        }
        mem_size = type() == integer() ? asm_mem::mem_size::dword : asm_mem::mem_size::byte;
        com_type = ops.at(operation_type_);
    }
    if (com_type == asm_command::type::idiv)
        code.push_back({ asm_command::type::cdq });
    code.push_back({ com_type,{ mem_size, code.get_offset(left_->name()) }, reg1 });
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
        const auto label = str(boost::format("$LN%1%AT%2%F@") % position().first % position().second);
        const auto end_l = str(boost::format("$LN%1%AT%2%E@") % position().first % position().second);
        code.push_back(asm_command::type::lahf);
        code.push_back({ asm_command::type::test, asm_reg::reg_type::ah,{ "68" } });
        code.push_back({ comm,{ label } });
        code.push_back({ asm_command::type::mov,{ asm_reg::reg_type::esp, asm_mem::mem_size::dword},{ "-1" } });
        code.push_back({ asm_command::type::jmp,{ end_l } });
        code.push_back({ asm_command::type::label,{ label + ':' } });
        code.push_back({ asm_command::type::mov,{ asm_reg::reg_type::esp, asm_mem::mem_size::dword},{ "0" } });
        code.push_back({ asm_command::type::label,{ end_l + ':' } });
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

void cast_node::to_asm_code(asm_code& code) {
    children()[0]->to_asm_code(code);
    const auto t = std::dynamic_pointer_cast<typed>(children()[0])->type();
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

void write_node::to_asm_code(asm_code& code) {
    std::vector<std::shared_ptr<asm_arg>> args;
    std::string f = "";
    long offset = 0;
    for (const auto& it : children()) {
        const auto t = std::dynamic_pointer_cast<typed>(it)->type();
        offset += t->category() != type::type_category::character 
            ? std::dynamic_pointer_cast<typed>(it)->type()->data_size()
            : 2;
    }
    auto stack_size = offset;
    for (std::vector<tree_node_p>::const_reverse_iterator it = children().rbegin(); it != children().rend(); ++it) {
        const auto type = std::dynamic_pointer_cast<typed>(*it)->type();
        asm_mem::mem_size size;
        switch (type->category()) {
        case type::type_category::character:
            f += 'c';
            (*it)->to_asm_code(code);
            code.push_back({ asm_command::type::sub, asm_reg::reg_type::esp, {"1"} });
            code.push_back({ asm_command::type::movsx, asm_reg::reg_type::eax, {asm_reg::reg_type::esp, asm_mem::mem_size::byte, 1} });
            code.push_back({ asm_command::type::mov, {asm_reg::reg_type::esp, asm_mem::mem_size::word}, asm_reg::reg_type::ax });
            offset -= 2;
            args.push_back(std::make_shared<asm_reg>(asm_reg::reg_type::eax, asm_mem::mem_size::word, offset));
            continue;
        case type::type_category::integer:
            f += 'd';
            size = asm_mem::mem_size::dword;
            break;
        case type::type_category::real:
            f += 'f';
            size = asm_mem::mem_size::qword;
            break;
        case type::type_category::string:
            f += 's';
            args.push_back(std::make_shared<asm_imm>(std::string("\"") + (*it)->name() + "\""));
            continue;
        default:
            throw std::logic_error("This point should never be reached");
        }
        (*it)->to_asm_code(code);
        offset -= type->data_size();
        args.push_back(std::make_shared<asm_reg>(asm_reg::reg_type::eax, size, offset));
    }
    reverse(args.begin(), args.end());
    reverse(f.begin(), f.end());
    std::string format = "\"";
    for (const auto& it : f) {
        format += '%'; 
        format += it;
    }
    code.push_back({ asm_command::type::mov, asm_reg::reg_type::eax, asm_reg::reg_type::esp });
    args.insert(args.begin(), std::make_shared<asm_imm>( format + "\\n\"" ));
    code.push_back({ asm_command::type::printf, args });
    code.push_back({ asm_command::type::add, asm_reg::reg_type::esp, std::to_string(stack_size) });
}
