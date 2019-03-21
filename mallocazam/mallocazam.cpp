// mallocazam.cpp
// llvm pass to deduce types of allocations
// Brandon Kammerdiener -- 2019

#include <llvm/Config/llvm-config.h>
#if LLVM_VERSION_MAJOR >= 4
/* Required for CompassQuickExit, requires LLVM 4.0 or newer */
#include "llvm/Bitcode/BitcodeWriter.h"
#endif
#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Operator.h"
#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <stdio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <vector>

using namespace llvm;

namespace {

struct mallocazam : public ModulePass {
    static char ID;

    std::map<std::string, Value*> string_value_map;
    IRBuilder<> * builder;

    mallocazam() : ModulePass(ID) {}

    void getAnalysisUsage(AnalysisUsage & AU) const { }

    Function * defineCastCatcherFunction(Module * module_ptr) {
        Type * pointer_type = Type::getInt8PtrTy(module_ptr->getContext());

        std::vector<Type*> param_types { pointer_type, pointer_type };

        FunctionType * fn_type = FunctionType::get(Type::getVoidTy(module_ptr->getContext()), param_types, false);

        Function * fn = Function::Create(fn_type, GlobalValue::LinkageTypes::WeakODRLinkage, "mallocazam", module_ptr);

        llvm::BasicBlock * entry = llvm::BasicBlock::Create(module_ptr->getContext(), "mallocazam_entry", fn);

        fn->addFnAttr(Attribute::NoInline);
        fn->addFnAttr(Attribute::OptimizeNone);

        builder->SetInsertPoint(entry);

        builder->CreateRetVoid();

        return fn;
    }

    Value * getOrCreateGlobalTypeString(Type * type, Module& theModule) {
        std::string str;
        raw_string_ostream rso(str);

        rso << *type;
        rso.flush();

        if (string_value_map.count(str)) {
            return string_value_map[str];
        }

        llvm::ArrayType * var_t = ArrayType::get(Type::getInt8Ty(theModule.getContext()), str.size() + 1);
        llvm::GlobalVariable * global_var = new GlobalVariable(
            /*Module=*/theModule,
            /*Type=*/var_t,
            /*isConstant=*/true,
            /*Linkage=*/GlobalValue::PrivateLinkage,
            /*Initializer=*/0, // has initializer, specified below
            /*Name=*/".str");
        global_var->setAlignment(1);
        global_var->setUnnamedAddr(GlobalVariable::UnnamedAddr::Global);

        llvm::Constant * str_constant =
            ConstantDataArray::getString(theModule.getContext(), str.c_str(), true);
        global_var->setInitializer(str_constant);

        return string_value_map[str] = global_var;
    }

    //////////////////////////////////////////////////////////////////// runOnModule
    ////////////////////////////////////////////////////////////////////////////////
    bool runOnModule(Module & theModule) {
        IRBuilder<> my_builder(theModule.getContext());
        builder = &my_builder;

        Function * fn = defineCastCatcherFunction(&theModule);

        for (auto& F : theModule) {
            for (auto& BB : F) {
                std::vector<BitCastInst*> casts;
                for (auto& I : BB) {
                    if (BitCastInst * cast = dyn_cast<BitCastInst>(&I)) {
                        if (cast->getType()->isPointerTy()               /* resulting type is a pointer */
                        && cast->getOperand(0)->getType()->isPointerTy() /* operand type is a pointer */
                        &&  !isa<AllocaInst>(*cast->getOperand(0))) {
                            casts.push_back(cast);
                        }
                    }
                }
                for (BitCastInst * cast : casts) {
                    Value * arg0    = nullptr;
                    Value * arg1    = nullptr;
                    Type * ptr_type = Type::getInt8PtrTy(theModule.getContext());

                    builder->SetInsertPoint(cast);

                    arg0 = cast->getOperand(0);
                    if (arg0->getType() != ptr_type) {
                        arg0 = builder->CreateBitCast(arg0, ptr_type);
                    }

                    arg1 = getOrCreateGlobalTypeString(cast->getType()->getPointerElementType(), theModule);
                    arg1 = builder->CreateInBoundsGEP(
                                arg1,
                                { builder->getInt32(0)
                                , builder->getInt32(0) });

                    builder->CreateCall(fn, { arg0, arg1 });
                }
            }
        }

        return true;
    }
    ////////////////////////////////////////////////////////////////////////////////
};

} // namespace
char mallocazam::ID = 0;

static void registerMyPass(const PassManagerBuilder &,
                           legacy::PassManagerBase &PM) {
    PM.add(new mallocazam());
}

static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_ModuleOptimizerEarly, registerMyPass);
static RegisterStandardPasses RegisterMyPass0(PassManagerBuilder::EP_EnabledOnOptLevel0, registerMyPass);
