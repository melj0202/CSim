#include <string>
#include "RenderWindow.h"
#include "GameOfLifeRuleSet.h"
#include <cassert>
#include "CellEditState.h"
#include "CellNormalState.h"

enum class application_mode {
	NORMAL, EDIT
};

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
			glfwSwapInterval(0); // Vsync is for bitches
			std::cout << "EDIT" << std::endl;
		}
		else {
			currentMode = application_mode::NORMAL;
			glfwSwapInterval(0); // Vsync is for bitches
			std::cout << "NORMAL" << std::endl;
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

	if (ModeString == "GAME_OF_LIFE") ruleSet = new GameOfLifeRuleSet();

	assert(ruleSet != nullptr);

	RenderWindow window(1280, 720, "Game of Life", lifeCanvas);

	glfwSetKeyCallback(window.getWindowInstance(), normalKeyCallback);

	while (!glfwWindowShouldClose(window.getWindowInstance())) {

		//Input
		glfwPollEvents();

		//update (calculate generation)
		ruleSet->calcGeneration(0, 0, 80, 60, lifeCanvas);

		//render
		window.updateWindow(lifeCanvas);

		Sleep(static_cast<DWORD>(speedFactor));
	}
}
int main(void) {
    
	std::string mode = "GAME_OF_LIFE";
    CellMain(mode);
	glfwTerminate();
    return 0;
}