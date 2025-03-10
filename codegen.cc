#include "codegen.h"
#include "typing.h"
#include "printer.h"
#include <iostream>
#include <deque>

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/IR/IRBuilder.h>

using llvm::Value;
using llvm::LLVMContext;
using llvm::Module;
using llvm::IRBuilder;
using llvm::Function;
using llvm::BasicBlock;
using llvm::Type;

using namespace mcc::operand;

namespace {
    using namespace mcc;
    using mcc::type::unwrap;

    template <typename Op>
    Value * binary_op(IRBuilder<> * b, Value * lhs, Value * rhs) {
        assert(false); // unknown
    }
    template <>
    Value * binary_op<op_add>(IRBuilder<> * b, Value * lhs, Value * rhs) {
        return b->CreateAdd(lhs, rhs);
    }
    template <>
    Value * binary_op<op_sub>(IRBuilder<> * b, Value * lhs, Value * rhs) {
        return b->CreateSub(lhs, rhs);
    }
    template <>
    Value * binary_op<op_mul>(IRBuilder<> * b, Value * lhs, Value * rhs) {
        return b->CreateMul(lhs, rhs);
    }
    template <>
    Value * binary_op<op_div>(IRBuilder<> * b, Value * lhs, Value * rhs) {
        return b->CreateSDiv(lhs, rhs);
    }
    template <>
    Value * binary_op<op_fadd>(IRBuilder<> * b, Value * lhs, Value * rhs) {
        return b->CreateFAdd(lhs, rhs);
    }
    template <>
    Value * binary_op<op_fsub>(IRBuilder<> * b, Value * lhs, Value * rhs) {
        return b->CreateFSub(lhs, rhs);
    }
    template <>
    Value * binary_op<op_fmul>(IRBuilder<> * b, Value * lhs, Value * rhs) {
        return b->CreateFMul(lhs, rhs);
    }
    template <>
    Value * binary_op<op_fdiv>(IRBuilder<> * b, Value * lhs, Value * rhs) {
        return b->CreateFDiv(lhs, rhs);
    }
    template <>
    Value * binary_op<op_eq>(IRBuilder<> * b, Value * lhs, Value * rhs) {
        if (lhs->getType()->isIntegerTy()) {
            return b->CreateICmpEQ(lhs, rhs);
        } else {
            return b->CreateFCmpOEQ(lhs, rhs);
        }
    }
    template <>
    Value * binary_op<op_le>(IRBuilder<> * b, Value * lhs, Value * rhs) {
        if (lhs->getType()->isIntegerTy()) {
            return b->CreateICmpSLE(lhs, rhs);
        } else {
            return b->CreateFCmpOLE(lhs, rhs);
        }
    }

