#pragma once
#include "symbols_table.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include "tokenizer.hpp"
#include <stack>

namespace pascal_compiler {

    using namespace syntax_analyzer;

    // ReSharper disable CppNonExplicitConvertingConstructor
    // ReSharper disable CppNonExplicitConversionOperator
    namespace code {

        class asm_arg {
            
        public:

            virtual ~asm_arg() = default;

            enum class type : unsigned char {
                reg, mem, imm, label
            };
            
            virtual std::string to_string() const = 0;

        protected:

            explicit asm_arg(const type type) : type_(type) {}
            type get_type() const;
           
        private:

            type type_;
        
        };

        class asm_mem : public asm_arg {

        public:

            enum class mem_size {
                byte, word, dword, qword
            };

            static const std::string mem_size_str[];

            asm_mem(const mem_size size, const size_t offset) :
                asm_arg(type::mem), offset_(offset), size_(size) {}

            mem_size get_mem_size() const;
            std::string to_string() const override;

        private:

            size_t offset_;
            mem_size size_;
          
        };

        class asm_reg : public asm_arg {

        public:

            enum class reg_type {
                eax, ebx, ecx, edx, xmm0, xmm1, esp, ebp, al, cl, ah, bl, ax
            };

            asm_reg(const reg_type reg) : asm_arg(type::reg), reg_(reg) {}
            asm_reg(const reg_type reg, const asm_mem::mem_size size, const long long offset = 0) 
                : asm_arg(type::reg), reg_(reg), typed_(true), size_(size), offset_(offset) {}
            reg_type get_reg_type() const;
            std::string to_string() const override;

        private:

            reg_type reg_;
            bool typed_ = false;
            const asm_mem::mem_size size_ = asm_mem::mem_size::dword;
            const long long offset_ = 0;
            static const std::string reg_type_str[];

        };

        class asm_imm : public asm_arg {
            
        public:

            asm_imm(const std::string value) : asm_arg(type::imm), value_(value) {}
            asm_imm(const std::string value1, const std::string value2) : asm_arg(type::imm), value_(value1 + ' ' + value2) {}
            asm_imm(const int value) : asm_arg(type::imm), value_(std::to_string(value)) {}

            asm_imm(const asm_mem::mem_size size, const std::string value, const long offset)
                : asm_arg(type::imm), value_(value), offset_(offset), size_(size) {}

            std::string to_string() const override;

        private:

            std::string value_;
            long offset_ = -1;
            asm_mem::mem_size size_ = asm_mem::mem_size::dword;

        };

        class asm_command {
            
        public:

            enum class type {
                mov, push, pop, add, sub, imul, idiv, printf, movsd, 
                and, or, xor, mulsd, addsd, divsd, subsd, neg, pxor, 
                not, cdq, movsx, shl, shr,
                // ReSharper disable CppInconsistentNaming
                cvtsi2sd, cvttsd2si,
                // ReSharper restore CppInconsistentNaming
                setge, setg, setle, setl, sete, setne, cmp, jmp, label, 
                comisd, ucomisd, setbe, setb, seta, setae, jp, jnp, lahf, test,
                loop, jnz, jz, inc, dec, jge, jle, call, lea
            };

            asm_command(const type type, const asm_mem& arg1, const asm_mem& arg2);
            asm_command(const type type, const asm_reg& arg1, const asm_mem& arg2);
            asm_command(const type type, const asm_mem& arg1, const asm_reg& arg2);
            asm_command(const type type, const asm_reg& arg1, const asm_reg& arg2);
            asm_command(const type type, const asm_mem& arg1, const asm_imm& arg2);
            asm_command(const type type, const asm_reg& arg1, const asm_imm& arg2);
            asm_command(const type type, const asm_mem& arg);
            asm_command(const type type, const asm_reg& arg);
            asm_command(const type type, const asm_imm& arg);
            asm_command(const type type, const std::vector<std::shared_ptr<asm_arg>>& args);
            asm_command(const type type) : type_(type) {}
            std::string to_string() const;

        private:

            void add(const std::shared_ptr<asm_arg> arg1, const std::shared_ptr<asm_arg> arg2);
            void add(const std::shared_ptr<asm_arg> arg1);

            static const std::string type_str[];
            std::vector<std::shared_ptr<asm_arg>> args_;
            type type_;

            
        };

        class asm_code {

        public:

            asm_code() {}

            void push_back(const asm_command& command);
            void push_back(asm_command&& command);
            std::string to_string() const;
            std::pair<long long, long long> get_offset(const std::string& name) const;
            std::string add_double_constant(const double  value);
            std::string add_string_constant(const std::string& value);
            static std::string get_label_name(const size_t row, const size_t col, const std::string& suffix);
            void add_loop_start(const std::string& value);
            void add_loop_end(const std::string& value);
            void pop_loop_start();
            void pop_loop_end();
            void push_break();
            void push_continue();
            void start_function(const std::string& name, const size_t row, const size_t col, const symbols_table& data_table, const symbols_table& param_table);
            void end_function();
            std::string get_function_label(const std::string&) const;
            static std::string wrap_function_name(const std::string& name, const size_t row, const size_t col);


        private:

            std::vector<std::pair<std::string, std::vector<asm_command>>> commands_;
            std::vector<symbols_table> data_tables_, param_tables_;
            static const std::string data_types_str[];
            std::unordered_map<double, std::string> double_const_;
            std::unordered_map<std::string, size_t> string_const_;
            size_t strings_ = 0;
            std::stack<std::string> loop_ends_;
            std::stack<std::string> loop_starts_;
            int64_t index_ = -1;

        };
       
    }// namespace code
     // ReSharper restore CppNonExplicitConversionOperator
     // ReSharper restore CppNonExplicitConvertingConstructor
    
}// namespace pascal_compiler
