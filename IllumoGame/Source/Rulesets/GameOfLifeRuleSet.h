#pragma once
#include "RuleSet.h"

class GameOfLifeRuleSet : public RuleSet
{
public:
  GameOfLifeRuleSet(CellGrid* targetCanvas)
    : RuleSet(targetCanvas)
  {
  }
  ~GameOfLifeRuleSet() override = default;

  unsigned char nextState(unsigned char cell,
                          unsigned char aliveNeighbors) const override final;
  void evalCell(const unsigned char& target,
                unsigned char dest[3]) const override;
  std::string getRuleTag() override { return "GAME_OF_LIFE"; }
};
