#pragma once
#include "CellCanvas.h"

constexpr auto BLOCK_X = 2;
constexpr auto BLOCK_Y = 2;

class CellRuleSet {
public:
	struct CellStates {
		const std::vector<unsigned char> CELL_DEAD = { 0, 0, 0 };
		const std::vector<unsigned char> CELL_ALIVE = { 255, 255, 255 };
	};
	CellRuleSet() = default;
	virtual ~CellRuleSet() = default;
	virtual void calcGeneration() const { /*nothing*/ };
	static CellStates ruleSetStates;
protected:
	constexpr int countNeighbors(const int r, const int c, const int w, const int h, CellCanvas& canvas) const;
};