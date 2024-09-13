#pragma once 
#include "CellRuleSet.h"




class BrainsBrainRuleSet: public CellRuleSet {
public:

	BrainsBrainRuleSet() = default;
	~BrainsBrainRuleSet() override = default;

	void calcGeneration(const int x_start, const int y_start, const int x_end, const int y_end, CellCanvas& canvas) const override;
};