#pragma once
#include "RuleSet.h"

class HighlifeRuleSet : public RuleSet
{
public:
  HighlifeRuleSet(CellGrid* targetCanvas)
    : RuleSet(targetCanvas)
  {
  }
  ~HighlifeRuleSet() override = default;

  unsigned char nextState(unsigned char cell,
                          unsigned char aliveNeighbors) const override final;
  void evalCell(const unsigned char& target,
                unsigned char dest[3]) const override;
  std::string getRuleTag() override { return "HIGHLIFE"; }
};
