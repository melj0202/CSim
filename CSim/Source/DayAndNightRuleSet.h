#pragma once 
#include "CellRuleSet.h"




class DayAndNightRuleSet : public CellRuleSet {
public:

	DayAndNightRuleSet() = default;
	~DayAndNightRuleSet() override = default;

	void evaluateNeighbors(unsigned char& cell, const unsigned char &ne, const int& x, const int& y) const override;
	void evalCell(const unsigned char& target, unsigned char dest[3]) const override;
};