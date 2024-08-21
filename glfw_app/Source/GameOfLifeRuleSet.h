#pragma once 
#include "CellRuleSet.h"

class GameOfLifeRuleSet : public CellRuleSet {
public:
	struct CellStates {
		const std::vector<unsigned char> CELL_DEAD = { 0, 0, 0 };
		const std::vector<unsigned char> CELL_ALIVE = { 255, 255, 255 };
	};
	GameOfLifeRuleSet() = default;
	~GameOfLifeRuleSet() override = default;
	static CellStates lifeStates;
	void calcGeneration(const int x_start, const int y_start, const int x_end, const int y_end, CellCanvas& canvas) const override;
};