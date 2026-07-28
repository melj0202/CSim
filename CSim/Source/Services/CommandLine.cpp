#include "CommandLine.h"
#include "CellMain.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "thirdparty/stb/stb_easy_font.h"
#include <sstream>
#include <iostream>
#include <cstdint>
#include <cctype>
#include <vector>


struct ConsoleVertex
{
	float x, y, z;
	uint8_t color[4];
};

CommandLine::CommandLine(IEnvVars* vars, CommandRegistry* commandRegistry, IRenderWindow* win) : envVars(vars), commandRegistry(commandRegistry), window(win)
{
	isOpen = false;
	currentInput = "";
	tempInput = "";
	history = {
		{240, 240, 240, 255, "CSim Developer Console"},
		{240, 240, 240, 255, "Press ` to toggle, type 'help' for commands"}
	};

	historyIndex = 0;
	scrollOffset = 0;

	if (!consoleInitialized)
	{
		initConsoleGL();
	}
}

void CommandLine::initConsoleGL()
{
	shaderID = new unsigned int(0);
	VAO = new unsigned int(0);
	VBO = new unsigned int(0);
	EBO = new unsigned int(0);
	shaderProgramID = new unsigned int(createShaderProgram(
		R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
out vec4 ourColor;
uniform vec2 u_resolution;
uniform vec2 u_scale;
void main() {
    vec2 scaledPos = aPos.xy * u_scale;
    float x = (scaledPos.x / u_resolution.x) * 2.0 - 1.0;
    float y = 1.0 - (scaledPos.y / u_resolution.y) * 2.0;
    gl_Position = vec4(x, y, aPos.z, 1.0);
    ourColor = aColor;
}
)",
R"(
#version 330 core
in vec4 ourColor;
out vec4 FragColor;
void main() {
    FragColor = ourColor;
}
)"));
	historyIndex = (int) commandHistory.size();
	glGenVertexArrays(1, VAO);
	glGenBuffers(1, VBO);
	glGenBuffers(1, EBO);

	glBindVertexArray(*VAO);

	glBindBuffer(GL_ARRAY_BUFFER, *VBO);
	glBufferData(GL_ARRAY_BUFFER, 8000 * sizeof(ConsoleVertex), nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
	unsigned int indices[12000];
	for (int i = 0; i < 2000; ++i)
	{
		indices[i * 6 + 0] = i * 4 + 0;
		indices[i * 6 + 1] = i * 4 + 1;
		indices[i * 6 + 2] = i * 4 + 2;
		indices[i * 6 + 3] = i * 4 + 2;
		indices[i * 6 + 4] = i * 4 + 3;
		indices[i * 6 + 5] = i * 4 + 0;
	}
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ConsoleVertex), (void*) 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ConsoleVertex), (void*) 12);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	consoleInitialized = true;

	//Add to the window's render queue
	//window->getRenderQueue()->add(this);

}

void CommandLine::Toggle()
{
	isOpen = !isOpen;
	if (isOpen)
	{
		scrollOffset = 0;
	}
}

void CommandLine::AddCharacter(unsigned int codepoint)
{
	if (currentInput.size() < MAX_CHARS_PER_LINE - 1)
	{
		if (codepoint >= 32 && codepoint <= 126)
		{
			currentInput += (char) codepoint;
		}
	}
}

void CommandLine::HandleBackspace()
{
	if (!currentInput.empty())
	{
		currentInput.pop_back();
	}
}

void CommandLine::logNormal(const std::string& str)
{
	AppendString(255, 255, 255, 255, str);
}
void CommandLine::logError(const std::string& str)
{
	AppendString(255, 100, 100, 255, "ERROR: " + str);
	//AppendStringLn(255, 255, 255, 255, str);
}
void CommandLine::logWarning(const std::string& str)
{
	AppendString(255, 220, 100, 255, "WARNING: " + str);
	//AppendStringLn(255, 255, 255, 255, str);
}
void CommandLine::logSuccess(const std::string& str)
{
	AppendString(100, 255, 100, 255, "SUCCESS: " + str);
	//AppendStringLn(255, 255, 255, 255, str);
}

