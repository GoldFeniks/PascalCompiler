#pragma once
#include <string>
#include <memory>
#include "symbols_table.hpp"

namespace pascal_compiler {

    namespace syntax_analyzer {

        namespace types {

            class type;
            typedef std::shared_ptr<type> type_p;

            class type {

            public:

                enum class type_category : unsigned int {
                    character, integer, real, type, array, nil,
                    record, function, modified, pointer, string
                };

                static const std::string type_strings[];

                type(const std::string& name, const type_category category, const size_t size = 0) : name_(name), 
                    category_(category), size_(size), is_anonymous_(!name.length()) {}

                virtual ~type() {}

                std::string name() const;
                type_category category() const;
                bool is_category(const type_category category) const;
                bool is_anonymous() const;
                bool is_scalar() const;
                size_t data_size() const;

                virtual std::string to_string(const std::string& prefix = "") const;

            private:

                std::string name_;
                type_category category_;
                size_t size_;
                bool is_anonymous_ = false;

            };// class type

            //basic types
            inline type_p integer() {
                static const auto integer = std::make_shared<type>("integer", type::type_category::integer, 4);
                return integer;
            }

            inline type_p real() {
                static const auto real = std::make_shared<type>("real", type::type_category::real, 8);
                return real;
            }

            inline type_p character() {
                static const auto symbol = std::make_shared<type>("char", type::type_category::character, 1);
                return symbol;
            }

            inline type_p nil() {
                static const auto nil = std::make_shared<type>("nil", type::type_category::nil, 0);
                return nil;
            }

            inline type_p string() {
                static const auto nil = std::make_shared<type>("string", type::type_category::string, 0);
                return nil;
            }

            class type_type;
            typedef std::shared_ptr<type_type> alias_type_p;

            class type_type : public type {

            public:

                type_type(const std::string& name, const type_p& alias_to) :
                    type(name, type_category::type), alias_to_(alias_to) {}

                type_p alias_to() const;
                std::string to_string(const std::string& prefix = "") const override;

            private:

                type_p alias_to_;

            };// class type_type

            class array_type;
            typedef std::shared_ptr<array_type> array_type_p;

            class array_type : public type {

            public:

                array_type(const std::string& name, const size_t min, const size_t max, const type_p& element_type) :
                    type(name, type_category::array), min_(min), max_(max), element_type_(element_type) {};

                size_t size() const;
                size_t min() const;
                size_t max() const;
                const type_p& element_type() const;
                std::string to_string(const std::string& prefix = "") const override;

            private:

                size_t min_ = 0, max_ = 0;
                type_p element_type_;

            };// class array_type

            class record_type;
            typedef std::shared_ptr<record_type> record_type_p;

            class record_type : public type {

            public:

                explicit record_type(const std::string& name) : type(name, type_category::record) {};

                void add_field(const std::string& name, const type_p& type);
                const symbols_table& fields() const;
                std::string to_string(const std::string& prefix = "") const override;

            private:

                symbols_table fields_;

            };//class record_type

            class function_type;
            typedef std::shared_ptr<function_type> function_type_p;

            class function_type : public type {

            public:

                function_type(const std::string& name, const symbols_table& parameters, const symbols_table& table,
                    const type_p& return_type = nil()) :
                    type(name, type_category::function), parameters_(parameters), table_(table), return_type_(return_type) {};

                const symbols_table& parameters() const;
                const symbols_table& table() const;
                const type_p& return_type() const;
                std::string to_string(const std::string& prefix = "") const override;

            private:

                symbols_table parameters_, table_;
                type_p return_type_;

            };// class procedure_type

            class modified_type;
            typedef std::shared_ptr<modified_type> modified_type_p;

            class modified_type : public type {

            public:

                enum class modificator_type { constant, var };

                modified_type(const modificator_type modificator, const type_p& type) :
                    type("", type_category::modified), type_(type), modificator_(modificator) {};

                modificator_type modificator() const;
                std::string to_string(const std::string& prefix = "") const override;
                const type_p& base_type() const;

            private:

                type_p type_;
                modificator_type modificator_;

            };// class modified_type

            class pointer_type;
            typedef std::shared_ptr<pointer_type> pointer_type_p;

            class pointer_type : public type {

            public:

                pointer_type(const std::string& name, const type_p& pointer_to) :
                    type(name, type_category::pointer), pointer_to_(pointer_to) {}

                const type_p& pointer_to() const;
                std::string to_string(const std::string& prefix = "") const override;

            private:

                type_p pointer_to_;

            };// class pointer_type

            type_p base_type(const type_p& t);

        }// namespase types

    }// namespase syntax_analyzer
    
}// namespace pascal_compiler
