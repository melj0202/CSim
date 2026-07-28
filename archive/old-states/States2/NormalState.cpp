#include "NormalState.h"
#include "Canvas.h"
#include "IRenderWindow.h"
#include "Core/CellContext.h"

State* NormalState::iterate(RuleSet *ruleSet, const char* filename, State* prevState)
{
	Canvas* canvas = ruleSet->canvas;
	int width = canvas->canvasWidth;
	int height = canvas->canvasHeight;

	ruleSet->calcGeneration(0, 0, width, height);
	for (int i = 0; i < width * height; i++) {
		ruleSet->evalCell(canvas->lifeCanvas[i], &canvas->texCanvasBuffer[i * 3]);
	}
	
	return prevState;
}
