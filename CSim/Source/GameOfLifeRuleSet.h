#pragma once 
#include "CellRuleSet.h"




class GameOfLifeRuleSet : public CellRuleSet {
public:
	
	GameOfLifeRuleSet() = default;
	~GameOfLifeRuleSet() override = default;
	
	void calcGeneration(const int x_start, const int y_start, const int x_end, const int y_end) const override;
	void evalCell(const unsigned char& target, unsigned char dest[3]) const override;
};