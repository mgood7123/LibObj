//===- LowerWidenableCondition.cpp - Lower the guard intrinsic ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This pass lowers the llvm.widenable.condition intrinsic to default value
// which is i1 true.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Scalar/LowerWidenableCondition.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Scalar.h"

using namespace LIBOBJ_llvm;

namespace {
struct LowerWidenableConditionLegacyPass : public FunctionPass {
  static char ID;
  LowerWidenableConditionLegacyPass() : FunctionPass(ID) {
    initializeLowerWidenableConditionLegacyPassPass(
        *PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override;
};
}

static bool lowerWidenableCondition(Function &F) {
  // Check if we can cheaply rule out the possibility of not having any work to
  // do.
  auto *WCDecl = F.getParent()->getFunction(
      Intrinsic::getName(Intrinsic::experimental_widenable_condition));
  if (!WCDecl || WCDecl->use_empty())
    return false;

  using namespace LIBOBJ_llvm::PatternMatch;
  SmallVector<CallInst *, 8> ToLower;
  // Traverse through the users of WCDecl.
  // This is presumably cheaper than traversing all instructions in the
  // function.
  for (auto *U : WCDecl->users())
    if (auto *CI = dyn_cast<CallInst>(U))
      if (CI->getFunction() == &F)
        ToLower.push_back(CI);

  if (ToLower.empty())
    return false;

  for (auto *CI : ToLower) {
    CI->replaceAllUsesWith(ConstantInt::getTrue(CI->getContext()));
    CI->eraseFromParent();
  }
  return true;
}

bool LowerWidenableConditionLegacyPass::runOnFunction(Function &F) {
  return lowerWidenableCondition(F);
}

char LowerWidenableConditionLegacyPass::ID = 0;
INITIALIZE_PASS(LowerWidenableConditionLegacyPass, "lower-widenable-condition",
                "Lower the widenable condition to default true value", false,
                false)

Pass *llvm::createLowerWidenableConditionPass() {
  return new LowerWidenableConditionLegacyPass();
}

PreservedAnalyses LowerWidenableConditionPass::run(Function &F,
                                               FunctionAnalysisManager &AM) {
  if (lowerWidenableCondition(F))
    return PreservedAnalyses::none();

  return PreservedAnalyses::all();
}
