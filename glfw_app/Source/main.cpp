#include <string>
#include "RenderWindow.h"
#include "AllSets.h"
#include <cassert>
#include "CellEditState.h"
#include "CellNormalState.h"

enum class application_mode {
	NORMAL, EDIT
};

//A dirty bit to only set mode when it has changed
bool dirty = false;

application_mode currentMode = application_mode::EDIT;
float speedFactor = 50.0f;
const float speedFactorMin = 1.0f;
const float speedFactorMax = 250.0f;

static void normalKeyCallback(GLFWwindow* window, const int key, int, const int action, const int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS && mods == GLFW_MOD_SHIFT) {
		if (speedFactor >= speedFactorMin) speedFactor /= 1.5f;
		else speedFactor = speedFactorMin;
		std::cout << "changed delay to: " << speedFactor << std::endl;
	}
	if (key == GLFW_KEY_MINUS && action == GLFW_PRESS && mods == GLFW_MOD_SHIFT) {
		if (speedFactor <= speedFactorMax)speedFactor *= 1.5f;
		else speedFactor = speedFactorMax;

		std::cout << "changed delay to: " << speedFactor << std::endl;
	}
	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		if (currentMode == application_mode::NORMAL)
		{
			currentMode = application_mode::EDIT;
			glfwSwapInterval(0); 
			std::cout << "EDIT" << std::endl;
			dirty = true;
		}
		else {
			currentMode = application_mode::NORMAL;
			glfwSwapInterval(0); 
			std::cout << "NORMAL" << std::endl;
			dirty = true;
		}
	}
}

void CellMain(const std::string &ModeString) {


	CellCanvas lifeCanvas(80, 60); //Cell canvas

	lifeCanvas.setCanvasPixel(50, 40, std::array<unsigned char, 3> {0, 0, 0});
	lifeCanvas.setCanvasPixel(51, 40, std::array<unsigned char, 3> {0, 0, 0});
	lifeCanvas.setCanvasPixel(52, 40, std::array<unsigned char, 3> {0, 0, 0});
	lifeCanvas.setCanvasPixel(52, 39, std::array<unsigned char, 3> {0, 0, 0});
	lifeCanvas.setCanvasPixel(51, 38, std::array<unsigned char, 3> {0, 0, 0});

	CellRuleSet* ruleSet = nullptr;

	CellState* normalState = new CellNormalState();
	CellState* editState = new CellEditState();

	CellState* currentState = editState;


	if (ModeString == "GAME_OF_LIFE") ruleSet = new GameOfLifeRuleSet();
	else if (ModeString == "BRIANS_BRAIN") ruleSet = new BrainsBrainRuleSet();
	else if (ModeString == "DAY_AND_NIGHT") ruleSet = new DayAndNightRuleSet();
	else if (ModeString == "HIGHLIFE") ruleSet = new HighlifeRuleSet();
	else if (ModeString == "LIFE_WITHOUT_DEATH") ruleSet = new LifeWithoutDeathRuleSet();
	else if (ModeString == "SEEDS") ruleSet = new SeedsRuleSet();
	if (ruleSet == nullptr) {
		std::cerr << "error: UNKNOWN RULESET" << std::endl;
		exit(1);
	}

	RenderWindow window(1280, 720, "Game of Life", lifeCanvas);
	window.updateWindow(lifeCanvas);
	glfwSetKeyCallback(window.getWindowInstance(), normalKeyCallback);

	while (!glfwWindowShouldClose(window.getWindowInstance())) {

		//Input
		glfwPollEvents();

		if (dirty && currentMode == application_mode::NORMAL) {
			currentState = normalState;
			dirty = !dirty;
		}
		else if (dirty && currentMode == application_mode::EDIT) {
			currentState = editState;
			dirty = !dirty;
		}

		currentState->iterate(lifeCanvas, *ruleSet, window);

		if (currentMode == application_mode::NORMAL) Sleep(static_cast<DWORD>(speedFactor));
	}

	free(ruleSet);
}
int main(int argc, char** argv) {
    
	std::string mode;

	if (argc < 2) mode = "GAME_OF_LIFE";
	else { mode = argv[1]; }

    CellMain(mode);
    return 0;
}