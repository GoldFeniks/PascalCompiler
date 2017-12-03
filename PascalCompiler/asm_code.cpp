#include "asm_code.hpp"
#include "tree.hpp"
#include "boost/format.hpp"
#include "type.hpp"

using namespace pascal_compiler;
using namespace code;

const std::string asm_reg::reg_type_str[] = { "eax", "ebx", "ecx", "edx", "xmm0", "xmm1", "esp", "ebp", "al", "cl", "ah", "bl", "ax" };
const std::string asm_mem::mem_size_str[] = { "byte", "word", "dword", "qword" };
const std::string asm_command::type_str[] = { 
    "mov", "push", "pop", "add", "sub", "imul", "idiv", "printf", "movsd", 
    "and", "or", "xor", "mulsd", "addsd", "divsd", "subsd", "neg", "pxor", 
    "not", "cdq", "movsx", "shl", "shr", "cvtsi2sd", "cvttsd2si",
    "setge", "setg", "setle", "setl", "sete", "setne", "cmp", "jmp", "", 
    "comisd", "ucomisd", "setbe", "setb", "seta", "setae", "jp", "jnp", "lahf", "test",
    "loop", "jnz", "jz", "inc", "dec", "jge", "jle", "call", "lea"
};

asm_arg::type asm_arg::get_type() const {
    return type_;
}

asm_mem::mem_size asm_mem::get_mem_size() const {
    return size_;
}

std::string asm_mem::to_string() const {
    return str(boost::format("%1% ptr [ebp - %2%]")
        % mem_size_str[static_cast<unsigned char>(size_)]
        % offset_);
}

std::string asm_code::add_double_constant(const double value) {
    const std::unordered_map<double, std::string>::const_iterator it = double_const_.find(value);
    if (it != double_const_.end())
        return std::string("__real@") + it->second;
    std::string result;
    auto mask = uint64_t(15) << 60;
    const auto val = *reinterpret_cast<const uint64_t*>(&value);
    for (auto i = 0; i < sizeof(value) * 2; ++i) {
        const unsigned char c = (val & mask) >> 60 - i * 4;
        result += c + (c < 10 ? 48 : 87);
        mask >>= 4;
    }
    if (result[0] >= 97)
        result.insert(result.begin(), '0');
    double_const_[value] = result;
    return std::string("__real@") + result;
}

std::string asm_code::add_string_constant(const std::string& value) {
    const std::unordered_map<std::string, size_t>::const_iterator it = string_const_.find(value);
    if (it != string_const_.end())
        return std::string("__string@") + std::to_string(it->second);
    string_const_[value] = strings_++;
    return std::string("__string@") + std::to_string(string_const_[value]);
}

std::string asm_code::get_label_name(const size_t row, const size_t col, const std::string& suffix) {
    return str(boost::format("$LN%1%AT%2%%3%@") % row % col % suffix);
}

void asm_code::add_loop_start(const std::string& value) {
    loop_starts_.push(value);
}

void asm_code::add_loop_end(const std::string& value) {
    loop_ends_.push(value);
}

void asm_code::pop_loop_start() {
    loop_starts_.pop();
}

void asm_code::pop_loop_end() {
    loop_ends_.pop();
}

void asm_code::push_break() {
    push_back({ asm_command::type::jmp, loop_ends_.top() });
}

void asm_code::push_continue() {
    push_back({ asm_command::type::jmp, loop_starts_.top() });  
}

void asm_code::start_function(const std::string& name, const size_t row, const size_t col, const symbols_table& data_table, const symbols_table& param_table) {
    if (commands_.size() == 0)
        main_func_name_ = wrap_function_name(name, row, col);
    data_tables_.push_back(data_table);
    param_tables_.push_back(param_table);
    commands_.emplace_back(wrap_function_name(name, row, col), std::vector<asm_command>());
}

void asm_code::end_function() {
    func_string_ += commands_.back().first + ":\n";
    func_string_ += str(boost::format("enter %1%, %2%\n") % data_tables_.back().get_data_size() % commands_.size());
    for (const auto com : commands_.back().second)
        func_string_ += com.to_string() + '\n';
    func_string_ += "leave\n";
    func_string_ += str(boost::format("ret %1%\n\n") % param_tables_.back().get_data_size());
    data_tables_.pop_back();
    param_tables_.pop_back();
    commands_.pop_back();
}

std::string asm_code::get_function_label(const std::string& name) const {
    for (auto i = commands_.size() - 1; i >= 0; --i) {
        const auto val = data_tables_[i].table().find(name);
        if (val != data_tables_[i].table().end())
            return wrap_function_name(name, val->second.second->position().first, val->second.second->position().second);
    }
    throw std::logic_error("This point should never be reached");
}

std::string asm_code::wrap_function_name(const std::string& name, const size_t row, const size_t col) {
    return str(boost::format("__function@LN%1%AT%2%%3%") % row % col % name);
}

asm_reg::reg_type asm_reg::get_reg_type() const {
    return reg_;
}

