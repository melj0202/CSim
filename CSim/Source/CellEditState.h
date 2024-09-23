#pragma once 
#include "CellState.h"

/*
	CellEditState
		Pauses the simulation, and allows the user to paint the canvas with cells or erase cells of the canvas.
*/
class CellEditState : public CellState {
public:
	~CellEditState() override = default;
	CellState* iterate(CellRuleSet* ruleSet, const char* filename, CellState* prevState) override;
};