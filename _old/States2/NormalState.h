#pragma once 
#include "State.h"

/*
	NormalState
		The normal state valuates the cell canvas and calculates the next generation of cells based on the currents cells in the canvas.
*/
class NormalState : public State {
public:
	State* iterate(RuleSet* ruleSet, const char* filename, State* prevState) override;
	~NormalState() override = default;
};