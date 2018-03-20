#ifndef MCC_CODEGEN_H
#define MCC_CODEGEN_H

#include <llvm/IR/Module.h>
#include "ast.h"

namespace mcc {
    namespace codegen {
        llvm::Module * f(const closure::module & mod);
    }
}

#endif