void CommandLine::logTrace(const std::string& str)
{
	AppendString(206, 0, 252, 255, "TRACE: " + str);
	//AppendStringLn(255, 255, 255, 255, str);
}

void CommandLine::ExecuteCommand()
{
	if (currentInput.empty()) return;

	// Command echoes can be cyan for premium look
	AppendString(100, 200, 255, 255, "> " + currentInput);

	std::stringstream ss(currentInput);
	std::string cmd;
	std::vector<std::string> args;
	ss >> cmd;
	while (!ss.eof())
	{
		std::string arg;
		ss >> arg;
		args.push_back(arg);
	}

	AddToHistory(currentInput);

	if (cmd == "help")
	{
		logNormal("Available commands:");
		logNormal("  help              - Show this help message");
		logNormal("  ruleset [name]    - Show or change CA ruleset");
		logNormal("                      GAME_OF_LIFE, BRIANS_BRAIN, DAY_AND_NIGHT,");
		logNormal("                      HIGHLIFE, LIFE_WITHOUT_DEATH, SEEDS");
		logNormal("  tps <n>           - Simulation ticks per second (via env)");
		logNormal("  speedFactor <n>   - Multiplier on tps (via env)");
		logNormal("  clear             - Clear console history");
		logNormal("  save <file>       - Save current state to filename");
		logNormal("  load <file>       - Load state from filename");
		logNormal("  vars              - Lists all environment variables and their values");
		logNormal("  vid_restart       - Restarts the renderer");
		logNormal("  close / quit      - Close console or exit app");
	}
	else if (cmd == "ruleset" || cmd == "mode" || cmd == "ModeString")
	{
		if (args.empty())
		{
			logNormal("Current ruleset: " + envVars->getVar("ModeString").value);
			logNormal("Usage: ruleset <name>");
			logNormal("  GAME_OF_LIFE | BRIANS_BRAIN | DAY_AND_NIGHT");
			logNormal("  HIGHLIFE | LIFE_WITHOUT_DEATH | SEEDS");
		}
		else
		{
			// Normalize to UPPER_SNAKE so GAME_OF_LIFE / game_of_life both work.
			std::string mode = args[0];
			for (size_t i = 0; i < mode.size(); ++i)
			{
				mode[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(mode[i])));
			}

			const bool known =
				mode == "GAME_OF_LIFE" ||
				mode == "BRIANS_BRAIN" ||
				mode == "DAY_AND_NIGHT" ||
				mode == "HIGHLIFE" ||
				mode == "LIFE_WITHOUT_DEATH" ||
				mode == "SEEDS";

			if (!known)
			{
				logError("Unknown ruleset '" + args[0] + "'");
				logNormal("Try: GAME_OF_LIFE, BRIANS_BRAIN, DAY_AND_NIGHT, HIGHLIFE, LIFE_WITHOUT_DEATH, SEEDS");
			}
			else
			{
				// CellGameModule watches ModeString and applies CellContext::setRuleSet.
				envVars->setVar("ModeString", mode);
				logSuccess("Ruleset set to " + mode);
			}
		}
	}
	else if (cmd == "clear")
	{
		history.clear();
		AppendString(240, 240, 240, 255, "CSim Developer Console");
	}
	else if (cmd == "vid_restart")
	{
//
	}
	else if (cmd == "save")
	{
		std::string filename;
		if (ss >> filename)
		{
//cContext->stateStruct.save->iterate(cContext->getRuleSet(), filename.c_str(), cContext->getCurrentState());
			logSuccess("Canvas saved to: " + filename);
		}
		else
		{
			logNormal("Usage: save <filename>");
		}
	}
	else if (cmd == "load")
	{
		std::string filename;
		if (ss >> filename)
		{
//State* res = cContext->stateStruct.load->iterate(cContext->getRuleSet(), filename.c_str(), cContext->getCurrentState());
// if (res) {
//     logSuccess("Canvas loaded from: " + filename);
// } else {
//     logError("Failed to load: " + filename);
// }
		}
		else
		{
			logNormal("Usage: load <filename>");
		}
	}
	else if (cmd == "vars")
	{
		for (const auto& pair : envVars->getVars())
		{
			logNormal(pair.first + " = " + pair.second.value);
		}
	}
	else if (cmd == "close")
	{
		isOpen = false;
	}
	else if (cmd == "quit")
	{
		window->requestClose();
	}
	else if (cmd == "test")
	{
		logWarning("WARNING: test");
	}
	else
	{
		if (commandRegistry->HasCommand(cmd))
		{
			commandRegistry->QueueCommand(cmd, args);
		}
		EnvVar tmp = envVars->getVar(cmd);
		if (!tmp.value.empty())
		{
			if (args.empty())
			{
				logNormal(cmd + " = " + tmp.value);
			}
			else if (args.size() == 1)
			{
				envVars->setVar(cmd, args[0]);
			}
		}
		else
		{
			if (args.size() == 1)
			{
				envVars->setVar(cmd, args[0]);
			}
			logError("Unknown command/var: " + cmd);
		}
	}

	currentInput.clear();
	scrollOffset = 0;
}

