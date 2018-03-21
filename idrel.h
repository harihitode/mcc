#ifndef MCC_ID_REL_H
#define MCC_ID_REL_H

#include "ast.h"

namespace mcc {
    namespace idrel {
        parser::module f(context & ctx, parser::module && ast);
    }
}

#endif
