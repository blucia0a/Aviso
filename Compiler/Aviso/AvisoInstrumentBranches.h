#ifndef _AVISO_INSERT_BRANCHES_H_
#define _AVISO_INSERT_BRANCHES_H_
#include <llvm/Pass.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Analysis/Dominators.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/PassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <set>

using namespace llvm;

class AvisoInstrumentBranches: public llvm::ModulePass {

    public:

        static char ID;
        AvisoInstrumentBranches();
        virtual ~AvisoInstrumentBranches();
        virtual const char *getPassName() const;

        virtual bool doInitialization (llvm::Module &M);
        virtual bool runOnModule (llvm::Module &M);
        virtual bool doFinalization (llvm::Module &M);
        virtual void getAnalysisUsage (llvm::AnalysisUsage &AU) const;

   private:
        std::set<BasicBlock *> InstrumentedFunctions;
        void TraverseInstrumentingBranches(Function &F, BasicBlock &B, std::set<BasicBlock *> &visited);
        void InstrumentBranchBlock(BasicBlock &B);

};
#endif //
