#pragma once
#include "RuleSet.h"

// Stub elementary CA (not yet fully implemented).
// Identity nextState keeps grid unchanged when selected.
class Rule90RuleSet : public RuleSet {
public:
	Rule90RuleSet(Canvas* targetCanvas) : RuleSet(targetCanvas) {}
	~Rule90RuleSet() override = default;

	std::string getRuleTag() override { return "RULE_90"; }
};
