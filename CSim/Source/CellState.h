#pragma once
#include "MacroDefs.h"
#include "Rulesets/CellRuleSet.h"

/*
	CellState
		This class defines the main loop behavior. Inherit and override the class functions to define main loop behavior...

		DO NOT EVER CALL THE BASECLASS DIRECTLY
*/
class CellState {
public: 
	virtual ~CellState() = default;
	virtual void iterate(CellRuleSet& ruleSet) {/*Baseclass does nothing...*/ };
};