    Type * convert_type(llvm::LLVMContext & lctx, const type::type_t & t) {
        if (std::get_if<sptr<type::boolean>>(&t)) { // boolean should be removed in k-normalization
            return llvm::Type::getInt1Ty(lctx);
        } else if (std::get_if<sptr<type::integer>>(&t)) {
            return llvm::Type::getInt32Ty(lctx);
        } else if (std::get_if<sptr<type::floating_point>>(&t)) {
            return llvm::Type::getFloatTy(lctx);
        } else if (std::get_if<sptr<type::array>>(&t)) {
            auto elem_t = std::get<sptr<type::array>>(t);
            return llvm::PointerType::getUnqual(convert_type(lctx, elem_t->value));
        } else if (std::get_if<sptr<type::tuple>>(&t)) {
            auto tuple = std::get<sptr<type::tuple>>(t);
            std::vector<Type *> ts;
            std::transform(tuple->value.begin(), tuple->value.end(), std::back_inserter(ts), [&lctx] (auto & t) { return convert_type(lctx, t); });
            auto struct_t = llvm::StructType::get(lctx, ts);
            return llvm::PointerType::getUnqual(struct_t);
        } else if (std::get_if<sptr<type::function>>(&t)) {
            auto function_t = std::get<sptr<type::function>>(t);
            auto result_t = std::get<0>(function_t->value);
            auto params_t = std::get<1>(function_t->value);
            Type * result_tt = convert_type(lctx, result_t);
            std::vector<Type *> params_tt;
            if (function_t->is_closure) {
                // +1 argument for closure
                // closure type is void*, needed to be casted
                // closure_type is actually ptr to struct {(ptr to function (closure_address, arg1, arg2, ...)), (ptr of fv)}
                params_tt.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt32Ty(lctx)));
            }
            std::transform(params_t.begin(), params_t.end(), std::back_inserter(params_tt), [&lctx] (auto & arg) {
                    return convert_type(lctx, arg);
                });
            auto rm_bgn = std::remove_if(params_tt.begin(), params_tt.end(), [] (auto & t) {
                    return t->isVoidTy();
                });
            params_tt.erase(rm_bgn, params_tt.end());
            // as closure
            return llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(llvm::FunctionType::get(result_tt, params_tt, false)));
        } else if (std::get_if<sptr<type::variable>>(&t)){
            auto variable_t = std::get<sptr<type::variable>>(t);
            return convert_type(lctx, unwrap(t));
        } else {
            return llvm::Type::getVoidTy(lctx);
        }
    }

    struct pass {
        using result_type = Value *;

        LLVMContext & lctx;
        IRBuilder<> * builder;
        Module * module;
        Function * function;

        pass(LLVMContext & l, IRBuilder<> * b, Module * m, Function * f = nullptr) : lctx(l), builder(b), module(m), function(f) { }

        result_type operator() (const closure::ast & e) {
            return std::visit(pass(lctx, builder, module, function), e);
        }
        result_type operator() (const sptr<closure::unit> & e) {
            return llvm::ConstantTokenNone::get(lctx);
        }
        result_type operator() (const sptr<closure::integer> & e) {
            return llvm::ConstantInt::get(llvm::Type::getInt32Ty(lctx), e->value);
        }
        result_type operator() (const sptr<closure::boolean> & e) {
            return llvm::ConstantInt::get(llvm::Type::getInt1Ty(lctx), e->value);
        }
        result_type operator() (const sptr<closure::floating_point> & e) {
            return llvm::ConstantFP::get(llvm::Type::getFloatTy(lctx), e->value);
        }
        result_type operator() (const sptr<closure::unary<op_fneg>> & e) {
            Value * rhs = pass(lctx, builder, module, function)(e->value);
            return builder->CreateFNeg(rhs, "fneg_tmp");
        }
        result_type operator() (const sptr<closure::unary<op_neg>> & e) {
            Value * rhs = pass(lctx, builder, module, function)(e->value);
            return builder->CreateNeg(rhs, "neg_tmp");
        }
        template <typename Op>
        result_type operator() (const sptr<closure::binary<Op>> & e) {
            Value * lhs = pass(lctx, builder, module, function)(std::get<0>(e->value));
            Value * rhs = pass(lctx, builder, module, function)(std::get<1>(e->value));
            return binary_op<Op>(builder, lhs, rhs);
        }
        result_type operator() (const sptr<closure::identifier> & e) {
            llvm::ValueSymbolTable * vs_table = function->getValueSymbolTable();
            auto & name = std::get<0>(e->value);
            auto typ = unwrap(std::get<1>(e->value));
            if (auto v = vs_table->lookup(name)) {
                return builder->CreateLoad(v->getType(), v, "var_tmp");
            }
            if (auto v = module->getGlobalVariable(name)) {
                return builder->CreateLoad(v->getType(), v, "var_tmp");
            }
            if (auto utyp = std::get_if<std::shared_ptr<type::function>>(&typ)) {
                //if (!(*utyp)->is_closure) {
                return module->getFunction(name);
                //}
            }
            assert(false);
            return nullptr;
        }
        result_type operator() (const sptr<closure::branch> & e) {
            Value * cond = pass(lctx, builder, module, function)(std::get<0>(e->value));
            BasicBlock * then_bb = llvm::BasicBlock::Create(lctx, "then", function);
            BasicBlock * else_bb = llvm::BasicBlock::Create(lctx, "else", function);
            BasicBlock * merge_bb = llvm::BasicBlock::Create(lctx, "merge", function);
            builder->CreateCondBr(cond, then_bb, else_bb);

            // then block
            builder->SetInsertPoint(then_bb);
            Value * then_v = std::visit(pass(lctx, builder, module, function), std::get<1>(e->value));
            builder->CreateBr(merge_bb);
            // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
            then_bb = builder->GetInsertBlock();

            // else block
            builder->SetInsertPoint(else_bb);
            Value * else_v = std::visit(pass(lctx, builder, module, function), std::get<2>(e->value));
            builder->CreateBr(merge_bb);
            // codegen of 'Else' can change the current block, update ElseBB for the PHI.
            else_bb = builder->GetInsertBlock();

            // merge block (phi)
            builder->SetInsertPoint(merge_bb);
            // if (then_v) { printf ("then_v: \n"); (*then_v)->getType()->print(llvm::outs(), false); putchar('\n'); }
            // if (else_v) { printf ("else_v: \n"); (*else_v)->getType()->print(llvm::outs(), false); putchar('\n'); }
            // printf("if_eq END: %s, %s\n", std::get<0>(e->value).c_str(), std::get<1>(e->value).c_str());
            if (then_v && else_v && !then_v->getType()->isVoidTy() && !else_v->getType()->isVoidTy()) {
                llvm::PHINode * phi = builder->CreatePHI(then_v->getType(), 2, "iftmp");
                phi->addIncoming(then_v, then_bb);
                phi->addIncoming(else_v, else_bb);
                return phi;
            } else {
                return nullptr;
            }
        }
        result_type operator() (const sptr<closure::tuple> & e) {
            std::vector<Value *> vs;
            std::vector<Type *> ts;
            std::transform(e->value.begin(), e->value.end(), std::back_inserter(vs), pass(lctx, builder, module, function));
            std::transform(vs.begin(), vs.end(), std::back_inserter(ts), [] (auto & v) { return v->getType(); });
            Type * tuple_type = llvm::StructType::get(lctx, ts);
            llvm::Constant * tuple_size = llvm::ConstantExpr::getSizeOf(tuple_type);
            tuple_size = llvm::ConstantExpr::getTruncOrBitCast(tuple_size, llvm::Type::getInt32Ty(lctx));
#if MCC_LLVM_VERSION >= 19
            llvm::Instruction * v = builder->CreateMalloc(llvm::Type::getInt32Ty(lctx),
                                                          tuple_type, tuple_size, nullptr, nullptr, "var_tuple");
#else
            BasicBlock * curr_bb = builder->GetInsertBlock();
            llvm::Instruction * v = llvm::CallInst::CreateMalloc(curr_bb,
                                                                 llvm::Type::getInt32Ty(lctx),
                                                                 tuple_type,
                                                                 tuple_size, nullptr, nullptr, "var_tuple");
            curr_bb->getInstList().push_back(v);
#endif
            // store values
            for (size_t i = 0; i < vs.size(); i++) {
                std::vector<Value *> idx = {llvm::ConstantInt::get(llvm::Type::getInt32Ty(lctx), 0),
                                            llvm::ConstantInt::get(llvm::Type::getInt32Ty(lctx), i)};
                llvm::Value * ptr = builder->CreateGEP(tuple_type, v, idx, "ptr_tmp");
                builder->CreateStore(vs[i], ptr);
            }
            return v;
        }
        result_type operator() (const sptr<closure::app> & e) {
            auto && ident = std::get<0>(e->value);
            auto && fun_type = std::get<1>(ident->value);
            std::vector<Value *> args;
            Value * fun_ptr = pass(lctx, builder, module, function)(ident);
            auto fun_typ = std::get<std::shared_ptr<type::function>>(unwrap(fun_type));
            if (fun_typ->is_closure) {
                // push closure arguments
                auto cls_ptr_raw = builder->CreatePointerCast(fun_ptr, llvm::PointerType::getUnqual(llvm::Type::getInt32Ty(lctx)));
                args.push_back(cls_ptr_raw);
                fun_ptr = builder->CreateLoad(fun_ptr->getType(), fun_ptr, "fun_tmp");
            }
            {
                std::vector<std::shared_ptr<closure::identifier>> args_filtered;
                std::copy_if(std::get<1>(e->value).begin(), std::get<1>(e->value).end(), std::back_inserter(args_filtered), [] (auto & n) {
                        return std::get_if<std::shared_ptr<type::unit>>(&std::get<1>(n->value)) == nullptr;
                    });
                std::transform(args_filtered.begin(), args_filtered.end(), std::back_inserter(args), [this] (auto & n) {
                        return pass(lctx, builder, module, function)(n);
                    });
            }
            return builder->CreateCall(module->getFunction(std::get<0>(ident->value))->getFunctionType(), fun_ptr, args);
        }
        result_type operator() (const sptr<closure::let_rec> & e) {
            // ts is struct of {function_address, args...}
            std::vector<Type *> ts;
            std::deque<Value *> vs;
            auto && ident = std::get<0>(e->value);
            auto && free_var = std::get<1>(e->value);
            auto && function_type = module->getFunction(std::get<0>(ident->value))->getFunctionType();
            // for function address
            ts.push_back(llvm::PointerType::getUnqual(function_type));
            // for free variables
            std::transform(free_var.begin(), free_var.end(), std::back_inserter(vs), pass(lctx, builder, module, function));
            std::transform(vs.begin(), vs.end(), std::back_inserter(ts), [] (auto & n) { return n->getType(); });
            // for function address
            vs.push_front(module->getFunction(std::get<0>(ident->value)));
            // make closure struct
            auto cls_type = llvm::StructType::get(lctx, ts);
            llvm::Constant * cls_size = llvm::ConstantExpr::getSizeOf(cls_type);
            cls_size = llvm::ConstantExpr::getTruncOrBitCast(cls_size, llvm::Type::getInt32Ty(lctx));
#if MCC_LLVM_VERSION >= 19
            llvm::Instruction * v = builder->CreateMalloc(llvm::Type::getInt32Ty(lctx),
                                                              cls_type, cls_size, nullptr, nullptr, "var_cls");
#else
            BasicBlock * curr_bb = builder->GetInsertBlock();
            llvm::Instruction * v = llvm::CallInst::CreateMalloc(curr_bb,
                                                                 llvm::Type::getInt32Ty(lctx),
                                                                 cls_type,
                                                                 cls_size, nullptr, nullptr, "var_cls");
            curr_bb->getInstList().push_back(v);
#endif
            // store closure variables
            {
                int i = 0;
                std::for_each(vs.begin(), vs.end(), [this, &v, &i, &cls_type] (auto & n) {
                        std::vector<Value *> idx = {llvm::ConstantInt::get(llvm::Type::getInt32Ty(lctx), 0),
                                                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(lctx), i++)};
                        llvm::Value * ptr = builder->CreateGEP(cls_type, v, idx, "ptr_tmp");
                        builder->CreateStore(n, ptr);
                    });
            }
            // TODO: type is functiontype is better
            Value * store_value = builder->CreatePointerCast(v, llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(function_type)), "ptrcast");
            Value * store = builder->CreateAlloca(llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(function_type)), 0, std::get<0>(std::get<0>(e->value)->value));
            builder->CreateStore(store_value, store);
            return pass(lctx, builder, module, function)(std::get<3>(e->value));
        }
        result_type operator() (const sptr<closure::get> & e) {
            Value * arr = pass(lctx, builder, module, function)(std::get<0>(e->value));
            Value * idx = pass(lctx, builder, module, function)(std::get<1>(e->value));
            Value * ptr = builder->CreateGEP(arr->getType(), arr, idx, "ptr_tmp");
            return builder->CreateLoad(ptr->getType(), ptr, "var_tmp");
        }
        result_type operator() (const sptr<closure::put> & e) {
            Value * arr = pass(lctx, builder, module, function)(std::get<0>(e->value));
            Value * idx = pass(lctx, builder, module, function)(std::get<1>(e->value));
            Value * val = pass(lctx, builder, module, function)(std::get<2>(e->value));
            Value * ptr = builder->CreateGEP(arr->getType(), arr, idx, "ptr_tmp");
            return builder->CreateStore(val, ptr);
        }
        result_type operator() (const sptr<closure::array> & e) {
            // printf("array\n");
            Value * num = pass(lctx, builder, module, function)(std::get<0>(e->value));
            Value * init = pass(lctx, builder, module, function)(std::get<1>(e->value));
            Type * elem_type = init->getType();
            llvm::Constant * elem_size = llvm::ConstantExpr::getSizeOf(elem_type);
            elem_size = llvm::ConstantExpr::getTruncOrBitCast(elem_size, llvm::Type::getInt32Ty(lctx));
            BasicBlock * curr_bb = builder->GetInsertBlock();
#if MCC_LLVM_VERSION >= 19
            llvm::Instruction * v = builder->CreateMalloc(llvm::Type::getInt32Ty(lctx),
                                                          elem_type, elem_size, num, nullptr, "var_array");
#else
            llvm::Instruction * v = llvm::CallInst::CreateMalloc(builder->GetInsertBlock(),
                                                                 llvm::Type::getInt32Ty(lctx),
                                                                 elem_type,
                                                                 elem_size, num, nullptr, "var_array");
            curr_bb->getInstList().push_back(v);
#endif
            BasicBlock * cond_bb = llvm::BasicBlock::Create(lctx, "cond", function);
            BasicBlock * body_bb = llvm::BasicBlock::Create(lctx, "body", function);
            BasicBlock * done_bb = llvm::BasicBlock::Create(lctx, "done", function);

            builder->CreateBr(cond_bb);
            builder->SetInsertPoint(cond_bb);

            auto counter = builder->CreatePHI(llvm::Type::getInt32Ty(lctx), 2, "iftmp");
            counter->addIncoming(llvm::ConstantInt::get(llvm::Type::getInt32Ty(lctx), 0), curr_bb);
            counter->addIncoming(builder->CreateAdd(counter, llvm::ConstantInt::get(llvm::Type::getInt32Ty(lctx), 1)), body_bb);
            auto cond = builder->CreateICmpSLT(counter, num);
            builder->CreateCondBr(cond, body_bb, done_bb);
            // initialization block
            builder->SetInsertPoint(body_bb);
            // init array elements
            auto ptr = builder->CreateGEP(llvm::PointerType::getUnqual(elem_type), v, counter, "ptr_tmp");
            builder->CreateStore(init, ptr);
            // jump
            builder->CreateBr(cond_bb);
            builder->SetInsertPoint(done_bb);
            return v;
        }
        result_type operator() (const sptr<closure::let_tuple> & e) {
            auto && tuple = pass(lctx, builder, module, function)(std::get<1>(e->value));
            std::vector<Value *> elems_vs;
            std::vector<Type *> elems_ts;
            std::transform(std::get<0>(e->value).begin(), std::get<0>(e->value).end(), std::back_inserter(elems_vs), pass(lctx, builder, module, function));
            std::transform(elems_vs.begin(), elems_vs.end(), std::back_inserter(elems_ts), [] (auto & v) { return v->getType(); });
            {
                size_t index = 0;
                auto elems_ts_itr = elems_ts.begin();
                std::for_each(std::get<0>(e->value).begin(), std::get<0>(e->value).end(), [this, &index, &elems_ts_itr, tuple] (auto & i) {
                        Type * ty = *(elems_ts_itr++);
                        auto && arg = builder->CreateAlloca(ty, 0, std::get<0>(i->value));
                        std::vector<Value *> t_index = {llvm::ConstantInt::get(llvm::Type::getInt32Ty(lctx), 0),
                                                        llvm::ConstantInt::get(llvm::Type::getInt32Ty(lctx), index++)};
                        Value * ptr = builder->CreateGEP(tuple->getType(), tuple, t_index, "ptr_tmp");
                        Value * var = builder->CreateLoad(ptr->getType(), ptr, "var_tmp");
                        builder->CreateStore(var, arg);
                    });
            }
            return std::visit(pass(lctx, builder, module, function), std::get<2>(e->value));
        }
        result_type operator() (const sptr<closure::let> & e) {
            auto && ident = std::get<0>(e->value);
            auto && id_name = std::get<0>(ident->value);
            auto && id_type = std::get<1>(ident->value);
            if (std::get_if<std::shared_ptr<type::unit>>(&id_type)) {
                // if the variable is unit
                std::visit(pass(lctx, builder, module, function), std::get<1>(e->value));
            } else {
                auto t = std::visit(pass(lctx, builder, module, function), std::get<1>(e->value));
                // real
                llvm::Type * ty = t->getType();
                auto alloc = builder->CreateAlloca(ty, 0, id_name);
                builder->CreateStore(t, alloc);
            }
            // printf("let end: %s\n", std::get<0>(e->value).first.c_str());
            auto e2 = std::visit(pass(lctx, builder, module, function), std::get<2>(e->value));
            return e2;
        }

    };
}

