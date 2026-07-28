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
#include "SceneObject.h"

#define MAX_CHARS_PER_LINE 1024
#define MAX_CMD_HISTORY 256

class Renderer;

class CommandLine : public SceneObject, public Drawable<CommandLine> {
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

	// CPU scratch for token payloads (valid until SubmitCommandQueue)
	ConsoleVertex panelVertices[4];
	ConsoleVertex sepVertices[4];
	ConsoleVertex trackVertices[4];
	ConsoleVertex thumbVertices[4];
	ConsoleVertex textQuads[12000];

	friend void CellMain(const std::string&);
	void enrollGpuResources();
};
