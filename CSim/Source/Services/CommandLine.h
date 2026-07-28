#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "IEnvVars.h"
#include "Rendering/IRenderWindow.h"
#include "InputContext.h"
#include "Drawable.h"
#include "Foundation/MathTypes.h"
#include "CommandRegistry.h"
#include "SceneObject.h"

#define MAX_CHARS_PER_LINE 1024
#define MAX_CMD_HISTORY 256

class CommandLine : public SceneObject, public Drawable<CommandLine> {
// ----------------------------------------------------
// PUBLIC INTERFACE: Anyone (and GLFW callbacks) can call these
// ----------------------------------------------------
public:
    struct historyBuffer {
        unsigned char r, g, b, a;
        std::string content;
    };
    CommandLine(IEnvVars* vars, CommandRegistry* commandRegistry, IRenderWindow* win);
    void Toggle();
    void AddCharacter(unsigned int codepoint);
    void HandleBackspace();
    void ExecuteCommand();
    void HistoryUp();
    void HistoryDown();
    void AddToHistory(std::string command);
    void ScrollUp();
    void ScrollDown();
    void DrawImpl(); // Your main render loop needs to call this!
    bool isOpen;
    void logNormal(const std::string& str);
    void logError(const std::string& str);
    void logWarning(const std::string& str);
    void logSuccess(const std::string& str);
    void logTrace(const std::string& str);
    void AppendStringLn(unsigned char r, unsigned char g, unsigned char b, unsigned char a, std::string str);
    void AppendString(unsigned char r, unsigned char g, unsigned char b, unsigned char a, std::string str);
    std::vector<std::string> ParseCommandArgs(const std::string& text, const std::string& delim);
// ----------------------------------------------------
// PRIVATE DATA: Only CommandLine and its friends can see these
// ----------------------------------------------------
private:
    struct Vertex;
    
    std::string currentInput;   
    std::string tempInput;
    std::vector<historyBuffer> history; 
    std::vector<std::string> commandHistory; 
    int historyIndex; 
    int scrollOffset;
	bool consoleInitialized = false;

    IEnvVars* envVars;
    IRenderWindow* window;
    CommandRegistry* commandRegistry;

    // Grants CellMain direct access to these private variables if needed
    friend void CellMain(const std::string&); 
    void initConsoleGL();
};