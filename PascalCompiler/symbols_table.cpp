#include "symbols_table.hpp"
#include "boost/format.hpp"
#include "type.hpp"
#include "tree.hpp"
#include "asm_code.hpp"

using namespace pascal_compiler::syntax_analyzer;

// class duplicate_symbol_exception
duplicate_symbol_exception::duplicate_symbol_exception(const std::string& symbol) {
    message_ = str(boost::format("Duplicate identifier \"%1%\"") % symbol).c_str();
}

// class symbols_table
void symbols_table::add(const std::string& name, const type_p type, const tree_node_p& value) {
    if (table_.find(name) != table_.end())
        throw duplicate_symbol_exception(name);
    table_[name] = make_pair(type, value);
    vector_.push_back(make_pair(name, table_[name]));
}

const type_p& symbols_table::get_type(const std::string& name) const { return table_.at(name).first; }

const type_p& symbols_table::get_type(const size_t index) const { return vector_.at(index).second.first; }

const tree_node_p& symbols_table::get_value(const std::string& name) const { return table_.at(name).second; }

const symbols_table::symbol_t& symbols_table::operator[](const std::string& name) const { return table_.at(name); }

const symbols_table::symbol_t& symbols_table::operator[](const size_t index) const { return vector_.at(index).second; }

void symbols_table::change_last(const symbol_t& symbol) {
    table_[vector_.back().first] = symbol;
    vector_.back().second = symbol;
}

void symbols_table::change(const std::string& name, const symbol_t& symbol) {
    table_[name] = symbol;
    for (auto& it : vector_)
        if (it.first == name) {
            it.second = symbol;
            return;
        }
}

size_t symbols_table::size() const { return vector_.size(); }

const symbols_table::table_t& symbols_table::table() const { return table_; }

const symbols_table::vector_t& symbols_table::vector() const { return vector_; }

std::string symbols_table::to_string(const std::string& prefix) const {
    std::string result = "";
    for (const auto it : vector_) {
        result += str(boost::format("%3%%1%: %2%\n") % it.first % 
            it.second.first->to_string(prefix + std::string(it.first.size(), ' ')) % prefix);
        if (it.second.second) {
            result += '\n';
            result += it.second.second->to_string(prefix + std::string(it.first.size(), ' '));
        }
    }
    result.pop_back();
    return result;
}

void symbols_table::to_asm_code(asm_code& code) const {
    for (auto it : vector_)
        if (it.second.first->category() == type::type_category::function) {
            const auto f = std::dynamic_pointer_cast<function_type>(it.second.first);
            code.start_function(it.first, it.second.second->position().first, it.second.second->position().second,
                f->table(), f->parameters());
            f->table().to_asm_code(code);
            it.second.second->to_asm_code(code);
            code.end_function();
        }
}

void symbols_table::calculate_offsets() {
    long long offset = 0;
    for (const auto it : vector_)
        offsets_[it.first] = offset += it.second.first->data_size();
    size_ = offset;
}

long long symbols_table::get_offset(const std::string& name) const {
    return offsets_.at(name);
}

long long symbols_table::get_data_size() const {
    return size_;
}
