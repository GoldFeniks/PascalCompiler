#pragma once
#include "tree.hpp"
#include "symbols_table.hpp"

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

            static bool get_int_value(const tree_node_p node, long long& value);
            tree_node_p optimize_for(const for_node_p node);
            tree_node_p optimize_repeat(const repeat_node_p node);
            tree_node_p optimize_while(const while_node_p node);
            tree_node_p optimize_if(const if_node_p node);

        };

    }// namespace optimizer
    
    
}// namespace pascal_compiler
