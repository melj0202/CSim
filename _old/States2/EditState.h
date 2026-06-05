#pragma once 
#include "State.h"

/*
	EditState
		Pauses the simulation, and allows the user to paint the canvas with cells or erase cells of the canvas.
*/
class EditState : public State {
public:
	EditState() : wasPressed(false), lastMouseX(0), lastMouseY(0) {}
	~EditState() override = default;
	State* iterate(RuleSet* ruleSet, const char* filename, State* prevState) override;
private:
	bool wasPressed;
	int lastMouseX;
	int lastMouseY;
};