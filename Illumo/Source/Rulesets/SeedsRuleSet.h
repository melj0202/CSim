#pragma once
#include "RuleSet.h"

class SeedsRuleSet : public RuleSet
{
public:
  SeedsRuleSet(CellGrid* targetCanvas)
    : RuleSet(targetCanvas)
  {
  }
  ~SeedsRuleSet() override = default;

  unsigned char nextState(unsigned char cell,
                          unsigned char aliveNeighbors) const override final;
  void evalCell(const unsigned char& target,
                unsigned char dest[3]) const override;
  std::string getRuleTag() override { return "SEEDS"; }
};
