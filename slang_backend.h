/*
 * Copyright 2010, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _FRAMEWORKS_COMPILE_SLANG_SLANG_BACKEND_H_  // NOLINT
#define _FRAMEWORKS_COMPILE_SLANG_SLANG_BACKEND_H_

#include "clang/AST/ASTConsumer.h"

#include "llvm/IR/LegacyPassManager.h"

#include "llvm/Support/raw_ostream.h"

#include "slang.h"
#include "slang_pragma_recorder.h"
#include "slang_rs_check_ast.h"
#include "slang_rs_object_ref_count.h"
#include "slang_version.h"

namespace llvm {
  class buffer_ostream;
  class LLVMContext;
  class NamedMDNode;
  class Module;
}

namespace clang {
  class ASTConsumer;
  class ASTContext;
  class CodeGenOptions;
  class CodeGenerator;
  class DeclGroupRef;
  class DiagnosticsEngine;
  class FunctionDecl;
  class TagDecl;
  class TargetOptions;
  class VarDecl;
}

namespace slang {

class RSContext;

class Backend : public clang::ASTConsumer {
 private:
  const clang::TargetOptions &mTargetOpts;

  llvm::Module *mpModule;

  // Output stream
  llvm::raw_ostream *mpOS;
  Slang::OutputType mOT;

  // This helps us translate Clang AST using into LLVM IR
  clang::CodeGenerator *mGen;

  // Passes

  // Passes apply on function scope in a translation unit
  llvm::legacy::FunctionPassManager *mPerFunctionPasses;
  // Passes apply on module scope
  llvm::legacy::PassManager *mPerModulePasses;
  // Passes for code emission
  llvm::legacy::FunctionPassManager *mCodeGenPasses;

  llvm::buffer_ostream mBufferOutStream;

  void CreateFunctionPasses();
  void CreateModulePasses();
  bool CreateCodeGenPasses();

  void WrapBitcode(llvm::raw_string_ostream &Bitcode);

  RSContext *mContext;

  clang::SourceManager &mSourceMgr;

  bool mAllowRSPrefix;

  bool mIsFilterscript;

  llvm::NamedMDNode *mExportVarMetadata;
  llvm::NamedMDNode *mExportFuncMetadata;
  llvm::NamedMDNode *mExportForEachNameMetadata;
  llvm::NamedMDNode *mExportForEachSignatureMetadata;
  llvm::NamedMDNode *mExportTypeMetadata;
  llvm::NamedMDNode *mRSObjectSlotsMetadata;

  RSObjectRefCount mRefCount;

  RSCheckAST mASTChecker;

  void AnnotateFunction(clang::FunctionDecl *FD);

  void dumpExportVarInfo(llvm::Module *M);
  void dumpExportFunctionInfo(llvm::Module *M);
  void dumpExportForEachInfo(llvm::Module *M);
  void dumpExportTypeInfo(llvm::Module *M);

 protected:
  llvm::LLVMContext &mLLVMContext;
  clang::DiagnosticsEngine &mDiagEngine;
  const clang::CodeGenOptions &mCodeGenOpts;

  PragmaList *mPragmas;

  unsigned int getTargetAPI() const { return mContext->getTargetAPI(); }

  // TODO These are no longer virtual from base.  Look into merging into caller.

  // This handler will be invoked before Clang translates @Ctx to LLVM IR. This
  // give you an opportunity to modified the IR in AST level (scope information,
  // unoptimized IR, etc.). After the return from this method, slang will start
  // translate @Ctx into LLVM IR. One should not operate on @Ctx afterwards
  // since the changes applied on that never reflects to the LLVM module used
  // in the final codegen.
  void HandleTranslationUnitPre(clang::ASTContext &Ctx);

  // This handler will be invoked when Clang have converted AST tree to LLVM IR.
  // The @M contains the resulting LLVM IR tree. After the return from this
  // method, slang will start doing optimization and code generation for @M.
  void HandleTranslationUnitPost(llvm::Module *M);

 public:
  Backend(RSContext *Context,
            clang::DiagnosticsEngine *DiagEngine,
            const clang::CodeGenOptions &CodeGenOpts,
            const clang::TargetOptions &TargetOpts,
            PragmaList *Pragmas,
            llvm::raw_ostream *OS,
            Slang::OutputType OT,
            clang::SourceManager &SourceMgr,
            bool AllowRSPrefix,
            bool IsFilterscript);

  virtual ~Backend();

  // Initialize - This is called to initialize the consumer, providing the
  // ASTContext.
  void Initialize(clang::ASTContext &Ctx) override;

  // TODO Clean up what should be private, protected
  // TODO Also clean up the include files

  // HandleTopLevelDecl - Handle the specified top-level declaration.  This is
  // called by the parser to process every top-level Decl*. Note that D can be
  // the head of a chain of Decls (e.g. for `int a, b` the chain will have two
  // elements). Use Decl::getNextDeclarator() to walk the chain.
  bool HandleTopLevelDecl(clang::DeclGroupRef D) override;

  // HandleTranslationUnit - This method is called when the ASTs for entire
  // translation unit have been parsed.
  void HandleTranslationUnit(clang::ASTContext &Ctx) override;

  // HandleTagDeclDefinition - This callback is invoked each time a TagDecl
  // (e.g. struct, union, enum, class) is completed.  This allows the client to
  // hack on the type, which can occur at any point in the file (because these
  // can be defined in declspecs).
  void HandleTagDeclDefinition(clang::TagDecl *D) override;

  // CompleteTentativeDefinition - Callback invoked at the end of a translation
  // unit to notify the consumer that the given tentative definition should be
  // completed.
  void CompleteTentativeDefinition(clang::VarDecl *D) override;
};

}   // namespace slang

#endif  // _FRAMEWORKS_COMPILE_SLANG_SLANG_BACKEND_H_  NOLINT
