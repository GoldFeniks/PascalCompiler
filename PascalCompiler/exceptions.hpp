#pragma once
#include <exception>
#include <string>

namespace pascal_compiler {

    class exception : public std::exception {
        
    public:

        explicit exception() {};

        const char* what() const override;

    protected:

        std::string message_ = "Pascal compiler exception";

    };
    
}// namespace pascal_compiler
