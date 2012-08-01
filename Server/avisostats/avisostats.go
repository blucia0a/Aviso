package avisostats

func ComputeChiSquare(numFail float64,numSucc float64,baseFail float64, baseSucc float64) float64{

  /*compute expected values for numFail, numSucc, baseFail, baseSucc*/

  //Contingency Table:
  //        fail                succ                total
  //base   baseFail             baseSucc            baseSucc + baseFail 
  //fsm    numFail              numSucc             numFail + numSucc 
  //tot    baseFail+numFail     baseSucc+numSucc    baseFail+numFail+
  //                                                baseSucc+numSucc

  //         fail                succ               total
  //base                                           baseRowTotal
  //fsm                                            fsmRowTotal 
  //tot   failColTotal        succColTotal         allTotal 

  baseRowTotal := baseSucc + baseFail
  fsmRowTotal := numFail + numSucc
  failColTotal := baseFail + numFail
  succColTotal := baseSucc + numSucc

  allTotal := failColTotal + succColTotal

  expectedBaseFail := failColTotal * baseRowTotal / allTotal
  expectedBaseSucc := succColTotal * baseRowTotal / allTotal
  expectedFsmFail  := failColTotal * fsmRowTotal / allTotal
  expectedFsmSucc  := succColTotal * fsmRowTotal / allTotal

  chiSquared := (( baseFail - expectedBaseFail ) * ( baseFail - expectedBaseFail) / expectedBaseFail) + (( baseSucc - expectedBaseSucc ) * ( baseSucc - expectedBaseSucc) / expectedBaseSucc) + (( numFail - expectedFsmFail ) * ( numFail - expectedFsmFail )    / expectedFsmFail ) + (( numSucc - expectedFsmSucc ) * ( numSucc - expectedFsmSucc )    / expectedFsmSucc )

  return chiSquared

}

