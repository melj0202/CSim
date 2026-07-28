#pragma once
#include "RuleSet.h"

class DayAndNightRuleSet : public RuleSet {
public:
	DayAndNightRuleSet(Canvas* targetCanvas) : RuleSet(targetCanvas) {}
	~DayAndNightRuleSet() override = default;

	unsigned char nextState(unsigned char cell, unsigned char aliveNeighbors) const override;
	void evalCell(const unsigned char& target, unsigned char dest[3]) const override;
	std::string getRuleTag() override { return "DAY_AND_NIGHT"; }
};
