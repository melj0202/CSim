#pragma once 
#include "RuleSet.h"

class BrainsBrainRuleSet: public RuleSet {
public:

	BrainsBrainRuleSet(Canvas* targetCanvas) : RuleSet(targetCanvas) {}
	~BrainsBrainRuleSet() override = default;

	void evaluateNeighbors(unsigned char& cell, const unsigned char& ne, const int& x, const int& y) const override;
	void evalCell(const unsigned char& target, unsigned char dest[3]) const override;
	std::string getRuleTag() override {
		return "BRIANS_BRAIN";
	}
};