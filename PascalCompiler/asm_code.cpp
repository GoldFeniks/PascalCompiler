#include "asm_code.hpp"
#include "tree.hpp"
#include "boost/format.hpp"
#include "type.hpp"

using namespace pascal_compiler;
using namespace code;

const std::string asm_reg::reg_type_str[] = { "eax", "ebx", "ecx", "edx", "xmm0", "xmm1", "esp", "ebp" };
const std::string asm_mem::mem_size_str[] = { "byte", "word", "dword", "qword" };
const std::string asm_command::type_str[] = { 
    "mov", "push", "pop", "add", "sub", "mul", "div", "printf", "movsd", 
    "and", "or", "xor", "mulsd", "addsd", "divsd", "subsd", "neg", "pxor", "not", "cdq"
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
    double_const_[value] = result;
    return std::string("__real@") + result;
}

asm_reg::reg_type asm_reg::get_reg_type() const {
    return reg_;
}

std::string asm_reg::to_string() const {
    if (typed_) {
        if (offset_ > 0)
            return str(boost::format("%1% ptr [%2% + %3%]")
                % asm_mem::mem_size_str[static_cast<unsigned char>(size_)]
                % reg_type_str[static_cast<unsigned char>(reg_)]
                % offset_
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
    commands_.push_back(command);
}

void asm_code::push_back(asm_command&& command) {
    commands_.push_back(std::move(command));
}

void asm_code::add_data(const std::string& name, const symbols_table::symbol_t& symbol) {
    data_size_ += symbol.first->data_size();
    offsets_[name] = data_size_;
    if (symbol.second != nullptr) {
        symbol.second->to_asm_code(*this);
        switch (symbol.first->category()) { 
        case type::type_category::character:
            push_back({ asm_command::type::pop,{ asm_mem::mem_size::word, get_offset(name) } });
            break;
        case type::type_category::integer:
            push_back({ asm_command::type::pop,{ asm_mem::mem_size::dword, get_offset(name) } });
            break;
        case type::type_category::real:
            push_back({ asm_command::type::pop,{ asm_mem::mem_size::dword, get_offset(name) - 4 } });
            push_back({ asm_command::type::pop,{ asm_mem::mem_size::dword, get_offset(name) } });
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
}

std::string asm_code::to_string() const {
    std::string result = "include c:\\masm32\\include\\masm32rt.inc\n.xmm\n.const\n__neg@ dq 8000000000000000r\n";
    for (const auto& it : double_const_)
        result += str(boost::format("__real\@%1% dq %1%r ;%2%\n") % it.second % it.first);
    result += ".code\nstart:\n";
    result += str(boost::format("push ebp\nmov ebp, esp\nsub esp, %1%\n") % data_size_);
    for (const auto& it : commands_)
        result += it.to_string() + '\n';
    result += "mov esp, ebp\npop ebp\n";
    return result + "exit\nend start";    
}

size_t asm_code::get_offset(const std::string& name) const {
    return offsets_.at(name);
}
