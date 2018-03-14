#ifndef MCC_PARSER_H
#define MCC_PARSER_H

#include <string>
#include "ast.h"

namespace mcc {
    namespace parser {
        bool parse(std::string::const_iterator & bgn,
                   std::string::const_iterator & end,
                   std::vector<parser::toplevel_t> & ast);
    }
}

#endif
