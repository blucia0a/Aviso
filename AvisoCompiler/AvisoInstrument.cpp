#include <iostream>
#include <string>
#include <fstream>
#include <set>
#include <ext/hash_map>
using __gnu_cxx::hash_map;

#include <utility>

#define DEBUG_TYPE "aviso"
#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Instruction.h>
#include <llvm/Metadata.h>
#include <llvm/LLVMContext.h>
#include <llvm/Value.h>
#include <llvm/Type.h>
#include <llvm/Constants.h>
#include <llvm/Instructions.h>
#include <llvm/Module.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/Analysis/DebugInfo.h>
#include <llvm/Analysis/Dominators.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Transforms/Scalar.h>
using namespace llvm;


namespace {
  class AvisoPass : public FunctionPass{

  public:

    static char ID;

    DominatorTreeBase<BasicBlock>* DT;
    std::set< std::pair<std::string, unsigned int> > points;
    Constant *eventFunc;

    AvisoPass() : FunctionPass(ID), eventFunc(NULL) {
        DT = new DominatorTreeBase<BasicBlock>(false);
        char *pFile = getenv("AVISOPOINTS");
        std::ifstream reader(pFile);
        char filename[512];
        unsigned int line;
        while (reader.good()) {
            reader >> filename;
            reader >> line;
            std::pair<std::string, unsigned int> point(filename, line);
            points.insert(point);
        }
        std::cerr << "[AVISO] Instrumenting at " << points.size() << " points\n";
    }

    ~AvisoPass() {
        if (points.size() == 0) {
            std::cerr << "[AVISO] All points instrumented\n";
        } else {
            std::cerr << "[AVISO] These points not instrumented:\n";
            for (std::set< std::pair<std::string, unsigned int> >::iterator
                 i = points.begin(); i != points.end(); ++i) {
                std::cerr << "  " << i->first << ":" << i->second << "\n";
            }
        }
        delete DT;
    }


    bool dominates(const BasicBlock* A, const BasicBlock* B);
    bool properlyDominates(BasicBlock *A, BasicBlock *B);
    virtual bool doInitialization(Module &M);
    virtual bool runOnFunction(Function &F); 

};

}

char AvisoPass::ID = 0;
INITIALIZE_PASS(AvisoPass, "aviso", "Insert Aviso Instrumentation", false, false);

FunctionPass *llvm::createAvisoInstrumentationPass(){
  return new AvisoPass();
}

bool AvisoPass::doInitialization(Module &M) {
  
  eventFunc = M.getOrInsertFunction("IR_SyntheticEvent",
                                    Type::getVoidTy(M.getContext()),
                                    //Type::getInt64Ty(M.getContext()), 
                                    NULL);
  return false;
}

bool AvisoPass::dominates(const BasicBlock* A, const BasicBlock* B) {
  return DT->dominates(A, B);
}
  
bool AvisoPass::properlyDominates(BasicBlock *A, BasicBlock *B) {
  return DT->properlyDominates(A, B);
}

bool AvisoPass::runOnFunction(Function &F) {

  /*Two passes over the instructions.
 *  1a)Find the basic blocks associated with each of the file:line pairs.
 *  1b)For each block found, if it is dominated by another found, remove its corresponding file:line
 *  2)Iterate over the instructions and add calls to eventFunc
 */
  DT->recalculate(F);
  std::string fname;
  hash_map< unsigned int, Function::iterator > blks;
  for (Function::iterator b = F.begin(), be = F.end(); b != be; ++b) {
    for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {

      MDNode *md = i->getMetadata("dbg");
      DILocation loc(md);
      std::string filename = loc.getFilename().str();
      unsigned int line = loc.getLineNumber();
      if (filename.length() == 0){ // No source line.
        continue;
      }
      fname = filename;
      blks[line] = Function::iterator(*b); 
    }
  }

  unsigned num = 0;
  hash_map< unsigned int, Function::iterator >::iterator b,e;
  for(b = blks.begin(), e = blks.end(); b!=e; b++){

    hash_map< unsigned int, Function::iterator >::iterator ib,ie;
    for(ib = b, ie = blks.end(); ib!=ie; ib++){

      if( b == ib ){ continue; }

      if( dominates(&(*(b->second)),&(*(ib->second))) ){
        std::pair<std::string, unsigned int> point(fname, ib->first);
        if(points.find( std::pair<std::string, unsigned int>(fname, b->first)) != points.end()){
          if(points.erase(point)){
            std::cerr << "[AVISO] " << fname << " " << b->first << " dominates " << ib->first << "\n";
            num++;
          }
        }
      }

    }
    
  }

  if(num > 0){
    std::cerr << "[AVISO] Eliminated " << num << " events.\n";
  }
  
  for (Function::iterator b = F.begin(), be = F.end(); b != be; ++b) {
    for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {

      MDNode *md = i->getMetadata("dbg");
      DILocation loc(md);
      std::string filename = loc.getFilename().str();
      unsigned int line = loc.getLineNumber();
      if (filename.length() == 0){ // No source line.
        continue;
      }
      std::pair<std::string, unsigned int> point(filename, line);
      if (points.erase(point)) {
      
        std::cerr << "[AVISO] Instrumenting " << filename << ":" << line << "\n";
        std::vector<Value *> params = std::vector<Value *>();
        //BasicBlock::iterator r = BasicBlock::iterator(F.getEntryBlock().begin());
        CallInst::Create(eventFunc, params.begin(), params.end(), "", &(*i));

      }
    }
  }

  return true;


/*
  for (inst_iterator i = inst_begin(F); i != inst_end(F); ++i) {
      

    }
  }
  return false;
*/
}

