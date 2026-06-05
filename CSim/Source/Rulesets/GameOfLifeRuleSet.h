#pragma once 
#include "RuleSet.h"




class GameOfLifeRuleSet : public RuleSet {
public:
	
	GameOfLifeRuleSet(Canvas* targetCanvas) : RuleSet(targetCanvas) {}
	~GameOfLifeRuleSet() override = default;
	
	void evaluateNeighbors(unsigned char& cell, const unsigned char& ne, const int& x, const int& y) const override;
	void evalCell(const unsigned char& target, unsigned char dest[3]) const override;
	std::string getRuleTag() override {
		return "GAME_OF_LIFE";
	}
};