namespace {
    Function * create_function_prototype(const std::shared_ptr<mcc::closure::identifier> & ident, LLVMContext & lctx, Module * module) {
        auto && id_name = std::get<0>(ident->value);
        auto id_type_conved = convert_type(lctx, std::get<1>(ident->value));
        // TODO: check the converted type
        auto func_type = static_cast<llvm::FunctionType *>(id_type_conved);
        // internal?
        return llvm::Function::Create(func_type,
                                      llvm::Function::ExternalLinkage,
                                      id_name,
                                      module);
    }
    void create_function_definition(const std::shared_ptr<closure::global_rec> & f, LLVMContext & lctx, Module * module, IRBuilder<> * builder) {
        auto && fun_name = std::get<0>(std::get<0>(f->value)->value);
        // printf("function def: %s\n", fun_name.c_str());
        auto fun_type = std::get<sptr<type::function>>(unwrap(std::get<1>(std::get<0>(f->value)->value)));
        auto func = module->getFunction(fun_name);
        auto func_type = func->getFunctionType();
        auto arg_itr = func->arg_begin();
        std::vector<std::string> arg_names;
        if (fun_type->is_closure) {
            arg_names.push_back("closure");
        }
        {
            std::vector<std::shared_ptr<closure::identifier>> arg_names_tmp;
            std::copy_if(std::get<1>(f->value).begin(), std::get<1>(f->value).end(), std::back_inserter(arg_names_tmp), [&lctx] (auto & n) {
                    auto && arg_type = std::get<1>(n->value);
                    Type * t = convert_type(lctx, arg_type);
                    return !(t->isVoidTy());
                });
            std::transform(arg_names_tmp.begin(), arg_names_tmp.end(), std::back_inserter(arg_names), [] (auto & n) {
                    return std::get<0>(n->value);
                });
        }
        std::for_each(arg_names.begin(), arg_names.end(), [&arg_itr] (auto & n) {
                (arg_itr++)->setName(n + "_arg");
            });
        auto bblock = llvm::BasicBlock::Create(lctx, "entry", func);
        builder->SetInsertPoint(bblock);
        // printf("argument_parameter num: %lu\n", std::get<1>(f).size());
        // printf("function_parameter num: %u\n", func_type->getNumParams());
        llvm::ValueSymbolTable * vs_table = func->getValueSymbolTable();
        {
            auto param_t = func_type->param_begin();
            auto arg_names_begin = arg_names.begin();
            if (fun_type->is_closure) {
                ++param_t;
                ++arg_names_begin;
            }
            std::for_each(arg_names_begin, arg_names.end(), [&param_t, &builder, &vs_table] (auto & n) {
                    auto && alloca = builder->CreateAlloca(*(param_t++), 0, n);
                    builder->CreateStore(vs_table->lookup(n + "_arg"), alloca);
                });
        }
        // load free variables
        if (fun_type->is_closure) {
            // printf("free variable restoration\n");
            std::vector<Type *> ts;
            ts.push_back(llvm::PointerType::getUnqual(func_type));
            std::transform(std::get<2>(f->value).begin(), std::get<2>(f->value).end(), std::back_inserter(ts), [&lctx] (auto && fv_name) {
                    return convert_type(lctx, std::get<1>(fv_name->value));
                });
            Type * closure_ty = llvm::PointerType::getUnqual(llvm::StructType::get(lctx, ts));
            Type * function_ptr_ptr = llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(func_type));
            auto && closure = builder->CreatePointerCast(vs_table->lookup("closure_arg"), closure_ty, "closure");
            auto && closure_stb_val = builder->CreatePointerCast(vs_table->lookup("closure_arg"), function_ptr_ptr, "clsstb");
            auto && closure_stb = builder->CreateAlloca(function_ptr_ptr, 0, fun_name);
            builder->CreateStore(closure_stb_val, closure_stb);
            {
                size_t index = 1;
                std::for_each(std::get<2>(f->value).begin(), std::get<2>(f->value).end(), [&lctx, &builder, &closure, &index] (auto & fv_ident) {
                        auto && fv_name = std::get<0>(fv_ident->value);
                        auto && fv_type = std::get<1>(fv_ident->value);
                        // printf("fv_load & store: %s\n", fv_name.first.c_str());
                        llvm::Type * t = convert_type(lctx, fv_type);
                        llvm::Value * fv_alloc = builder->CreateAlloca(t, 0, fv_name);
                        std::vector<Value *> idx = {llvm::ConstantInt::get(llvm::Type::getInt32Ty(lctx), 0),
                                                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(lctx), index++)};
                        auto && ptr = builder->CreateGEP(closure->getType(), closure, idx, "ptr_tmp");
                        auto && fv_load = builder->CreateLoad(ptr->getType(), ptr, "fv_load");
                        builder->CreateStore(fv_load, fv_alloc);
                    });
            }
        }
        Value * retval = pass(lctx, builder, module, func)(std::get<3>(f->value));
        // printf("function definition coded\n");
        Value * ret;
        if (!func->getReturnType()->isVoidTy() && retval) {
            ret = builder->CreateRet(retval);
        } else {
            ret = builder->CreateRetVoid();
        }
    }
}

