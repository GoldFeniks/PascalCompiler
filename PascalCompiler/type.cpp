#include "type.hpp"
#include "tree.hpp"
#include <boost/format.hpp>

using namespace pascal_compiler;
using namespace syntax_analyzer;
using namespace types;

const std::string type::type_strings[] = {
    "char",   "integer",  "real",     "type",   "array",   "nil",
    "record", "function", "modified", "pointer", "string"
};

//class type
std::string type::name() const { return is_anonymous_ ? "anonymous" : name_; }

type::type_category type::category() const { return category_; }

bool type::is_category(const type_category category) const { return category == category_; }

bool type::is_anonymous() const { return is_anonymous_; }

bool type::is_scalar() const {
    return category_ == type_category::integer ||
           category_ == type_category::real    ||
           category_ == type_category::character;        
}

size_t type::data_size() const {
    return size_;
}

std::string type::to_string(const std::string& prefix) const {
    return type_strings[static_cast<unsigned int>(category_)];
}

//class type_type
type_p type_type::alias_to() const { return alias_to_; }

std::string type_type::to_string(const std::string& prefix) const {
    return str(boost::format("type %1%") % alias_to_->to_string(prefix + "    "));    
}

//class array_type
size_t array_type::size() const { return max_ - min_ + 1; }

size_t array_type::min() const { return min_; }

size_t array_type::max() const { return max_; }

const type_p& array_type::element_type() const { return element_type_; }

std::string array_type::to_string(const std::string& prefix) const {
    const auto result = str(boost::format("array [%1%..%2%] of ") % min_ % max_);
    return result + element_type_->to_string(prefix + std::string(result.length(), ' '));
}

size_t array_type::data_size() const {
    const auto result = size() * element_type_->data_size();
    return result + result % 4;
}

//class record_type
void record_type::add_field(const std::string& name, const type_p& type) {
    fields_.add(name, type);
    size_ += type->data_size();
}

const symbols_table& record_type::fields() const { return fields_; }

std::string record_type::to_string(const std::string& prefix) const {
    auto result = str(boost::format("record:"));
    const auto n_prefix = std::string(result.length() - 4, ' ') + prefix;
    for (const auto it : fields_.table()) {
        result += '\n';
        result += str(boost::format("%1% %2%: %3%") %
            n_prefix % it.first % it.second.first->to_string(n_prefix));
    }
    return result;
}

size_t record_type::get_field_offset(const std::string name) const {
    size_t result = 0;
    for (const auto it : fields_.vector()) {
        if (it.first == name)
            return result;
        result += it.second.first->data_size();
    }
    throw std::logic_error("This point should never be reached");
}

size_t record_type::data_size() const {
    return size_ + size_ % 4;
}

//class function_type
const symbols_table& function_type::parameters() const { return parameters_; }

const symbols_table& function_type::table() const { return table_; }

const type_p& function_type::return_type() const { return return_type_; }

std::string function_type::to_string(const std::string& prefix) const {
    std::string result = "function (";
    for (const auto it : parameters_.vector()) {
        result += str(boost::format("%1%: %2%") % it.first %
            it.second.first->to_string());
        if (it.second.second)
            result += str(boost::format(" = %1%") % it.second.second->name());
        result += "; ";
    }
    result += ")\n";
    return result + table_.to_string(prefix + "    ");
}

//class modified_type
modified_type::modificator_type modified_type::modificator() const { return modificator_; }

std::string modified_type::to_string(const std::string& prefix) const {
    const std::string str = modificator_ == modificator_type::constant ? "const " : "var ";
    return str + type_->to_string(prefix + std::string(str.length(), ' '));
}

const type_p& modified_type::base_type() const { return type_; }

size_t modified_type::data_size() const {
    return type_->data_size();
}

//class pointer_type
const type_p& pointer_type::pointer_to() const { return pointer_to_; }

std::string pointer_type::to_string(const std::string& prefix) const {
    return str(boost::format("pointer to %1%") % pointer_to_->to_string());
}

type_p types::base_type(const type_p& t) {
    if (t->is_category(type::type_category::type))
        return std::dynamic_pointer_cast<type_type>(t)->alias_to();
    if (t->is_category(type::type_category::modified))
        return std::dynamic_pointer_cast<modified_type>(t)->base_type();
    return t;
}


