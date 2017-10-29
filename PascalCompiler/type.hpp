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
                    alias, integer, real, symbol, array, nil,
                    record, function, modified, pointer, string
                };

                static const std::string type_strings[];

                type(const std::string& name, const type_category category) : name_(name), 
                    category_(category), is_anonymous_(name.length()) {}

                virtual ~type() {}

                const std::string& name() const;
                type_category category() const;
                bool is_category(const type_category category) const;
                virtual const type_p& base_type() const;
                bool is_anonymous() const;
                bool is_scalar() const;

            private:

                std::string name_;
                type_category category_;
                bool is_anonymous_ = false;

            };// class type

            //basic types
            const type_p integer = std::make_shared<type>("integer", type::type_category::integer);
            const type_p real = std::make_shared<type>("real", type::type_category::real);
            const type_p symbol = std::make_shared<type>("char", type::type_category::symbol);
            const type_p string = std::make_shared<type>("string", type::type_category::string);
            const type_p nil = std::make_shared<type>("nil", type::type_category::nil);

            class alias_type;
            typedef std::shared_ptr<alias_type> alias_type_p;

            class alias_type : public type {

            public:

                alias_type(const std::string& name, const type_p& alias_to) :
                    type(name, type_category::alias), alias_to_(alias_to) {}

                type_p alias_to() const;
                const type_p& base_type() const override;

            private:

                type_p alias_to_;

            };// class alias_type

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

            private:

                symbols_table fields_;

            };//class record_type

            class function_type;
            typedef std::shared_ptr<function_type> function_type_p;

            class function_type : public type {

            public:

                function_type(const std::string& name, const symbols_table& parameters, const symbols_table& table,
                    const type_p& return_type = nil) :
                    type(name, type_category::function), parameters_(parameters), table_(table), return_type_(return_type) {};

                const symbols_table& parameters() const;
                const symbols_table& table() const;
                const type_p& return_type() const;

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
                const type_p& base_type() const override;

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

            private:

                type_p pointer_to_;

            };// class pointer_type

        }// namespase types

    }// namespase syntax_analyzer
    
}// namespace pascal_compiler
