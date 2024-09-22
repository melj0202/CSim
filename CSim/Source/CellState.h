#pragma once
#include "MacroDefs.h"
#include "Rulesets/CellRuleSet.h"

/*
	CellState
		This class defines the main loop behavior. Inherit and override the class functions to define main loop behavior...

		DO NOT EVER CALL THE BASECLASS DIRECTLY

  		TODO: Add extra data to this class so that a state may switch back to a previous state if needed.

    		This would allow for the load/save states to go back into edit mode when done.
*/
class CellState {
public: 
	virtual ~CellState() = default;
	virtual void iterate(CellRuleSet& ruleSet) {/*Baseclass does nothing...*/ };
};
