//
// Created by gravi on 10/6/2024.
//
#include "CellMain.h"
#include "SaveLoad.h"

void CellMain(const std::string &ModeString) {

    CellLogger::initLogger();

    initCanvas(CanvasX, CanvasY); //Cell canvas

    //Default config: Lay down a glider.
    setCanvasPixel(50, 40, 0);
    setCanvasPixel(51, 40, 0);
    setCanvasPixel(52, 40, 0);
    setCanvasPixel(52, 39, 0);
    setCanvasPixel(51, 38, 0);

    //Create the program states, and set the default mode to edit mode. Makes reimplementing pausing redundant. Go into edit mode to pause the sim.


    //Select a ruleset based on the command line arguements.
    if (ModeString == "GAME_OF_LIFE") ruleSet = new GameOfLifeRuleSet();
    else if (ModeString == "BRIANS_BRAIN") ruleSet = new BrainsBrainRuleSet();
    else if (ModeString == "DAY_AND_NIGHT") ruleSet = new DayAndNightRuleSet();
    else if (ModeString == "HIGHLIFE") ruleSet = new HighlifeRuleSet();
    else if (ModeString == "LIFE_WITHOUT_DEATH") ruleSet = new LifeWithoutDeathRuleSet();
    else if (ModeString == "SEEDS") ruleSet = new SeedsRuleSet();
    else {
        std::cerr << "error: UNKNOWN RULESET" << std::endl;
        exit(1);
    }

    RenderWindow window(WinX, WinY, "CSim");

    GLFWwindow* win = window.getWindowInstance();
    glfwSetKeyCallback(win, normalKeyCallback);

    //Main program loop
    while (!glfwWindowShouldClose(win)) {

        //Input
        glfwPollEvents();

        currentState->iterate(ruleSet, nullptr, currentState);

        if (currentState == canvasState.normal) __SLEEP(speedFactor);
    }

    free(ruleSet);
    freeCanvas();
}

void normalKeyCallback(GLFWwindow* window, const int key, int, const int action, const int mods)
{
	/*
	*	Allow the user to quit the program
	*/
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	/*
	*	Allow the user to adjust the speed of the cell simulation
	*/
	if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS && mods == GLFW_MOD_SHIFT) {
		if (speedFactor > speedFactorMin) speedFactor /= 2.0f;
		else speedFactor = speedFactorMin;
		std::cout << "changed delay to: " << speedFactor << std::endl;
	}
	if (key == GLFW_KEY_MINUS && action == GLFW_PRESS && mods == GLFW_MOD_SHIFT) {
		if (speedFactor < speedFactorMax) speedFactor *= 2.0f;
		else speedFactor = speedFactorMax;

		std::cout << "changed delay to: " << speedFactor << std::endl;
	}
	/*
		Toggle between normal and edit modes.
	*/
	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		if (currentState == canvasState.normal)
		{
			currentState = canvasState.edit;
			glfwSwapInterval(0);
			std::cout << "EDIT" << std::endl;
			//dirty = true;
		}
		else {
			currentState = canvasState.normal;
			glfwSwapInterval(0);
			std::cout << "NORMAL" << std::endl;
			//dirty = true;
		}
	}

	/*
		Go into save state and have the user pick a name for the file to save the data to
	*/
	else if (key == GLFW_KEY_S && mods == GLFW_MOD_CONTROL && action == GLFW_PRESS && currentState == canvasState.edit) {
		//Go into save State
		std::cout << "SAVE" << std::endl;

		currentState = canvasState.save;
		currentState = currentState->iterate(ruleSet, SaveLoad::GetSaveLocation().c_str(), canvasState.edit);
	}
	/*
		Go into load state and have the user pick a file to load in.
	*/
	else if (key == GLFW_KEY_O && mods == GLFW_MOD_CONTROL && action == GLFW_PRESS && currentState == canvasState.edit) {
		std::cout << "LOAD" << std::endl;

		currentState = canvasState.load;
		currentState = currentState->iterate(ruleSet, SaveLoad::GetLoadLocation().c_str(), canvasState.edit);
	}
	else if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS) {
		std::cout << "OPEN THE CONSOLE\n";
	}
}