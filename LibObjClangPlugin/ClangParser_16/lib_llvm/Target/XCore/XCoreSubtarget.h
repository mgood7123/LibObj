//===-- XCoreSubtarget.h - Define Subtarget for the XCore -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the XCore specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XCORE_XCORESUBTARGET_H
#define LLVM_LIB_TARGET_XCORE_XCORESUBTARGET_H

#include "XCoreFrameLowering.h"
#include "XCoreISelLowering.h"
#include "XCoreInstrInfo.h"
#include "XCoreSelectionDAGInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetMachine.h"
#include <string>

#define GET_SUBTARGETINFO_HEADER
#include "XCoreGenSubtargetInfo.inc"

namespace LIBOBJ_llvm {
class StringRef;

class XCoreSubtarget : public XCoreGenSubtargetInfo {
  virtual void anchor();
  XCoreInstrInfo InstrInfo;
  XCoreFrameLowering FrameLowering;
  XCoreTargetLowering TLInfo;
  XCoreSelectionDAGInfo TSInfo;

public:
  /// This constructor initializes the data members to match that
  /// of the specified triple.
  ///
  XCoreSubtarget(const Triple &TT, const std::string &CPU,
                 const std::string &FS, const TargetMachine &TM);

  /// ParseSubtargetFeatures - Parses features string setting specified
  /// subtarget options.  Definition of function is auto generated by tblgen.
  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const XCoreInstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const XCoreFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const XCoreTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const XCoreSelectionDAGInfo *getSelectionDAGInfo() const override {
    return &TSInfo;
  }
  const TargetRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
};
} // End llvm namespace

#endif
