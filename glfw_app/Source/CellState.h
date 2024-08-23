#pragma once
#include "CellCanvas.h"
#include "MacroDefs.h"
#include "RenderWindow.h"
#include "CellRuleSet.h"

class CellState {
public: 
	virtual ~CellState() = default;
	virtual void iterate(CellCanvas& lifeCanvas, CellRuleSet& ruleSet, RenderWindow& window) {/*Baseclass does nothing...*/ };
};