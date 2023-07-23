//===---- BDCE.cpp - Bit-tracking dead code elimination -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the Bit-Tracking Dead Code Elimination pass. Some
// instructions (shifts, some ands, ors, etc.) kill some of their input bits.
// We track these dead bits and remove instructions that compute only these
// dead bits. We also simplify sext that generates unused extension bits,
// converting it to a zext.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Scalar/BDCE.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/DemandedBits.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Local.h"
using namespace LIBOBJ_llvm;

#define DEBUG_TYPE "bdce"

STATISTIC(NumRemoved, "Number of instructions removed (unused)");
STATISTIC(NumSimplified, "Number of instructions trivialized (dead bits)");
STATISTIC(NumSExt2ZExt,
          "Number of sign extension instructions converted to zero extension");

/// If an instruction is trivialized (dead), then the chain of users of that
/// instruction may need to be cleared of assumptions that can no longer be
/// guaranteed correct.
static void clearAssumptionsOfUsers(Instruction *I, DemandedBits &DB) {
  assert(I->getType()->isIntOrIntVectorTy() &&
         "Trivializing a non-integer value?");

  // Initialize the worklist with eligible direct users.
  SmallPtrSet<Instruction *, 16> Visited;
  SmallVector<Instruction *, 16> WorkList;
  for (User *JU : I->users()) {
    // If all bits of a user are demanded, then we know that nothing below that
    // in the def-use chain needs to be changed.
    auto *J = dyn_cast<Instruction>(JU);
    if (J && J->getType()->isIntOrIntVectorTy() &&
        !DB.getDemandedBits(J).isAllOnes()) {
      Visited.insert(J);
      WorkList.push_back(J);
    }

    // Note that we need to check for non-int types above before asking for
    // demanded bits. Normally, the only way to reach an instruction with an
    // non-int type is via an instruction that has side effects (or otherwise
    // will demand its input bits). However, if we have a readnone function
    // that returns an unsized type (e.g., void), we must avoid asking for the
    // demanded bits of the function call's return value. A void-returning
    // readnone function is always dead (and so we can stop walking the use/def
    // chain here), but the check is necessary to avoid asserting.
  }

  // DFS through subsequent users while tracking visits to avoid cycles.
  while (!WorkList.empty()) {
    Instruction *J = WorkList.pop_back_val();

    // NSW, NUW, and exact are based on operands that might have changed.
    J->dropPoisonGeneratingFlags();

    // We do not have to worry about llvm.assume or range metadata:
    // 1. llvm.assume demands its operand, so trivializing can't change it.
    // 2. range metadata only applies to memory accesses which demand all bits.

    for (User *KU : J->users()) {
      // If all bits of a user are demanded, then we know that nothing below
      // that in the def-use chain needs to be changed.
      auto *K = dyn_cast<Instruction>(KU);
      if (K && Visited.insert(K).second && K->getType()->isIntOrIntVectorTy() &&
          !DB.getDemandedBits(K).isAllOnes())
        WorkList.push_back(K);
    }
  }
}

static bool bitTrackingDCE(Function &F, DemandedBits &DB) {
  SmallVector<Instruction*, 128> Worklist;
  bool Changed = false;
  for (Instruction &I : instructions(F)) {
    // If the instruction has side effects and no non-dbg uses,
    // skip it. This way we avoid computing known bits on an instruction
    // that will not help us.
    if (I.mayHaveSideEffects() && I.use_empty())
      continue;

    // Remove instructions that are dead, either because they were not reached
    // during analysis or have no demanded bits.
    if (DB.isInstructionDead(&I) ||
        (I.getType()->isIntOrIntVectorTy() && DB.getDemandedBits(&I).isZero() &&
         wouldInstructionBeTriviallyDead(&I))) {
      Worklist.push_back(&I);
      Changed = true;
      continue;
    }

    // Convert SExt into ZExt if none of the extension bits is required
    if (SExtInst *SE = dyn_cast<SExtInst>(&I)) {
      APInt Demanded = DB.getDemandedBits(SE);
      const uint32_t SrcBitSize = SE->getSrcTy()->getScalarSizeInBits();
      auto *const DstTy = SE->getDestTy();
      const uint32_t DestBitSize = DstTy->getScalarSizeInBits();
      if (Demanded.countl_zero() >= (DestBitSize - SrcBitSize)) {
        clearAssumptionsOfUsers(SE, DB);
        IRBuilder<> Builder(SE);
        I.replaceAllUsesWith(
            Builder.CreateZExt(SE->getOperand(0), DstTy, SE->getName()));
        Worklist.push_back(SE);
        Changed = true;
        NumSExt2ZExt++;
        continue;
      }
    }

    for (Use &U : I.operands()) {
      // DemandedBits only detects dead integer uses.
      if (!U->getType()->isIntOrIntVectorTy())
        continue;

      if (!isa<Instruction>(U) && !isa<Argument>(U))
        continue;

      if (!DB.isUseDead(&U))
        continue;

      LLVM_DEBUG(dbgs() << "BDCE: Trivializing: " << U << " (all bits dead)\n");

      clearAssumptionsOfUsers(&I, DB);

      // Substitute all uses with zero. In theory we could use `freeze poison`
      // instead, but that seems unlikely to be profitable.
      U.set(ConstantInt::get(U->getType(), 0));
      ++NumSimplified;
      Changed = true;
    }
  }

  for (Instruction *&I : llvm::reverse(Worklist)) {
    salvageDebugInfo(*I);
    I->dropAllReferences();
  }

  for (Instruction *&I : Worklist) {
    ++NumRemoved;
    I->eraseFromParent();
  }

  return Changed;
}

PreservedAnalyses BDCEPass::run(Function &F, FunctionAnalysisManager &AM) {
  auto &DB = AM.getResult<DemandedBitsAnalysis>(F);
  if (!bitTrackingDCE(F, DB))
    return PreservedAnalyses::all();

  PreservedAnalyses PA;
  PA.preserveSet<CFGAnalyses>();
  return PA;
}