#pragma once
#include "Init/MacroDefs.h"
#include "Rulesets/RuleSet.h"
/*
	State
		This class defines the main loop behavior. Inherit and override the class functions to define main loop behavior...

		DO NOT EVER CALL THE BASECLASS DIRECTLY

  		TODO: Add extra data to this class so that a state may switch back to a previous state if needed.

    		This would allow for the load/save states to go back into edit mode when done.
*/

class State {
public: 
	virtual ~State() = default;
	virtual State* iterate(RuleSet* ruleSet, const char* filename, State* prevState) { return prevState; };
};