struct prototype_pass {
    using result_type = void;
    LLVMContext & lctx;
    Module * module;

    prototype_pass(LLVMContext & l, Module * m) : lctx(l), module(m) { }

    template <typename T>
    result_type operator()(T) { }

    result_type operator()(const std::shared_ptr<closure::external> & e) {
        // register external variables to the module
        auto ident = std::get<0>(e->value);
        std::get<0>(ident->value) = std::get<1>(e->value);
        auto t = unwrap(std::get<1>(ident->value));
        if (std::get_if<sptr<type::function>>(&t)) {
            create_function_prototype(ident, lctx, module);
        } else {
            new llvm::GlobalVariable(*module, convert_type(lctx, std::get<1>(ident->value)),
                                     false, llvm::GlobalValue::ExternalLinkage,
                                     nullptr,
                                     std::get<0>(ident->value));
        }
    }

    result_type operator()(const std::shared_ptr<closure::global_rec> & f) {
        create_function_prototype(std::get<0>(f->value), lctx, module);
    }

    result_type operator()(const std::shared_ptr<closure::global> & g) {
        auto ident = std::get<0>(g->value);
        new llvm::GlobalVariable(*module, convert_type(lctx, std::get<1>(ident->value)),
                                 false, llvm::GlobalValue::ExternalLinkage,
                                 llvm::Constant::getNullValue(convert_type(lctx, std::get<1>(ident->value))),
                                 std::get<0>(ident->value));
    }

