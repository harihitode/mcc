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
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

int main(int argc, char * argv[]) {

    if (argc < 2) {
        std::cerr << "USAGE: mcc [path to file]" << std::endl;
        return 0;
    }

    mcc::context ctx;
    llvm::LLVMContext lctx;
    std::experimental::filesystem::path filepath(argv[1]);

    bool gen_ir = true;

    auto start = std::chrono::system_clock::now();
    mcc::parser::module mod = mcc::parser::f(ctx, filepath.string());
    mod = mcc::idrel::f(ctx, std::move(mod));
    auto parsed = std::chrono::system_clock::now();
    mod = mcc::type::f(ctx, std::move(mod));

    auto typed = std::chrono::system_clock::now();

    auto mod_knormal = mcc::knormal::f(ctx, std::move(mod));
    auto knormalized = std::chrono::system_clock::now();

    mod_knormal = mcc::alpha::f(ctx, std::move(mod_knormal));
    auto optimized = std::chrono::system_clock::now();

    auto ast_closured = mcc::closure::f(ctx, std::move(mod_knormal));
    auto closured = std::chrono::system_clock::now();

    llvm::Module * module = mcc::codegen::f(lctx, ctx, ast_closured);
    auto finish = std::chrono::system_clock::now();

    llvm::legacy::PassManager pm;
    std::error_code ec;

    std::string ext = gen_ir ? ".ll" : ".s";
    llvm::raw_fd_ostream ofile(filepath.replace_extension(ext).filename().c_str(), ec, llvm::sys::fs::F_RW);
    pm.add(llvm::createPromoteMemoryToRegisterPass());
    pm.add(llvm::createIPSCCPPass());
    pm.add(llvm::createFunctionInliningPass());
    pm.add(llvm::createPartialInliningPass());
    pm.add(llvm::createConstantPropagationPass());
    pm.add(llvm::createLICMPass());
    pm.add(llvm::createNewGVNPass());
    pm.add(llvm::createGlobalDCEPass());
    if (gen_ir) {
        pm.add(llvm::createPrintModulePass(ofile));
    } else {
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();
        llvm::InitializeAllDisassemblers();

        std::string err;
        llvm::Triple tg_triple("sample", "hitode", "linux");

        llvm::TargetOptions opt;
        auto rm = llvm::Optional<llvm::Reloc::Model>();

        auto tg = llvm::TargetRegistry::lookupTarget("sample", tg_triple, err);
        std::cerr << "triple: " << tg_triple.str() << std::endl;

        if (!tg) {
            assert(false);
        }
        auto tg_machine = tg->createTargetMachine(tg_triple.str(), "sample", "", opt, rm);

        module->setTargetTriple(tg_triple.str());
        module->setDataLayout(tg_machine->createDataLayout());

        tg_machine->addPassesToEmitFile(pm, ofile, llvm::TargetMachine::CGFT_ObjectFile);
    }
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
