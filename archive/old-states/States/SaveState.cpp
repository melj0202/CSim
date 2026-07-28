#include "SaveState.h"
#include <iostream>
#include <fstream>
#include <string>
#include "Logger.h"
#include "Core/CellContext.h"
#include "System/ServiceLocator.h"

State* SaveState::iterate(RuleSet* ruleSet, const char* filename, State* prevState) {

    CellContext* cellContext = ServiceLocator::get<CellContext>();
    if (!filename || filename[0] == '\0') {
        Logger::LogWarning("Save cancelled: file path is empty.");
        return prevState;
    }

    std::fstream myfile(filename, std::ios::binary | std::ios::out);
    if (!myfile.is_open()) {
        std::string err = "Failed to open file for saving: " + std::string(filename);
        Logger::LogError(err.c_str());
        return prevState;
    }

    myfile.write(ruleSet->getRuleTag().c_str(), MAX_RULETAG_SIZE);
    int width = cellContext->getCellCanvas()->canvasWidth;
    int height = cellContext->getCellCanvas()->canvasHeight;
    myfile.write(reinterpret_cast<char*>(&width), sizeof(width));
    myfile.write(reinterpret_cast<char*>(&height), sizeof(height));
    myfile.write(reinterpret_cast<char*>(&cellContext->getCellCanvas()->lifeCanvas[0]), width * height);
    myfile.close();

    std::string info = "Saved canvas to File : " + std::string(filename);
    Logger::LogInfo(info.c_str());
    return prevState;
}