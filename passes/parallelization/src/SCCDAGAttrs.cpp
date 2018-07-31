#include "SCCDAGAttrs.hpp"

using namespace llvm;

bool SCCDAGAttrs::doesHaveLoopCarriedDataDependences (void) const {
  for (auto &sccInfoPair : this->sccToInfo) {
    if (sccInfoPair.second->hasLoopCarriedDep) return true;
  }
  return false;
}

bool SCCDAGAttrs::loopHasInductionVariable (ScalarEvolution &SE) const {
  
  /*
   * Assumption(angelo): An induction variable will be the root SCC of the loop
   */
  auto topLevelNodes = sccdag->getTopLevelNodes();
  if (topLevelNodes.size() != 1) return false;

  auto scc = (*topLevelNodes.begin())->getT();
  return this->isInductionVariableSCC(SE, scc);
}

bool SCCDAGAttrs::isInductionVariableSCC (ScalarEvolution &SE, SCC *scc) const {
  auto isIvSCC = true;
  for (auto iNodePair : scc->internalNodePairs()) {
    auto V = iNodePair.first;
    auto canBePartOfSCC = isa<CmpInst>(V) || isa<TerminatorInst>(V);

    auto scev = SE.getSCEV(V);
    switch (scev->getSCEVType()) {
    case scConstant:
    case scTruncate:
    case scZeroExtend:
    case scSignExtend:
    case scAddExpr:
    case scMulExpr:
    case scUDivExpr:
    case scAddRecExpr:
    case scSMaxExpr:
    case scUMaxExpr:
      continue;
    case scUnknown:
    case scCouldNotCompute:
      isIvSCC &= canBePartOfSCC;
      continue;
    default:
     llvm_unreachable("DSWP: Unknown SCEV type!");
    }
  }
  return isIvSCC;
}

void SCCDAGAttrs::setSCCToHaveLoopCarriedDataDependence (SCC *scc, bool doesItHaveLoopCarriedDataDependence){
  auto &sccInfo = this->sccToInfo[scc];
  sccInfo->hasLoopCarriedDep = doesItHaveLoopCarriedDataDependence;

  return ;
}

std::set<BasicBlock *> & SCCDAGAttrs::getBasicBlocks (SCC *scc){
  auto &sccInfo = this->sccToInfo[scc];

  return sccInfo->bbs;
}

// REFACTOR(angelo): find better workaround than just a getter for SCCAttrs
std::unique_ptr<SCCAttrs> & SCCDAGAttrs::getSCCAttrs (SCC *scc){
  return this->sccToInfo[scc];
}

void SCCDAGAttrs::populate (SCCDAG *loopSCCDAG) {
  this->sccdag = loopSCCDAG;
  for (auto node : loopSCCDAG->getNodes()) {
    this->sccToInfo[node->getT()] = std::move(std::make_unique<SCCAttrs>(node->getT()));
  }

  return ;
}