void CommandLine::AddToHistory(std::string command)
{
	commandHistory.push_back(command);
	//imitate stack behavior by setting historyIndex to top of stack and popping the first entry when full.
	if (commandHistory.size() > MAX_CMD_HISTORY)
	{
		commandHistory.erase(commandHistory.begin());
	}
	historyIndex = (int) commandHistory.size();
	tempInput = "";
}

void CommandLine::HistoryDown()
{
	if (commandHistory.empty()) return;
	if (historyIndex < (int) commandHistory.size())
	{
		historyIndex++;
		if (historyIndex == (int) commandHistory.size())
		{
			currentInput = tempInput;
		}
		else
		{
			currentInput = commandHistory[historyIndex];
		}
	}
}

void CommandLine::HistoryUp()
{
	if (commandHistory.empty()) return;
	if (historyIndex > 0)
	{
		if (historyIndex == (int) commandHistory.size())
		{
			tempInput = currentInput;
		}
		historyIndex--;
		currentInput = commandHistory[historyIndex];
	}
}

void CommandLine::ScrollUp()
{
	EnvVar winHeightVar = envVars->getVar("WinHeight");
	int winHeight = std::stoi(winHeightVar.value);
	float panelHeight = winHeight * 0.45f;
	if (panelHeight < 200.0f) panelHeight = 200.0f;
	float lineSpacing = 24.0f;
	int maxHistoryLines = (int) ((panelHeight - 40.0f) / lineSpacing);
	if (maxHistoryLines < 1) maxHistoryLines = 1;

	int maxScroll = (int) history.size() - maxHistoryLines;
	if (maxScroll < 0) maxScroll = 0;

	if (scrollOffset < maxScroll)
	{
		scrollOffset++;
	}
}

void CommandLine::ScrollDown()
{
	if (scrollOffset > 0)
	{
		scrollOffset--;
	}
}

void CommandLine::AppendString(unsigned char r, unsigned char g, unsigned char b, unsigned char a, std::string str)
{
	history.push_back({r, g, b, a, str});
	if (history.size() > MAX_CMD_HISTORY)
	{
		history.erase(history.begin());
	}
}
void CommandLine::AppendStringLn(unsigned char r, unsigned char g, unsigned char b, unsigned char a, std::string str)
{
	history.push_back({r, g, b, a, str + "\n"});
	if (history.size() > MAX_CMD_HISTORY)
	{
		history.erase(history.begin());
	}
}

