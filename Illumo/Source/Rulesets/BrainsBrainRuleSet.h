#pragma once
#include "RuleSet.h"

class BrainsBrainRuleSet : public RuleSet {
public:
	BrainsBrainRuleSet(Canvas* targetCanvas) : RuleSet(targetCanvas) {}
	~BrainsBrainRuleSet() override = default;

	unsigned char nextState(unsigned char cell, unsigned char aliveNeighbors) const override;
	void evalCell(const unsigned char& target, unsigned char dest[3]) const override;
	std::string getRuleTag() override { return "BRIANS_BRAIN"; }
};
