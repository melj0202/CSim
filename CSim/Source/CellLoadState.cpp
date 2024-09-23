#include "CellLoadState.h"
#include <iostream>
#include <fstream>
#include "CellCanvas.h"
#include "CellLogger.h"

CellState* CellLoadState::iterate(CellRuleSet* ruleSet, const char* filename, CellState* prevState) {

    /*Open a save dialog so that the user can specify a save file*/

    

    std::fstream myfile(filename, std::ios::binary | std::ios::in);
    char instring[MAX_RULETAG_SIZE+1];
    memset(instring,0, MAX_RULETAG_SIZE + 1);
    static int fWidth = 0;
    static int fHeight = 0;

    myfile.read(instring, MAX_RULETAG_SIZE);
    std::string ruleString = instring;
    if (std::strcmp(ruleString.c_str(), ruleSet->getRuleTag().data())) {
        char err[33] = "This data is meant for ruleset: ";
        CellLogger::LogError(std::strcat(err, instring));
        std::cin.get();
        std::exit(0);
    }
    myfile.read(reinterpret_cast<char*>(&fWidth), sizeof(fWidth));
    myfile.read(reinterpret_cast<char*>(&fHeight), sizeof(fHeight));
    if (fWidth > CellCanvas::canvasWidth || fHeight > CellCanvas::canvasHeight) {
        CellLogger::LogWarning("Input canvas is larger than allocated canvas. Cell data may not read correctly...");
    }
    myfile.read(reinterpret_cast<char*>(&CellCanvas::lifeCanvas[0]), CellCanvas::canvasWidth * CellCanvas::canvasHeight);
    myfile.close();
    return prevState;
}