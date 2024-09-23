#include "CellSaveState.h"
#include <iostream>
#include <fstream>
#include "CellCanvas.h"
#include "CellLogger.h"

CellState* CellSaveState::iterate(CellRuleSet* ruleSet, const char* filename, CellState* prevState) {

    /*Open a save dialog so that the user can specify a save file*/
    std::fstream myfile(filename, std::ios::binary | std::ios::out);
    myfile.write(ruleSet->getRuleTag().c_str(), MAX_RULETAG_SIZE);
    myfile.write(reinterpret_cast<char*>(&CellCanvas::canvasWidth), sizeof(CellCanvas::canvasWidth));
    myfile.write(reinterpret_cast<char*>(&CellCanvas::canvasHeight), sizeof(CellCanvas::canvasHeight));
    myfile.write(reinterpret_cast<char*>(&CellCanvas::lifeCanvas[0]), CellCanvas::canvasWidth * CellCanvas::canvasHeight);
    myfile.close();

    char info[288] = "Saved canvas to File : ";
    strcat_s(info, filename);
    CellLogger::LogInfo(info);
    return prevState;
}