std::string asm_reg::to_string() const {
    if (typed_) {
        if (offset_ != 0)
            return str(boost::format("%1% ptr [%2% %4% %3%]")
                % asm_mem::mem_size_str[static_cast<unsigned char>(size_)]
                % reg_type_str[static_cast<unsigned char>(reg_)]
                % abs(offset_) % (offset_ > 0 ? '+' : '-')
            );
        return str(boost::format("%1% ptr [%2%]")
            % asm_mem::mem_size_str[static_cast<unsigned char>(size_)]
            % reg_type_str[static_cast<unsigned char>(reg_)]
        );
    }
    return reg_type_str[static_cast<unsigned char>(reg_)];
}

std::string asm_imm::to_string() const {
    if (offset_ != -1) {
        if (offset_ > 0)
            return str(boost::format("%1% ptr [%2% + %3%]")
                % asm_mem::mem_size_str[static_cast<unsigned char>(size_)]
                % value_ % offset_);
        return str(boost::format("%1% ptr [%2%]")
            % asm_mem::mem_size_str[static_cast<unsigned char>(size_)]
            % value_);
    }
    return value_;
}

asm_command::asm_command(const type type, const asm_mem& arg1, const asm_mem& arg2) : type_(type) {
    add(std::make_shared<asm_mem>(arg1), std::make_shared<asm_mem>(arg2));
}

asm_command::asm_command(const type type, const asm_reg& arg1, const asm_mem& arg2) : type_(type) {
    add(std::make_shared<asm_reg>(arg1), std::make_shared<asm_mem>(arg2));
}

asm_command::asm_command(const type type, const asm_mem& arg1, const asm_reg& arg2) : type_(type) {
    add(std::make_shared<asm_mem>(arg1), std::make_shared<asm_reg>(arg2));
}

asm_command::asm_command(const type type, const asm_reg& arg1, const asm_reg& arg2) : type_(type) {
    add(std::make_shared<asm_reg>(arg1), std::make_shared<asm_reg>(arg2));
}

asm_command::asm_command(const type type, const asm_mem& arg1, const asm_imm& arg2) : type_(type) {
    add(std::make_shared<asm_mem>(arg1), std::make_shared<asm_imm>(arg2));
}

asm_command::asm_command(const type type, const asm_reg& arg1, const asm_imm& arg2) : type_(type) {
    add(std::make_shared<asm_reg>(arg1), std::make_shared<asm_imm>(arg2));
}

asm_command::asm_command(const type type, const asm_mem& arg) : type_(type) {
    add(std::make_shared<asm_mem>(arg));
}

asm_command::asm_command(const type type, const asm_reg& arg) : type_(type) {
    add(std::make_shared<asm_reg>(arg));
}

asm_command::asm_command(const type type, const asm_imm& arg) : type_(type) {
    add(std::make_shared<asm_imm>(arg));
}

asm_command::asm_command(const type type, const std::vector<std::shared_ptr<asm_arg>>& args) : type_(type) {
    for (const auto& it : args)
        args_.push_back(it);
}

std::string asm_command::to_string() const {
    if (type_ == type::label)
        return args_[0]->to_string() + ':';
    auto result = type_str[static_cast<unsigned char>(type_)]
        + (type_ == type::printf ? "(" : " ");
    if (args_.size() > 0) {
        result += args_[0]->to_string();
        for (auto i = 1; i < args_.size(); ++i)
            result += std::string(", ") + args_[i]->to_string();
    }
    return type_ == type::printf ? result + ")" : result;
}

void asm_command::add(const std::shared_ptr<asm_arg> arg1, const std::shared_ptr<asm_arg> arg2) {
    args_.push_back(arg1);
    args_.push_back(arg2);
}

void asm_command::add(const std::shared_ptr<asm_arg> arg1) { args_.push_back(arg1); }

void asm_code::push_back(const asm_command& command) {
    commands_.back().second.push_back(command);
}

void asm_code::push_back(asm_command&& command) {
    commands_.back().second.push_back(std::move(command));
}

std::string asm_code::to_string() const {
    std::string result = "include c:\\masm32\\include\\masm32rt.inc\n.xmm\n.const\n";
    for (const auto& it : double_const_)
        result += str(boost::format("__real\@%1% dq %1%r ;%2%\n") % it.second % it.first);
    for (const auto& it : string_const_) {
        result += "__string@";
        result += std::to_string(it.second);
        result += " db ";
        for (const auto& c : it.first)
            result += std::to_string(int(c)) + ", ";
        result += "0\n";
    }
    result += ".code\n";
    result += func_string_;
    result += "start:\n";
    result += str(boost::format("call %1%\n") % main_func_name_);
    return result + "exit\nend start";    
}

std::pair<long long, long long> asm_code::get_offset(const std::string& name) const {
    for (auto i = commands_.size() - 1; i >= 0; --i) {
        auto val = data_tables_[i].table().find(name);
        if (val != data_tables_[i].table().end())
            return std::make_pair((i + 1) * -4, data_tables_[i].get_offset(name) + 4 * (i + 1));
        val = param_tables_[i].table().find(name);
        if (val != param_tables_[i].table().end())
            return std::make_pair((i + 1) * -4, +param_tables_[i].get_offset(name) - 8 - param_tables_[i].get_data_size());
    }
    throw std::logic_error("This point should never be reached");
}