    result_type operator()(const std::shared_ptr<closure::global_tuple> & g) {
        std::for_each(std::get<0>(g->value).begin(), std::get<0>(g->value).end(), [this] (auto & ident) {
                new llvm::GlobalVariable(*module, convert_type(lctx, std::get<1>(ident->value)),
                                         false, llvm::GlobalValue::ExternalLinkage,
                                         llvm::Constant::getNullValue(convert_type(lctx, std::get<1>(ident->value))),
                                         std::get<0>(ident->value));
            });
    }

};

struct definition_pass {
    using result_type = void;
    LLVMContext & lctx;
    IRBuilder<> * builder;
    Module * module;

    definition_pass(LLVMContext & l, IRBuilder<> * b, Module * m) : lctx(l), builder(b), module(m) { }

    template <typename T>
    result_type operator()(T) { }

    result_type operator()(const std::shared_ptr<closure::global_rec> & f) {
        create_function_definition(f, lctx, module, builder);
    }

};

struct main_routine_pass {
    using result_type = Value *;
    LLVMContext & lctx;
    IRBuilder<> * builder;
    Module * module;
    Function * function;

    main_routine_pass(LLVMContext & l, IRBuilder<> * b, Module * m, Function * f) : lctx(l), builder(b), module(m), function(f) { }

