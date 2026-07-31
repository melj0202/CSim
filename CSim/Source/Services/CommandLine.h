#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <cstdint>
#include "IEnvVars.h"
#include "Rendering/IRenderWindow.h"
#include "InputContext.h"
#include "Drawable.h"
#include "Foundation/MathTypes.h"
#include "CommandRegistry.h"

#define MAX_CHARS_PER_LINE 1024
#define MAX_CMD_HISTORY 256

class Renderer;

// Console UI drawable (token path). No longer inherits SceneObject (D-E4).
class CommandLine : public Drawable<CommandLine> {
public:
	struct historyBuffer {
		unsigned char r, g, b, a;
		std::string content;
	};
	CommandLine(IEnvVars* vars, CommandRegistry* commandRegistry, IRenderWindow* win, Renderer* renderer = nullptr);
	void Toggle();
	void AddCharacter(unsigned int codepoint);
	void HandleBackspace();
	void ExecuteCommand();
	void HistoryUp();
	void HistoryDown();
	void AddToHistory(std::string command);
	void ScrollUp();
	void ScrollDown();
	void DrawImpl();
	bool AppendCommands(Renderer* renderer) override;
	bool isOpen;
	// True while open or still sliding (avoid dispatch when fully closed).
	bool wantsDraw() const { return isVisible() && (isOpen || animationProgress > 0.0f); }
	void logNormal(const std::string& str);
	void logError(const std::string& str);
	void logWarning(const std::string& str);
	void logSuccess(const std::string& str);
	void logTrace(const std::string& str);
	void AppendStringLn(unsigned char r, unsigned char g, unsigned char b, unsigned char a, std::string str);
	void AppendString(unsigned char r, unsigned char g, unsigned char b, unsigned char a, std::string str);
	std::vector<std::string> ParseCommandArgs(const std::string& text, const std::string& delim);

private:
	struct ConsoleVertex {
		float x, y, z;
		uint8_t color[4];
	};

	std::string currentInput;
	std::string tempInput;
	std::vector<historyBuffer> history;
	std::vector<std::string> commandHistory;
	int historyIndex;
	int scrollOffset;
	bool consoleInitialized;

	IEnvVars* envVars;
	IRenderWindow* window;
	CommandRegistry* commandRegistry;
	Renderer* renderer;

	unsigned long meshHandle;
	unsigned long shaderHandle;
	bool gpuReady;

	// Animation (was static in DrawImpl)
	float animationProgress;
	std::chrono::high_resolution_clock::time_point lastAnimTime;

	// Batched UI verts for one UpdateBuffer + DrawIndexed (valid until Submit).
	// Capacity matches dynamic mesh enroll (2000 quads × 4 = 8000; room for safety).
	static const unsigned int kUiVertCap = 12000;
	ConsoleVertex uiVerts[kUiVertCap];

	friend void CellMain(const std::string&);
	void enrollGpuResources();
};
