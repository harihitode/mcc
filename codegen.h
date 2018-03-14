#ifndef MCC_CODEGEN_H
#define MCC_CODEGEN_H

#include <llvm/IR/Module.h>
#include "ast.h"

namespace mcc {
    namespace codegen {
        llvm::Module * f(const std::vector<mcc::closure::toplevel_t> & ast);
    }
}

#endif
