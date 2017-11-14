#pragma once
#include "symbols_table.hpp"
#include <string>
#include <vector>
#include <unordered_map>

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
                eax, ebx, ecx, edx, xmm0, xmm1, esp, ebp, al, cl, ah, bl
            };

            asm_reg(const reg_type reg) : asm_arg(type::reg), reg_(reg) {}
            asm_reg(const reg_type reg, const asm_mem::mem_size size, const long offset = 0) 
                : asm_arg(type::reg), reg_(reg), typed_(true), size_(size), offset_(offset) {}
            reg_type get_reg_type() const;
            std::string to_string() const override;

        private:

            reg_type reg_;
            bool typed_ = false;
            const asm_mem::mem_size size_ = asm_mem::mem_size::dword;
            const long offset_ = 0;
            static const std::string reg_type_str[];

        };

        class asm_imm : public asm_arg {
            
        public:

            asm_imm(const std::string value) : asm_arg(type::imm), value_(value) {}

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
                comisd, ucomisd, setbe, setb, seta, setae, jp, jnp, lahf, test
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
            void add(const std::shared_ptr<asm_arg>arg1);

            static const std::string type_str[];
            std::vector<std::shared_ptr<asm_arg>> args_;
            type type_;

            
        };

        class asm_code {

        public:

            asm_code() {}

            void push_back(const asm_command& command);
            void push_back(asm_command&& command);
            void add_data(const std::string& name, const symbols_table::symbol_t& symbol);
            std::string to_string() const;
            size_t get_offset(const std::string& name) const;
            std::string add_double_constant(const double  value);

        private:

            std::unordered_map<std::string, size_t> offsets_;
            size_t data_size_ = 0;
            std::vector<asm_command> commands_;
            static const std::string data_types_str[];
            std::unordered_map<double, std::string> double_const_;

        };
       
    }// namespace code
     // ReSharper restore CppNonExplicitConversionOperator
     // ReSharper restore CppNonExplicitConvertingConstructor
    
}// namespace pascal_compiler
