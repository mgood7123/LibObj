//===- Solver.h -------------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines an interface for a SAT solver that can be used by
//  dataflow analyses.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_ANALYSIS_FLOWSENSITIVE_SOLVER_H
#define LLVM_CLANG_ANALYSIS_FLOWSENSITIVE_SOLVER_H

#include "clang/Analysis/FlowSensitive/Value.h"
#include "clang/Basic/LLVM.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/Compiler.h"
#include <optional>
#include <vector>

namespace clang {
namespace dataflow {

/// An interface for a SAT solver that can be used by dataflow analyses.
class Solver {
public:
  struct Result {
    enum class Status {
      /// Indicates that there exists a satisfying assignment for a boolean
      /// formula.
      Satisfiable,

      /// Indicates that there is no satisfying assignment for a boolean
      /// formula.
      Unsatisfiable,

      /// Indicates that the solver gave up trying to find a satisfying
      /// assignment for a boolean formula.
      TimedOut,
    };

    /// A boolean value is set to true or false in a truth assignment.
    enum class Assignment : uint8_t { AssignedFalse = 0, AssignedTrue = 1 };

    /// Constructs a result indicating that the queried boolean formula is
    /// satisfiable. The result will hold a solution found by the solver.
    static Result
    Satisfiable(llvm::DenseMap<AtomicBoolValue *, Assignment> Solution) {
      return Result(Status::Satisfiable, std::move(Solution));
    }

    /// Constructs a result indicating that the queried boolean formula is
    /// unsatisfiable.
    static Result Unsatisfiable() { return Result(Status::Unsatisfiable, {}); }

    /// Constructs a result indicating that satisfiability checking on the
    /// queried boolean formula was not completed.
    static Result TimedOut() { return Result(Status::TimedOut, {}); }

    /// Returns the status of satisfiability checking on the queried boolean
    /// formula.
    Status getStatus() const { return SATCheckStatus; }

    /// Returns a truth assignment to boolean values that satisfies the queried
    /// boolean formula if available. Otherwise, an empty optional is returned.
    std::optional<llvm::DenseMap<AtomicBoolValue *, Assignment>>
    getSolution() const {
      return Solution;
    }

  private:
    Result(
        enum Status SATCheckStatus,
        std::optional<llvm::DenseMap<AtomicBoolValue *, Assignment>> Solution)
        : SATCheckStatus(SATCheckStatus), Solution(std::move(Solution)) {}

    Status SATCheckStatus;
    std::optional<llvm::DenseMap<AtomicBoolValue *, Assignment>> Solution;
  };

  virtual ~Solver() = default;

  /// Checks if the conjunction of `Vals` is satisfiable and returns the
  /// corresponding result.
  ///
  /// Requirements:
  ///
  ///  All elements in `Vals` must not be null.
  virtual Result solve(llvm::ArrayRef<BoolValue *> Vals) = 0;

  LLVM_DEPRECATED("Pass ArrayRef for determinism", "")
  virtual Result solve(llvm::DenseSet<BoolValue *> Vals) {
    return solve(ArrayRef(std::vector<BoolValue *>(Vals.begin(), Vals.end())));
  }
};

} // namespace dataflow
} // namespace clang

#endif // LLVM_CLANG_ANALYSIS_FLOWSENSITIVE_SOLVER_H
