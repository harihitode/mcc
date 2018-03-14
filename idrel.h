#ifndef MCC_ID_REL_H
#define MCC_ID_REL_H

#include "ast.h"

namespace mcc {
    namespace idrel {
        std::vector<parser::toplevel_t> f(std::vector<parser::toplevel_t> && ast);
    }
}

#endif
