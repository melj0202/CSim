#pragma once
#include "CellCanvas.h"

constexpr auto BLOCK_X = 2;
constexpr auto BLOCK_Y = 2;

class CellRuleSet {
public:
	
	CellRuleSet() = default;
	virtual ~CellRuleSet() = default;
	virtual void calcGeneration(const int x_start, const int y_start, const int x_end, const int y_end, CellCanvas& canvas) const { /*nothing*/ };
protected:
	constexpr int countNeighbors(const int r, const int c, const int w, const int h, CellCanvas& canvas) const;
};