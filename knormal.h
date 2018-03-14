#ifndef KNORMAL_H
#define KNORMAL_H

#include "ast.h"

namespace mcc {
    namespace knormal {
        std::vector<toplevel_t> f(std::vector<mcc::parser::toplevel_t> && ast);
    }
}

#endif
