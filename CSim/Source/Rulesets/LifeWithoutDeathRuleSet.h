#pragma once 
#include "RuleSet.h"




class LifeWithoutDeathRuleSet: public RuleSet {
public:

	LifeWithoutDeathRuleSet(Canvas* targetCanvas) : RuleSet(targetCanvas) {}
	~LifeWithoutDeathRuleSet() override = default;

	void evaluateNeighbors(unsigned char& cell, const unsigned char& ne, const int& x, const int& y) const override;
	void evalCell(const unsigned char& target, unsigned char dest[3]) const override;
	std::string getRuleTag() override {
		return "LIFE_WITHOUT_DEATH";
	}
};