void CommandLine::DrawImpl()
{
	static float animationProgress = 0.0f;
	static double lastTime = glfwGetTime();
	double currentTime = glfwGetTime();
	float deltaTime = (float) (currentTime - lastTime);
	lastTime = currentTime;


	if (deltaTime > 0.1f) deltaTime = 0.1f;

	// Update animation progress (slide duration: ~0.15s at speed 6.5f)
	float animationSpeed = 8.5f;
	if (isOpen)
	{
		animationProgress = Math::lerp(animationProgress, 1.0f, animationSpeed * deltaTime);
	}
	else
	{
		animationProgress = Math::lerp(animationProgress, 0.0f, animationSpeed * deltaTime);
	}

	if (animationProgress <= 0.0f) return;

	EnvVar winHeightVar = envVars->getVar("WinY");
	int winHeight = std::stoi(winHeightVar.value);
	EnvVar winWidthVar = envVars->getVar("WinX");
	int winWidth = std::stoi(winWidthVar.value);
	float width = (float) winWidth;
	float height = (float) winHeight;

	if (!consoleInitialized)
	{
		initConsoleGL();
	}

	// Save state
	GLint lastProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &lastProgram);
	GLint lastVAO;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &lastVAO);
	GLint lastVBO;
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &lastVBO);
	GLint lastEBO;
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &lastEBO);
	GLboolean blendEnabled = glIsEnabled(GL_BLEND);
	GLint lastBlendSrc, lastBlendDst;
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &lastBlendSrc);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &lastBlendDst);

	// ADD THIS: Save and disable depth testing for UI rendering
	GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(*shaderProgramID);
	glBindVertexArray(*VAO);

	// Set resolution uniform
	glUniform2f(glGetUniformLocation(*shaderProgramID, "u_resolution"), width, height);

	// 1. Draw Panel Background
	float panelHeight = height * 0.45f;
	if (panelHeight < 200.0f) panelHeight = 200.0f;

	float yOffset = -panelHeight * (1.0f - animationProgress);

	ConsoleVertex panelVertices[4] = {
		{ 0.0f,  yOffset,               0.0f, { 30, 30, 30, 200 } },
		{ width, yOffset,               0.0f, { 30, 30, 30, 200 } },
		{ width, yOffset + panelHeight, 0.0f, { 30, 30, 30, 200 } },
		{ 0.0f,  yOffset + panelHeight, 0.0f, { 30, 30, 30, 200 } }
	};

	glBindBuffer(GL_ARRAY_BUFFER, *VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(ConsoleVertex), panelVertices);

	glUniform2f(glGetUniformLocation(*shaderProgramID, "u_scale"), 1.0f, 1.0f);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

	// 2. Draw Separator Line
	float sepY = panelHeight - 12.0f;
	ConsoleVertex sepVertices[4] = {
		{ 0.0f,  yOffset + sepY,        0.0f, { 80, 80, 80, 255 } },
		{ width, yOffset + sepY,        0.0f, { 80, 80, 80, 255 } },
		{ width, yOffset + sepY + 2.0f, 0.0f, { 80, 80, 80, 255 } },
		{ 0.0f,  yOffset + sepY + 2.0f, 0.0f, { 80, 80, 80, 255 } }
	};
	glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(ConsoleVertex), sepVertices);
	glUniform2f(glGetUniformLocation(*shaderProgramID, "u_scale"), 1.0f, 1.0f);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

	float lineSpacing = 24.0f;
	int maxHistoryLines = (int) ((panelHeight - 30.0f) / lineSpacing);
	if (maxHistoryLines < 1) maxHistoryLines = 1;

	// 2.5 Draw Scroll Bar (if history overflows the viewport)
	int totalLines = (int) history.size();
	if (totalLines > maxHistoryLines)
	{
		float trackTop = yOffset + 5.0f;
		float trackBottom = yOffset + sepY - 5.0f;
		float trackHeight = trackBottom - trackTop;

		float scrollbarWidth = 6.0f;
		float scrollbarRightMargin = 6.0f;
		float barX1 = width - scrollbarWidth - scrollbarRightMargin;
		float barX2 = width - scrollbarRightMargin;

		// Draw track background
		ConsoleVertex trackVertices[4] = {
			{ barX1, trackTop,    0.0f, { 15, 15, 15, 150 } },
			{ barX2, trackTop,    0.0f, { 15, 15, 15, 150 } },
			{ barX2, trackBottom, 0.0f, { 15, 15, 15, 150 } },
			{ barX1, trackBottom, 0.0f, { 15, 15, 15, 150 } }
		};
		glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(ConsoleVertex), trackVertices);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

		// Calculate thumb dimensions
		float thumbHeight = trackHeight * ((float) maxHistoryLines / (float) totalLines);
		if (thumbHeight < 15.0f) thumbHeight = 15.0f;

		int maxScroll = totalLines - maxHistoryLines;
		float scrollPercent = (float) scrollOffset / (float) maxScroll;

		// scrollOffset == 0 is bottom (newest messages), so thumb is at the bottom of the track
		// scrollOffset == maxScroll is top (oldest messages), so thumb is at the top of the track
		float thumbTop = (trackBottom - thumbHeight) - scrollPercent * (trackHeight - thumbHeight);
		float thumbBottom = thumbTop + thumbHeight;

		// Draw thumb
		ConsoleVertex thumbVertices[4] = {
			{ barX1, thumbTop,    0.0f, { 120, 120, 120, 230 } },
			{ barX2, thumbTop,    0.0f, { 120, 120, 120, 230 } },
			{ barX2, thumbBottom, 0.0f, { 120, 120, 120, 230 } },
			{ barX1, thumbBottom, 0.0f, { 120, 120, 120, 230 } }
		};
		glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(ConsoleVertex), thumbVertices);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	}

	// 3. Draw History Text

	static ConsoleVertex textQuads[12000];
	unsigned char textColor[4] = {240, 240, 240, 255};
	unsigned char inputColor[4] = {100, 200, 255, 255};

	float currentY = (yOffset + 6.0f) / 2.0f;
	float yOffsetStep = 12.0f; // 12px * 2.0 scale = 24px step size on-screen

	glUniform2f(glGetUniformLocation(*shaderProgramID, "u_scale"), 2.0f, 2.0f);

	int endIdx = (int) history.size() - 1 - scrollOffset;
	if (endIdx >= 0)
	{
		int startIdx = endIdx - (maxHistoryLines - 1);
		if (startIdx < 0) startIdx = 0;
		for (int i = startIdx; i <= endIdx; ++i)
		{
			const auto& item = history[i];
			unsigned char itemColor[4] = {item.r, item.g, item.b, item.a};
			int numQuads = stb_easy_font_print(10.0f / 2.0f, currentY, (char*) item.content.c_str(), itemColor, textQuads, sizeof(textQuads));

			if (numQuads > 0)
			{
				if (numQuads > 1990) numQuads = 1990;
				glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(numQuads * 4 * sizeof(ConsoleVertex)), textQuads);
				glDrawElements(GL_TRIANGLES, static_cast<GLuint>(numQuads * 6), GL_UNSIGNED_INT, nullptr);
			}
			currentY += yOffsetStep;
		}
	}

	// 4. Draw Input Line at the bottom
	float inputY = (yOffset + panelHeight - 25.0f) / 2.0f;
	std::string inputStr = "> " + currentInput + "_";
	int numQuads = stb_easy_font_print(10.0f / 2.0f, inputY, (char*) inputStr.c_str(), inputColor, textQuads, sizeof(textQuads));
	if (numQuads > 0)
	{
		if (numQuads > 1990) numQuads = 1990;
		glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(numQuads * 4 * sizeof(ConsoleVertex)), textQuads);
		glDrawElements(GL_TRIANGLES, static_cast<GLuint>(numQuads * 6), GL_UNSIGNED_INT, nullptr);
	}

	// Restore state
	glUseProgram(lastProgram);
	glBindVertexArray(lastVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lastVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lastEBO);
	if (depthTestEnabled)
	{
		glEnable(GL_DEPTH_TEST);
	}
	if (!blendEnabled)
	{
		glDisable(GL_BLEND);
	}
	else
	{
		glBlendFunc(lastBlendSrc, lastBlendDst);
	}
}