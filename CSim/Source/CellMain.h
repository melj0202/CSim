//
// Created by gravi on 10/6/2024.
//

#include "Init/MacroDefs.h"
#include <string>
#include "RenderWindow.h"
#include "Init/AllSets.h"
#include <cassert>
#include "Init/AllState.h"
#include "Init/BuildInfo.h"
#include "CellCanvas.h"
#include "CellLogger.h"

#ifndef CELLMAIN_H
#define CELLMAIN_H

static struct stateStruct {
    CellState* normal = new CellNormalState();
    CellState* edit = new CellEditState();
    CellState* save = new CellSaveState();
    CellState* load = new CellLoadState();
} canvasState;

//GLOBAL VARIABLES
static int speedFactor = 64;
static const unsigned int speedFactorMin = 1;
static const unsigned int speedFactorMax = 128;
static unsigned int WinX = 1280;
static unsigned int WinY = 720;
static unsigned int CanvasX = 80;
static unsigned int CanvasY = 60;
static std::string ModeString = "GAME_OF_LIFE";
static CellRuleSet* ruleSet = nullptr;

static auto currentState = canvasState.edit;

extern void CellMain(const std::string &ModeString);

extern void normalKeyCallback(GLFWwindow* window, const int key, int, const int action, const int mods);
#endif //CELLMAIN_H
