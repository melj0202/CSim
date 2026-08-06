#pragma once
#include "RuleSet.h"

// Stub elementary CA (not yet fully implemented).
class Rule184RuleSet : public RuleSet
{
public:
  Rule184RuleSet(Canvas* targetCanvas)
    : RuleSet(targetCanvas)
  {
  }
  ~Rule184RuleSet() override = default;

  std::string getRuleTag() override { return "RULE_184"; }
};
