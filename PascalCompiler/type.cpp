#include "type.hpp"

using namespace pascal_compiler;
using namespace syntax_analyzer;
using namespace types;

const std::string type::type_strings[] = {
    "alias",  "integer",  "real",      "symbol",   "array",   "nil",
    "record", "function", "procedure", "modified", "pointer", "string"
};

//class type
const std::string& type::name() const { return name_; }

type::type_category type::category() const { return category_; }

bool type::is_category(const type_category category) const { return category == category_; }

const type_p& type::base_type() const { return std::make_shared<type>(*this); }

bool type::is_anonymous() const { return is_anonymous_; }

bool type::is_scalar() const {
    return category_ == type_category::integer ||
           category_ == type_category::real    ||
           category_ == type_category::symbol;        
}

//class alias_type
type_p alias_type::alias_to() const { return alias_to_; }

const type_p& alias_type::base_type() const {
    auto base = alias_to();
    while (base->is_category(type_category::alias))
        base = std::static_pointer_cast<alias_type>(base)->alias_to();
    return base;
}

//class array_type
size_t array_type::size() const { return max_ - min_; }

size_t array_type::min() const { return min_; }

size_t array_type::max() const { return max_; }

const type_p& array_type::element_type() const { return element_type_; }

//class record_type
void record_type::add_field(const std::string& name, const type_p& type) { fields_.add(name, type); }

const symbols_table& record_type::fields() const { return fields_; }

//class procedure_type
const symbols_table& function_type::parameters() const { return parameters_; }

const symbols_table& function_type::table() const { return table_; }

const type_p& function_type::return_type() const { return return_type_; }

//class modified_type
modified_type::modificator_type modified_type::modificator() const { return modificator_; }

const type_p& modified_type::base_type() const {
    auto base = type_;
    while (base->is_category(type_category::alias))
        base = std::static_pointer_cast<alias_type>(base)->alias_to();
    return base;
}

//class pointer_type
const type_p& pointer_type::pointer_to() const { return pointer_to_; }
