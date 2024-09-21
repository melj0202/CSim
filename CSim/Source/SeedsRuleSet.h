#pragma once 
#include "CellRuleSet.h"

class SeedsRuleSet : public CellRuleSet {
public:

	SeedsRuleSet() = default;
	~SeedsRuleSet() override = default;

	void evaluateNeighbors(unsigned char& cell, const unsigned char& ne, const int& x, const int& y) const override;
	void evalCell(const unsigned char& target, unsigned char dest[3]) const override;
};