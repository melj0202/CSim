#pragma once 
#include "RuleSet.h"

class Rule90RuleSet : public RuleSet {
public:
	Rule90RuleSet(Canvas* targetCanvas) : RuleSet(targetCanvas) {}
	int countNeighbors(const int &r, const int &c, const int &w, const int &h) const override;
private:

};