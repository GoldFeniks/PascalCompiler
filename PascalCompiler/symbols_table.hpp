#pragma once
#include <map>
#include <utility>
#include <memory>
#include <string>
#include "exceptions.hpp"
#include <vector>

namespace pascal_compiler {

    namespace optimizer {

        class basic_optimizer;
        class unreachable_code_optimizer;

    }// namespace optimizer

    namespace code {

        class asm_code;

    }//namespace code

    using namespace code;

    namespace syntax_analyzer {

        namespace types {

            class type;
            typedef std::shared_ptr<type> type_p;

        }//namespace type

        using namespace types;

        namespace tree {

            class tree_node;

            typedef std::shared_ptr<tree_node> tree_node_p;

        }//namespace tree

        using namespace tree;

        class duplicate_symbol_exception : public exception {

        public:

            explicit duplicate_symbol_exception(const std::string& symbol);

        };

        class symbols_table {

        public:

            typedef std::pair<type_p, tree_node_p> symbol_t;
            typedef std::map<std::string, symbol_t> table_t;
            typedef std::vector<std::pair<std::string, symbol_t>> vector_t;
            typedef std::map<std::string, long long> offsets_t;

            symbols_table() {}

            void add(const std::string& name, const type_p type, const tree_node_p& value = nullptr);
            const type_p& get_type(const std::string& name) const;
            const type_p& get_type(const size_t index) const;
            const tree_node_p& get_value(const std::string& name) const;
            const symbol_t& operator[](const std::string& name) const;
            const symbol_t& operator[](const size_t index) const;
            void change_last(const symbol_t& symbol);
            void change(const std::string& name, const symbol_t& symbol);
            size_t size() const;
            const table_t& table() const;
            const vector_t& vector() const;
            std::string to_string(const std::string& prefix = "") const;
            void to_asm_code(asm_code& code) const;
            void calculate_offsets();
            long long get_offset(const std::string& name) const;
            long long get_data_size() const;

        private:

            friend class optimizer::basic_optimizer;
            friend class optimizer::unreachable_code_optimizer;

            table_t table_;
            vector_t vector_;
            offsets_t offsets_;
            size_t index_ = 0;
            long long size_ = 0;

        };

    }// namespace syntax_analyzer
    
}// namespace pascal_compiler