    template <typename T>
    result_type operator()(T) { return nullptr; }

    result_type operator()(const closure::ast & ast) {
        return std::visit(pass(lctx, builder, module, function), ast);
    }

    result_type operator()(const std::shared_ptr<closure::global> & g) {
        auto ident = std::get<0>(g->value);
        Value * v = pass(lctx, builder, module, function)(std::get<1>(g->value));
        return builder->CreateStore(v, module->getGlobalVariable(std::get<0>(ident->value)));
    }

    result_type operator()(const std::shared_ptr<closure::global_tuple> & g) {
        Value * tuple = pass(lctx, builder, module, function)(std::get<1>(g->value));
        Value * ret;
        {
            size_t index = 0;
            std::for_each(std::get<0>(g->value).begin(), std::get<0>(g->value).end(), [this, &index, &ret, tuple] (auto & i) {
                    std::vector<Value *> t_index = {llvm::ConstantInt::get(llvm::Type::getInt32Ty(lctx), 0),
                                                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(lctx), index++)};
                    Value * ptr = builder->CreateGEP(tuple->getType(), tuple, t_index, "ptr_tmp");
                    Value * var = builder->CreateLoad(ptr->getType(), ptr, "var_tmp");
                    ret = builder->CreateStore(var, module->getGlobalVariable(std::get<0>(i->value)));
                });
        }
        return ret;
    }

};


