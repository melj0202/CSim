#pragma once
#include "MacroDefs.h"
#include "CellRuleSet.h"

class CellState {
public: 
	virtual ~CellState() = default;
	virtual void iterate(CellRuleSet& ruleSet) {/*Baseclass does nothing...*/ };
};