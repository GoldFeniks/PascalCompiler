#include "exceptions.hpp"

using namespace pascal_compiler;

const char* exception::what() const { return message_.c_str(); }
