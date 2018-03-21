#ifndef MCC_CLOSURE_H
#define MCC_CLOSURE_H

#include "ast.h"

namespace mcc {
    namespace closure {
        closure::module f(context & ctx, knormal::module && mod);
    }
}

#endif
