#include "AvisoInstrumentBranches.h"
#include <llvm/Support/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CFG.h>
#include <set>
#include <cstdio>
#include <iostream>

using namespace llvm;

namespace llvm{
  ModulePass *createAvisoInstrumentBranches();
}

ModulePass *llvm::createAvisoInstrumentBranches() {
    return new AvisoInstrumentBranches();
}

namespace {
    static void registerAviso (const llvm::PassManagerBuilder &, llvm::PassManagerBase &PM) {
        PM.add(llvm::createAvisoInstrumentBranches());
    }

    static llvm::RegisterStandardPasses
        RegisterAviso(llvm::PassManagerBuilder::EP_EnabledOnOptLevel0, registerAviso);
}


AvisoInstrumentBranches::AvisoInstrumentBranches() : ModulePass(ID)
{ 

#ifdef AVISO_VERBOSE
  llvm::outs() << "Starting the Aviso Branch Instrumentation Transformation\n";
#endif 

}

AvisoInstrumentBranches::~AvisoInstrumentBranches (){ 

}

bool AvisoInstrumentBranches::doInitialization (Module &M){

    InstrumentedFunctions = std::set<BasicBlock *>();

    /*
    ArrayRef<Type *> a;
    FunctionType *f = FunctionType::get(Type::getVoidTy(M.getContext()), a, false);

    auto instfn = Function::Create(f, GlobalValue::ExternalLinkage, "IR_SyntheticEvent", &M);

    assert(instfn);
    */
 
    return false;

}

void AvisoInstrumentBranches::InstrumentBranchBlock(BasicBlock &B){

  if( InstrumentedFunctions.find(&B) != InstrumentedFunctions.end() ){  return;  }
  InstrumentedFunctions.insert(&B);

  Function *F = B.getParent();
  Module *M = F->getParent();

  IRBuilder<> Builder( M->getContext() );
  Constant *InstFuncConst = M->getOrInsertFunction("IR_SyntheticEvent",Type::getVoidTy(M->getContext()), (Type*)0);
  Function *InstFunc = dyn_cast<Function>( InstFuncConst );
  Builder.SetInsertPoint(&B,B.getFirstInsertionPt());
  Builder.CreateCall(InstFunc,"");

  outs() << "[Aviso] Instrumented Block\n" << B << "\n"; 
 
}

void AvisoInstrumentBranches::TraverseInstrumentingBranches(Function &F, BasicBlock &B, std::set<BasicBlock *> &visited){

  if( visited.find( &B ) != visited.end() ){ return; }
  auto nextVisited = std::set<BasicBlock *>(visited);
  nextVisited.insert(&B);
  
  
  InstrumentBranchBlock(B);

  
  auto TI = B.getTerminator();
  bool doInstrument = (TI->getNumSuccessors() > 1);


  for(succ_iterator SI = succ_begin(&B), E = succ_end(&B); SI != E; ++SI){

    BasicBlock &nB = **SI;
    if( doInstrument ){

      InstrumentBranchBlock(nB);

    }

    TraverseInstrumentingBranches(F, nB, nextVisited);

  } 

}

// Recognize stores to nonvolatile locations.
bool AvisoInstrumentBranches::runOnModule (Module &M)
{

    // For each Instruction in each Function, note whether it's a store to
    // a variable we just marked as nonvolatile.
    for (auto &F : M.getFunctionList()) {

      if( F.empty() ){ continue; }
      BasicBlock &entryBlock = F.front();
      auto visited = std::set<BasicBlock *>();
      TraverseInstrumentingBranches(F,entryBlock,visited);


    }

    return true;

}




void AvisoInstrumentBranches::getAnalysisUsage (AnalysisUsage &AU) const
{
  /*Modifies the CFG!*/

}

bool AvisoInstrumentBranches::doFinalization (Module &M)
{
    return false;
}

const char *AvisoInstrumentBranches::getPassName () const {
    return "Aviso Branch Instrumentation Transformation Pass";
}

char AvisoInstrumentBranches::ID = 0;

