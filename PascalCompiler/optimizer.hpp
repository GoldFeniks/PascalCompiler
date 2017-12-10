#pragma once
#include "tree.hpp"
#include "symbols_table.hpp"

#include <unordered_set>
#include <string>

namespace pascal_compiler {

    using namespace tree;

    namespace optimizer {

        class basic_optimizer {
            
        public:

            basic_optimizer() = default;
            virtual ~basic_optimizer() = default;

            virtual void optimize(tree_node_p node) {}
            virtual void optimize(symbols_table& table) {}

        };// class basic_optimizer

        class unreachable_code_optimizer : public basic_optimizer {
            
        public:

            unreachable_code_optimizer() = default;
            virtual ~unreachable_code_optimizer() = default;

            void optimize(tree_node_p node) override;
            void optimize(symbols_table& table) override;

        private:

            std::unordered_set<std::string> used_symbols_;

            static bool get_int_value(const tree_node_p node, long long& value);
            tree_node_p optimize_for(const for_node_p node);
            tree_node_p optimize_repeat(const repeat_node_p node);
            tree_node_p optimize_while(const while_node_p node);
            tree_node_p optimize_if(const if_node_p node);
            tree_node_p optimize_loop_body(const tree_node_p node, const tree_node_p body);
            tree_node_p remove_break_continue(const tree_node_p node) const;
            if_node_p make_if_from_loop(const tree_node_p loop_node) const;

        };

    }// namespace optimizer
    
    
}// namespace pascal_compiler
