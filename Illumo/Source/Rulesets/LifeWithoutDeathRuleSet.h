#pragma once
#include "RuleSet.h"

class LifeWithoutDeathRuleSet : public RuleSet
{
public:
  LifeWithoutDeathRuleSet(Canvas* targetCanvas)
    : RuleSet(targetCanvas)
  {
  }
  ~LifeWithoutDeathRuleSet() override = default;

  unsigned char nextState(unsigned char cell,
                          unsigned char aliveNeighbors) const override;
  void evalCell(const unsigned char& target,
                unsigned char dest[3]) const override;
  std::string getRuleTag() override { return "LIFE_WITHOUT_DEATH"; }
};
