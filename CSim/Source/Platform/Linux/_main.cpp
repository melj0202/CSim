/*
    This is the main file for the linux source port of CSim
*/

#include <string>
#include "RenderWindow.h"
#include "AllSets.h"
#include <cassert>
#include "CellEditState.h"
#include "CellNormalState.h"
#include "BuildInfo.h"
#include "CellCanvas.h"

static struct stateStruct {
	CellState* normal = new CellNormalState();
	CellState* edit = new CellEditState();
} canvasState;

//GLOBAL VARIABLES
static float speedFactor = 50.0f;
static const float speedFactorMin = 1.0f;
static const float speedFactorMax = 250.0f;
static unsigned int WinX = 1280;
static unsigned int WinY = 720;
static unsigned int CanvasX = 80;
static unsigned int CanvasY = 60;
static std::string ModeString = "GAME_OF_LIFE";

static CellState* currentState = canvasState.edit;

//Key callback function that is assigned to the window
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
}

void CellMain(const std::string &ModeString) {

	std::cout << WinX  << "\t" << WinY << std::endl;
	initCanvas(CanvasX, CanvasY); //Cell canvas

	//Default config: Lay down a glider.
	setCanvasPixel(50, 40, 0);
	setCanvasPixel(51, 40, 0);
	setCanvasPixel(52, 40, 0);
	setCanvasPixel(52, 39, 0);
	setCanvasPixel(51, 38, 0);

	//Create the program states, and set the default mode to edit mode. Makes reimplementing pausing redundant. Go into edit mode to pause the sim.
	CellRuleSet* ruleSet = nullptr;

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
	
	RenderWindow window(WinX, WinY, "Game of Life");
	
	GLFWwindow* win = window.getWindowInstance();
	glfwSetKeyCallback(win, normalKeyCallback);

	while (!glfwWindowShouldClose(win)) {

		//Input
		glfwPollEvents();
		
		currentState->iterate(*ruleSet);

		if (currentState == canvasState.normal) Sleep(static_cast<DWORD>(speedFactor));
	}

	free(ruleSet);
	freeCanvas();
}

/*
Command Line Arguements:

 "-ww": Render window width
 "-wh": Render window height
 "-cw": Cell canvas width
 "-ch": Cell canvas height
 "--help": Displays the help message and then kills the program.
 "--version": Displays extended version information and then kills the program.
*/

bool StringIsDigit(char* target) {
	long size = strlen(wtarget);
	for (int i = 0; i < size; i++) {
		if (!isdigit((wint_t)wtarget[i])) {
			std::wcout << wtarget[i]<< std::endl;
			return false;
		}
	}
	return true;
}

bool StringIsModeString(char* target) {
	long size = strlen(wtarget);
	for (int i = 0; i < size; i++) {
		if (!isalpha((wint_t)wtarget[i])) {
			if (wtarget[i] == (wint_t)'_') continue; //spare the '_' character
			return false;
		}
	}
	return true;
}

void parseCommandLineArgs(int argc, char** argv) {
	for (int i = 0; i < argc; i++) {
		
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
		{
			//Print out a helpful message dialog and then exit the program completely.
			std::cout <<
				"CSim Cell Automata Simulator\nUsage: csim.exe RULESET [OPTION] ... [FILE] ...\n\n" <<
				"Options:\n-ww\t The render window width in pixels\n" <<
				"-wh\t The render window height in pixels\n" <<
				"-cw\t The cell colony width in pixels\n" <<
				"-ch\t The cell colony height in pixels\n" <<
				"-h, --help\t Print out this help message\n" << 
				"-v, --version\t Print out version information for this application.\n\n" <<
				"Rulesets:\nGAME_OF_LIFE\t\t Conway's Game of Life\n" << 
				"BRIANS_BRAIN\t\t Brians Brain\n" << 
				"LIFE_WITHOUT_DEATH\t Life Without Death\n" << 
				"HIGHLIFE\t\t Similar to Game of Life\n" << 
				"SEEDS\t\t\t Seeds\n" <<
				"DAY_AND_NIGHT\t\t Day and Night\n";
			std::exit(0);
			return;
		}
		if (!strcmp(argv[i], "-v") || !lstrcmpW(argv[i], L"--version")) {
			std::cout << "CSim Cell Automata Simulator\nVersion: " << VERSION_NUMBER <<"\nBuild Date: " << BUILD_DATE_SHORT << " " << BUILD_TIMESTAMP << std::endl;
			std::exit(0);
			return;
		}
		if (!strcmp(argv[i], "-ww")) {
			if (!StringIsDigit(argv[i + 1])) {
				std::cout << "ERROR: option '-ww' has invalid type!\n";
				std::exit(1);
			}
			else {
				WinX = _wtoi(argv[i + 1]);
				std::wcout << "This is a test\n";
				i += 2;
			}
			
		}
		if (!strcmp(argv[i], "-wh")) {
			if (!StringIsDigit(argv[i + 1])) {
				std::cout << "ERROR: option '-wh' has invalid type!\n";
				std::exit(1);
			}
			else {
				WinY = _wtoi(argv[i + 1]);
				std::wcout << "This is a test\n";
				i += 2;
			}
			
		}
		if (!strcmp(argv[i], "-cw")) {
			if (!StringIsDigit(argv[i + 1])) {
				std::cout << "ERROR: option '-cw' has invalid type!\n";
				std::exit(1);
			}
			else {
				CanvasX = _wtoi(argv[i + 1]);
				i += 2;
			}
			
		}
		if (!strcmp(argv[i], "-ch")) {
			if (!StringIsDigit(argv[i + 1])) {
				std::cout << "ERROR: option '-ch' has invalid type!\n";
				std::exit(1);
			}
			else {
				CanvasY = _wtoi(argv[i + 1]);
				i += 2;
			}
			
		}
	}
}

int main(int argc, wchar_t** argv) {
    
	std::string mode;

	//Parse command line args

	if (argc < 2) mode = "GAME_OF_LIFE";
	else { 
		parseCommandLineArgs(argc, argv);
		if (!StringIsModeString(argv[1])) {
			std::cout << "ERROR: Missing Ruleset" << std::endl;
			std::exit(0);
		}
		mode = argv[1];
	}

    CellMain(mode);
    return 0;
}