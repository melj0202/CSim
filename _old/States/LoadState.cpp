#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include "Canvas.h"
#include "Logger.h"
#include "Core/CellContext.h"


State* LoadState::iterate(RuleSet* ruleSet, const char* filename, State* prevState) {

    
    CellContext* cellContext = ServiceLocator::get<CellContext>();
    if (!filename || filename[0] == '\0') {
        Logger::LogWarning("Load cancelled: file path is empty.");
        return prevState;
    }

    std::fstream myfile(filename, std::ios::binary | std::ios::in);
    if (!myfile.is_open()) {
        std::string err = "Failed to open file for loading: " + std::string(filename);
        Logger::LogError(err.c_str());
        return prevState;
    }

    char instring[MAX_RULETAG_SIZE + 1];
    memset(instring, 0, MAX_RULETAG_SIZE + 1);
    static int fWidth = 0;
    static int fHeight = 0;

    myfile.read(instring, MAX_RULETAG_SIZE);
    std::string ruleString = instring;
    if (ruleString != ruleSet->getRuleTag()) {
        std::string err = "This data is meant for ruleset: " + ruleString;
        Logger::LogError(err.c_str());
        return prevState;
    }

    myfile.read(reinterpret_cast<char*>(&fWidth), sizeof(fWidth));
    myfile.read(reinterpret_cast<char*>(&fHeight), sizeof(fHeight));
    int width = cellContext->getCellCanvas()->canvasWidth;
    int height = cellContext->getCellCanvas()->canvasHeight;
    if (fWidth > width || fHeight > height) {
        Logger::LogWarning("Input canvas is larger than allocated canvas. Cell data may not read correctly...");
    }

    // Clear the current canvas first to avoid remnants
    memset(cellContext->getCellCanvas()->lifeCanvas, 1, width * height);

    // Read only as many bytes as can fit in the current canvas
    int bytesToRead = std::min(fWidth * fHeight, width * height);
    myfile.read(reinterpret_cast<char*>(&cellContext->getCellCanvas()->lifeCanvas[0]), bytesToRead);
    myfile.close();

    std::string info = "Loaded canvas from File : " + std::string(filename);
    Logger::LogInfo(info.c_str());
    return prevState;
}