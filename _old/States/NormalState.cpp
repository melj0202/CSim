#include "NormalState.h"
#include "Canvas.h"
#include "IRenderWindow.h"
#include "Core/CellContext.h"
#include "System/ServiceLocator.h"

State* NormalState::iterate(RuleSet *ruleSet, const char* filename, State* prevState)
{
	CellContext* cellContext = ServiceLocator::get<CellContext>();
	int width = cellContext->getCellCanvas()->canvasWidth;
	int height = cellContext->getCellCanvas()->canvasHeight;

	ruleSet->calcGeneration(0, 0, width, height);
	for (int i = 0; i < width * height; i++) {
		ruleSet->evalCell(cellContext->getCellCanvas()->lifeCanvas[i], &cellContext->getCellCanvas()->texCanvasBuffer[i * 3]);
	}
	
	return prevState;
}
