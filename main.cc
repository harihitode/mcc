#include <iostream>
#include <experimental/filesystem>
#include <tuple>
#include <chrono>

#include "ast.h"
#include "printer.h"
#include "parser.h"
#include "idrel.h"
#include "typing.h"
#include "knormal.h"
#include "alpha.h"
#include "closure.h"
#include "codegen.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>

int main(int argc, char * argv[]) {

    if (argc < 2) {
        std::cerr << "USAGE: mcc [path to file]" << std::endl;
        return 0;
    }

    std::experimental::filesystem::path filepath(argv[1]);

    auto start = std::chrono::system_clock::now();
    mcc::parser::module mod = mcc::parser::f(filepath.string());
    mod = mcc::idrel::f(std::move(mod));
    auto parsed = std::chrono::system_clock::now();
    mod = mcc::type::f(std::move(mod));

    auto typed = std::chrono::system_clock::now();

    auto mod_knormal = mcc::knormal::f(std::move(mod));
    auto knormalized = std::chrono::system_clock::now();

    mod_knormal = mcc::alpha::f(std::move(mod_knormal));
    auto optimized = std::chrono::system_clock::now();

    auto ast_closured = mcc::closure::f(std::move(mod_knormal));
    auto closured = std::chrono::system_clock::now();

    llvm::Module * module = mcc::codegen::f(ast_closured);
    auto finish = std::chrono::system_clock::now();

    llvm::legacy::PassManager pm;
    std::error_code ec;
    llvm::raw_fd_ostream ofile(filepath.replace_extension(".ll").filename().c_str(), ec, llvm::sys::fs::F_RW);
    pm.add(llvm::createPromoteMemoryToRegisterPass());
    pm.add(llvm::createIPSCCPPass());
    pm.add(llvm::createFunctionInliningPass());
    pm.add(llvm::createPartialInliningPass());
    pm.add(llvm::createConstantPropagationPass());
    pm.add(llvm::createLICMPass());
    pm.add(llvm::createNewGVNPass());
    pm.add(llvm::createGlobalDCEPass());
    pm.add(llvm::createPrintModulePass(ofile));
    pm.run(*module);
    ofile.close();

    std::cerr << "[PARSING TIME] " << std::chrono::duration_cast<std::chrono::milliseconds>(parsed - start).count() << "(ms)" << std::endl;
    std::cerr << "[TYPING TIME] " << std::chrono::duration_cast<std::chrono::milliseconds>(typed - parsed).count() << "(ms)" << std::endl;
    std::cerr << "[K-NORMALIZING TIME] " << std::chrono::duration_cast<std::chrono::milliseconds>(knormalized - typed).count() << "(ms)" << std::endl;
    std::cerr << "[ALPHA TRANSFORM TIME] " << std::chrono::duration_cast<std::chrono::milliseconds>(optimized - knormalized).count() << "(ms)" << std::endl;
    std::cerr << "[FLATTEN TIME] " << std::chrono::duration_cast<std::chrono::milliseconds>(closured - optimized).count() << "(ms)" << std::endl;
    std::cerr << "[TOTAL] " << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << "(ms)" << std::endl;

    return 0;
}
