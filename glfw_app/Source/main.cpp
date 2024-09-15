#include <string>
#include "RenderWindow.h"
#include "AllSets.h"
#include <cassert>
#include "CellEditState.h"
#include "CellNormalState.h"
#include "BuildInfo.h"

enum class application_mode {
	NORMAL, EDIT
};

//A dirty bit to only set mode when it has changed
bool dirty = false;


//GLOBAL VARIABLES
application_mode currentMode = application_mode::EDIT;
float speedFactor = 50.0f;
const float speedFactorMin = 1.0f;
const float speedFactorMax = 250.0f;
unsigned int WinX = 1280;
unsigned int WinY = 720;
unsigned int CanvasX = 80;
unsigned int CanvasY = 60;
std::string ModeString = "GAME_OF_LIFE";


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

void CellMain(const std::wstring &ModeString) {

	std::cout << WinX  << "\t" << WinY << std::endl;
	CellCanvas lifeCanvas(CanvasX, CanvasY); //Cell canvas

	//Default config: Lay down a glider.
	lifeCanvas.setCanvasPixel(50, 40, std::array<unsigned char, 3> {0, 0, 0});
	lifeCanvas.setCanvasPixel(51, 40, std::array<unsigned char, 3> {0, 0, 0});
	lifeCanvas.setCanvasPixel(52, 40, std::array<unsigned char, 3> {0, 0, 0});
	lifeCanvas.setCanvasPixel(52, 39, std::array<unsigned char, 3> {0, 0, 0});
	lifeCanvas.setCanvasPixel(51, 38, std::array<unsigned char, 3> {0, 0, 0});

	//Create the program states, and set the default mode to edit mode. Makes reimplementing pausing redundant. Go into edit mode to pause the sim.
	CellRuleSet* ruleSet = nullptr;

	CellState* normalState = new CellNormalState();
	CellState* editState = new CellEditState();

	CellState* currentState = editState;

	//Select a ruleset based on the command line arguements.  
	if (ModeString == L"GAME_OF_LIFE") ruleSet = new GameOfLifeRuleSet();
	else if (ModeString == L"BRIANS_BRAIN") ruleSet = new BrainsBrainRuleSet();
	else if (ModeString == L"DAY_AND_NIGHT") ruleSet = new DayAndNightRuleSet();
	else if (ModeString == L"HIGHLIFE") ruleSet = new HighlifeRuleSet();
	else if (ModeString == L"LIFE_WITHOUT_DEATH") ruleSet = new LifeWithoutDeathRuleSet();
	else if (ModeString == L"SEEDS") ruleSet = new SeedsRuleSet();
	if (ruleSet == nullptr) {
		std::cerr << "error: UNKNOWN RULESET" << std::endl;
		exit(1);
	}

	RenderWindow window(WinX, WinY, "Game of Life", lifeCanvas);
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


/*
Command Line Arguements:

 "-ww": Render window width
 "-wh": Render window height
 "-cw": Cell canvas width
 "-ch": Cell canvas height
 "--help": Displays the help message and then kills the program.
 "--version": Displays extended version information and then kills the program.
*/

bool StringIsDigit(wchar_t* wtarget) {
	long wsize = wcslen(wtarget);
	for (int i = 0; i < wsize; i++) {
		if (!iswdigit((wint_t)wtarget[i])) {
			std::wcout << wtarget[i]<< std::endl;
			return false;
		}
	}
	return true;
}

bool StringIsModeString(wchar_t* wtarget) {
	long wsize = wcslen(wtarget);
	for (int i = 0; i < wsize; i++) {
		if (!iswalpha((wint_t)wtarget[i])) {
			if (wtarget[i] == (wint_t)'_') continue; //spare the '_' character
			return false;
		}
	}
	return true;
}

void parseCommandLineArgs(int argc, wchar_t** argv) {
	for (int i = 0; i < argc; i++) {
		
		if (!lstrcmpW(argv[i], L"-h") || !lstrcmpW(argv[i], L"--help"))
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
		if (!lstrcmpW(argv[i], L"-v") || !lstrcmpW(argv[i], L"--version")) {
			std::cout << "CSim Cell Automata Simulator\nVersion: " << VERSION_NUMBER <<"\nBuild Date: " << BUILD_DATE_SHORT << " " << BUILD_TIMESTAMP << std::endl;
			std::exit(0);
			return;
		}
		if (!lstrcmpW(argv[i], L"-ww")) {
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
		if (!lstrcmpW(argv[i], L"-wh")) {
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
		if (!lstrcmpW(argv[i], L"-cw")) {
			if (!StringIsDigit(argv[i + 1])) {
				std::cout << "ERROR: option '-cw' has invalid type!\n";
				std::exit(1);
			}
			else {
				CanvasX = _wtoi(argv[i + 1]);
				i += 2;
			}
			
		}
		if (!lstrcmpW(argv[i], L"-ch")) {
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

int wmain(int argc, wchar_t** argv) {
    
	std::wstring mode;

	//Parse command line args

	if (argc < 2) mode = L"GAME_OF_LIFE";
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