#pragma once
#include <string>
#include <cctype>
#include "Canvas.h"
#include "Rulesets/AllSets.h"
#include "Services/IEnvVars.h"
#include "Services/Logger.h"
#include "Services/CommandLine.h"
#include "Rendering/IRenderWindow.h"
#include "Rendering/Camera.h"

class Renderer;

class CellContext {
    public:
        CellContext(std::string modeString, IEnvVars* envVars = nullptr, IRenderWindow* window = nullptr, Camera* camera = nullptr, Renderer* renderer = nullptr) {
            this->envVars = envVars;
            this->window = window;
            this->camera = camera;
            this->renderer = renderer;
            this->commandLine = nullptr;

            long cx = 80;
            long cy = 60;
            if (envVars) {
                cx = envVars->getVar("CanvasX").valueAsLong;
                cy = envVars->getVar("CanvasY").valueAsLong;
                if (cx < 1) cx = 80;
                if (cy < 1) cy = 60;
            }
            canvas = new Canvas(static_cast<int>(cx), static_cast<int>(cy), window, camera, renderer);
            ruleSet = nullptr;
            ModeString = "";
            setRuleSet(modeString);
        }
        ~CellContext() {
            delete ruleSet;
            delete canvas;
        }

        static std::string NormalizeModeString(std::string modeString) {
            for (size_t i = 0; i < modeString.size(); ++i) {
                modeString[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(modeString[i])));
            }
            return modeString;
        }

        static bool IsKnownModeString(const std::string& modeString) {
            return modeString == "GAME_OF_LIFE"
                || modeString == "BRIANS_BRAIN"
                || modeString == "DAY_AND_NIGHT"
                || modeString == "HIGHLIFE"
                || modeString == "LIFE_WITHOUT_DEATH"
                || modeString == "SEEDS"
                || modeString == "WIREWORLD";
        }

        // Returns true if the active ruleset instance changed.
        bool setRuleSet(std::string modeString) {
            modeString = NormalizeModeString(modeString);
            if (modeString.empty()) {
                modeString = "GAME_OF_LIFE";
            }

            // Avoid thrashing if console/env re-asserts the same mode.
            if (ruleSet != nullptr && modeString == ModeString) {
                return false;
            }

            delete ruleSet;
            ruleSet = nullptr;

            if (modeString == "GAME_OF_LIFE") {
                ruleSet = new GameOfLifeRuleSet(canvas);
            }
            else if (modeString == "BRIANS_BRAIN") {
                ruleSet = new BrainsBrainRuleSet(canvas);
            }
            else if (modeString == "DAY_AND_NIGHT") {
                ruleSet = new DayAndNightRuleSet(canvas);
            }
            else if (modeString == "HIGHLIFE") {
                ruleSet = new HighlifeRuleSet(canvas);
            }
            else if (modeString == "LIFE_WITHOUT_DEATH") {
                ruleSet = new LifeWithoutDeathRuleSet(canvas);
            }
            else if (modeString == "SEEDS") {
                ruleSet = new SeedsRuleSet(canvas);
            }
            else if (modeString == "WIREWORLD") {
                ruleSet = new WireworldRuleSet(canvas);
            }
            else {
                Logger::LogError("Invalid rule set name: " + modeString);
                ruleSet = new GameOfLifeRuleSet(canvas);
                modeString = "GAME_OF_LIFE";
            }

            ModeString = modeString;
            if (envVars) {
                envVars->setVar("ModeString", ModeString);
            }
            return true;
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
        IEnvVars* envVars;
        IRenderWindow* window;
        Camera* camera;
        Renderer* renderer;
};
