#pragma once 
#include "CellRuleSet.h"

class Rule90RuleSet : public CellRuleSet {
public:
	int countNeighbors(const int &r, const int &c, const int &w, const int &h) const override;
private:

};