Module * mcc::codegen::f(LLVMContext & lctx, mcc::context & ctx, const mcc::closure::module & mod) {
    IRBuilder<> * builder = new llvm::IRBuilder<>(lctx);
    Module * module = new llvm::Module(mod.module_name, lctx);

    // toplevel function prototype
    std::for_each(mod.value.rbegin(), mod.value.rend(), [&lctx, module] (auto & t) {
            std::visit(prototype_pass(lctx, module), t);
        });
    // toplevel function definition
    std::for_each(mod.value.begin(), mod.value.end(), [&lctx, module, builder] (auto & t) {
            std::visit(definition_pass(lctx, builder, module), t);
        });
    // main pass create
    auto && main_type = std::make_shared<type::function>(std::make_tuple(mod.module_type, std::vector<type::type_t>{ }), false);
    auto main_ident = std::make_shared<closure::identifier>("main", std::move(main_type));
    auto main_routine = create_function_prototype(main_ident, lctx, module);
    if (!main_routine) {
        assert(false);
    }
    auto bblock = llvm::BasicBlock::Create(lctx, "entry", main_routine);
    builder->SetInsertPoint(bblock);
    // main routine pass
    Value * retval = nullptr;
    std::for_each(mod.value.begin(), mod.value.end(), [&lctx, module, builder, main_routine, &retval] (auto & t) {
            retval = std::visit(main_routine_pass(lctx, builder, module, main_routine), t);
        });
    // main pass return
    if (!main_routine->getReturnType()->isVoidTy() && retval) {
        builder->CreateRet(retval);
    } else {
        builder->CreateRetVoid();
    }
    return module;
}
