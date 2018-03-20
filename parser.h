#ifndef MCC_PARSER_H
#define MCC_PARSER_H

#include <string>
#include "ast.h"

namespace mcc {
    namespace parser {
        using itr_t = std::string::const_iterator;

        bool parse(itr_t & bgn, itr_t & end, std::vector<parser::toplevel_t> & asts);
        module f(const std::string & filepath);

    }
}

#endif
