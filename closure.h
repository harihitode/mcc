#ifndef MCC_CLOSURE_H
#define MCC_CLOSURE_H

#include "ast.h"

namespace mcc {
    namespace closure {
        std::vector<mcc::closure::toplevel_t> f(std::vector<mcc::knormal::toplevel_t> && ast);
    }
}

#endif
