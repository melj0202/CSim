#pragma once
#include <string>
#include "Canvas.h"
#include "Init/AllSets.h"
#include "System/IEnvVars.h"
#include "System/Logger.h"
#include "System/CommandLine.h"
#include "Rendering/IRenderWindow.h"
#include "Rendering/Camera.h"


class CellContext {
    public:
        CellContext(std::string modeString, IEnvVars* envVars = nullptr, IRenderWindow* window = nullptr, Camera* camera = nullptr) {
            this->ModeString = modeString;
            
            long cx = 80;
            long cy = 60;
            if (envVars) {
                cx = envVars->getVar("CanvasX").valueAsLong;
                cy = envVars->getVar("CanvasY").valueAsLong;
            }
            canvas = new Canvas(cx, cy, window, camera);
            ruleSet = nullptr;
            setRuleSet(modeString);
        }
        ~CellContext() {
            delete ruleSet;
            delete canvas;
        }

        void setRuleSet(std::string modeString) {
            delete ruleSet;
            if (modeString == "GAME_OF_LIFE") ruleSet = new GameOfLifeRuleSet(canvas);
            else if (modeString == "BRIANS_BRAIN") ruleSet = new BrainsBrainRuleSet(canvas);
            else if (modeString == "DAY_AND_NIGHT") ruleSet = new DayAndNightRuleSet(canvas);
            else if (modeString == "HIGHLIFE") ruleSet = new HighlifeRuleSet(canvas);
            else if (modeString == "LIFE_WITHOUT_DEATH") ruleSet = new LifeWithoutDeathRuleSet(canvas);
            else if (modeString == "SEEDS") ruleSet = new SeedsRuleSet(canvas);
            else {
                Logger::LogError("Invalid rule set name: " + modeString);
                ruleSet = new GameOfLifeRuleSet(canvas);
            }
        }

        Canvas* getCanvas() const { return canvas; }
        Canvas* getCellCanvas() const { return canvas; }
        RuleSet* getRuleSet() const { return ruleSet; }
        std::string getModeString() const { return ModeString; }
        CommandLine* getCommandLine() const { return commandLine; }
        
    private:
        RuleSet* ruleSet;
        Canvas* canvas;
        std::string ModeString;
        CommandLine* commandLine;
        IRenderWindow* window;
        Camera* camera;
};