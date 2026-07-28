#pragma once
#include "RuleSet.h"

// Stub Wireworld CA (not yet fully implemented).
// Full Wireworld needs electron-head neighbor counts, not Moore "alive" counts.
class WireworldRuleSet : public RuleSet {
public:
	WireworldRuleSet(Canvas* targetCanvas) : RuleSet(targetCanvas) {}
	~WireworldRuleSet() override = default;

	std::string getRuleTag() override { return "WIREWORLD"; }
};
