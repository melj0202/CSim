#include "CommandLine.h"
#include "CellMain.h"
#include "Rendering/Renderer.h"
#include "Rendering/IShaderProgram.h"
#include "Rendering/IMesh.h"
#include "Services/Logger.h"
#include "thirdparty/stb/stb_easy_font.h"
#include <sstream>
#include <iostream>
#include <cstdint>
#include <cctype>
#include <cstring>
#include <vector>

namespace {
const char* kConsoleVertexShader = R"(
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
)";

const char* kConsoleFragmentShader = R"(
#version 330 core
in vec4 ourColor;
out vec4 FragColor;
void main() {
    FragColor = ourColor;
}
)";
}

CommandLine::CommandLine(IEnvVars* vars, CommandRegistry* commandRegistry, IRenderWindow* win, Renderer* rendererIn)
	: envVars(vars)
	, window(win)
	, commandRegistry(commandRegistry)
	, renderer(rendererIn)
	, meshHandle(0)
	, shaderHandle(0)
	, gpuReady(false)
	, animationProgress(0.0f)
	, lastAnimTime(std::chrono::high_resolution_clock::now())
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
	consoleInitialized = false;
	enrollGpuResources();
}

void CommandLine::enrollGpuResources()
{
	gpuReady = false;
	consoleInitialized = false;
	if (!renderer)
	{
		Logger::LogError("CommandLine: no Renderer — cannot enroll GPU resources");
		return;
	}

	const int maxQuads = 2000;
	std::vector<unsigned int> indices(static_cast<size_t>(maxQuads * 6));
	for (int i = 0; i < maxQuads; ++i)
	{
		indices[static_cast<size_t>(i * 6 + 0)] = static_cast<unsigned int>(i * 4 + 0);
		indices[static_cast<size_t>(i * 6 + 1)] = static_cast<unsigned int>(i * 4 + 1);
		indices[static_cast<size_t>(i * 6 + 2)] = static_cast<unsigned int>(i * 4 + 2);
		indices[static_cast<size_t>(i * 6 + 3)] = static_cast<unsigned int>(i * 4 + 2);
		indices[static_cast<size_t>(i * 6 + 4)] = static_cast<unsigned int>(i * 4 + 3);
		indices[static_cast<size_t>(i * 6 + 5)] = static_cast<unsigned int>(i * 4 + 0);
	}

	const size_t vboBytes = static_cast<size_t>(maxQuads) * 4 * sizeof(ConsoleVertex);
	meshHandle = renderer->allocateHandle();
	renderer->enrollDynamicMesh(
		vboBytes,
		indices.data(),
		indices.size() * sizeof(unsigned int),
		meshHandle,
		MeshVertexLayout::Pos3Color4U8);

	ShaderSources sources;
	sources.vertexSource = kConsoleVertexShader;
	sources.fragmentSource = kConsoleFragmentShader;
	shaderHandle = renderer->allocateHandle();
	renderer->enrollShader(sources, shaderHandle);

	historyIndex = static_cast<int>(commandHistory.size());
	consoleInitialized = true;
	gpuReady = true;
	Logger::LogTrace("CommandLine enrolled (token path)");
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
		logNormal("                      HIGHLIFE, LIFE_WITHOUT_DEATH, SEEDS, WIREWORLD");
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
			logNormal("  HIGHLIFE | LIFE_WITHOUT_DEATH | SEEDS | WIREWORLD");
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
				mode == "SEEDS" ||
				mode == "WIREWORLD";

			if (!known)
			{
				logError("Unknown ruleset '" + args[0] + "'");
				logNormal("Try: GAME_OF_LIFE, BRIANS_BRAIN, DAY_AND_NIGHT, HIGHLIFE, LIFE_WITHOUT_DEATH, SEEDS, WIREWORLD");
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
	// Migrated to tokens.
}

namespace {

struct UiVert {
	float x, y, z;
	uint8_t color[4];
};

static unsigned int packSolidQuad(
	UiVert* dest,
	unsigned int destCap,
	unsigned int writeAt,
	float x0, float y0, float x1, float y1,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	if (writeAt + 4 > destCap)
	{
		return writeAt;
	}
	dest[writeAt + 0] = { x0, y0, 0.0f, { r, g, b, a } };
	dest[writeAt + 1] = { x1, y0, 0.0f, { r, g, b, a } };
	dest[writeAt + 2] = { x1, y1, 0.0f, { r, g, b, a } };
	dest[writeAt + 3] = { x0, y1, 0.0f, { r, g, b, a } };
	return writeAt + 4;
}

// stb_easy_font half-scale coords; bake ×2 so shader uses u_scale=(1,1).
static unsigned int packFontLine(
	UiVert* dest,
	unsigned int destCap,
	unsigned int writeAt,
	float x, float y,
	const char* text,
	unsigned char color[4])
{
	if (writeAt >= destCap || text == nullptr)
	{
		return writeAt;
	}
	const unsigned int remaining = destCap - writeAt;
	int numQuads = stb_easy_font_print(
		x * 0.5f,
		y * 0.5f,
		const_cast<char*>(text),
		color,
		&dest[writeAt],
		static_cast<int>(remaining * sizeof(UiVert)));
	if (numQuads <= 0)
	{
		return writeAt;
	}
	if (numQuads > static_cast<int>(remaining / 4))
	{
		numQuads = static_cast<int>(remaining / 4);
	}
	const unsigned int vCount = static_cast<unsigned int>(numQuads * 4);
	for (unsigned int i = 0; i < vCount; ++i)
	{
		dest[writeAt + i].x *= 2.0f;
		dest[writeAt + i].y *= 2.0f;
	}
	return writeAt + vCount;
}

} // namespace

bool CommandLine::AppendCommands(Renderer* r)
{
	if (!isVisible())
	{
		return true;
	}
	if (!gpuReady || !r)
	{
		return false;
	}

	// Animation
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	float deltaTime = std::chrono::duration<float>(now - lastAnimTime).count();
	lastAnimTime = now;
	if (deltaTime > 0.1f)
	{
		deltaTime = 0.1f;
	}
	const float animationSpeed = 8.5f;
	if (isOpen)
	{
		animationProgress = Math::lerp(animationProgress, 1.0f, animationSpeed * deltaTime);
	}
	else
	{
		animationProgress = Math::lerp(animationProgress, 0.0f, animationSpeed * deltaTime);
	}
	if (animationProgress <= 0.0f)
	{
		return true;
	}

	EnvVar winHeightVar = envVars->getVar("WinY");
	int winHeight = std::stoi(winHeightVar.value);
	EnvVar winWidthVar = envVars->getVar("WinX");
	int winWidth = std::stoi(winWidthVar.value);
	float width = static_cast<float>(winWidth);
	float height = static_cast<float>(winHeight);

	float panelHeight = height * 0.45f;
	if (panelHeight < 200.0f)
	{
		panelHeight = 200.0f;
	}
	float yOffset = -panelHeight * (1.0f - animationProgress);
	float sepY = panelHeight - 12.0f;

	float lineSpacing = 24.0f;
	int maxHistoryLines = static_cast<int>((panelHeight - 30.0f) / lineSpacing);
	if (maxHistoryLines < 1)
	{
		maxHistoryLines = 1;
	}

	// Pack entire console (chrome + text) into one vertex buffer for a single
	// UpdateBuffer + DrawIndexed (P2). Index buffer is sequential quads.
	const unsigned int kCap = kUiVertCap;
	unsigned int packed = 0;
	UiVert* batch = reinterpret_cast<UiVert*>(uiVerts);

	// 1. Panel
	packed = packSolidQuad(batch, kCap, packed, 0.0f, yOffset, width, yOffset + panelHeight, 30, 30, 30, 200);
	// 2. Separator
	packed = packSolidQuad(batch, kCap, packed, 0.0f, yOffset + sepY, width, yOffset + sepY + 2.0f, 80, 80, 80, 255);

	// 2.5 Scroll bar
	int totalLines = static_cast<int>(history.size());
	if (totalLines > maxHistoryLines)
	{
		float trackTop = yOffset + 5.0f;
		float trackBottom = yOffset + sepY - 5.0f;
		float trackHeight = trackBottom - trackTop;
		float scrollbarWidth = 6.0f;
		float scrollbarRightMargin = 6.0f;
		float barX1 = width - scrollbarWidth - scrollbarRightMargin;
		float barX2 = width - scrollbarRightMargin;

		packed = packSolidQuad(batch, kCap, packed, barX1, trackTop, barX2, trackBottom, 15, 15, 15, 150);

		float thumbHeight = trackHeight * (static_cast<float>(maxHistoryLines) / static_cast<float>(totalLines));
		if (thumbHeight < 15.0f)
		{
			thumbHeight = 15.0f;
		}
		int maxScroll = totalLines - maxHistoryLines;
		float scrollPercent = (maxScroll > 0)
			? (static_cast<float>(scrollOffset) / static_cast<float>(maxScroll))
			: 0.0f;
		float thumbTop = (trackBottom - thumbHeight) - scrollPercent * (trackHeight - thumbHeight);
		float thumbBottom = thumbTop + thumbHeight;
		packed = packSolidQuad(batch, kCap, packed, barX1, thumbTop, barX2, thumbBottom, 120, 120, 120, 230);
	}

	// 3. History text (screen-space; scale baked into verts)
	float currentY = yOffset + 6.0f;
	int endIdx = static_cast<int>(history.size()) - 1 - scrollOffset;
	if (endIdx >= 0)
	{
		int startIdx = endIdx - (maxHistoryLines - 1);
		if (startIdx < 0)
		{
			startIdx = 0;
		}
		for (int i = startIdx; i <= endIdx; ++i)
		{
			const historyBuffer& item = history[static_cast<size_t>(i)];
			unsigned char itemColor[4] = { item.r, item.g, item.b, item.a };
			packed = packFontLine(batch, kCap, packed, 10.0f, currentY, item.content.c_str(), itemColor);
			currentY += lineSpacing;
		}
	}

	// 4. Input line
	unsigned char inputColor[4] = {100, 200, 255, 255};
	float inputY = yOffset + panelHeight - 25.0f;
	std::string inputStr = "> " + currentInput + "_";
	packed = packFontLine(batch, kCap, packed, 10.0f, inputY, inputStr.c_str(), inputColor);

	if (packed < 4)
	{
		return true;
	}

	const unsigned int totalQuads = packed / 4;
	// Dynamic mesh index buffer covers 2000 quads; clamp draw if somehow larger.
	unsigned int drawQuads = totalQuads;
	if (drawQuads > 2000)
	{
		drawQuads = 2000;
	}

	PipelineState ps;
	ps.depthTestEnabled = false;
	ps.blendEnabled = true;
	ps.blendSrc = BlendFactor::SrcAlpha;
	ps.blendDst = BlendFactor::OneMinusSrcAlpha;
	ps.faceCullingEnabled = false;
	ps.primitives = Primitives::Triangles;
	r->pushPipelineState(ps);
	r->pushSetShader(shaderHandle);
	r->pushSetMesh(meshHandle);
	r->pushUniformVec2("u_resolution", width, height);
	r->pushUniformVec2("u_scale", 1.0f, 1.0f);

	r->pushUpdateBuffer(
		meshHandle,
		0,
		static_cast<unsigned int>(drawQuads * 4 * sizeof(ConsoleVertex)),
		uiVerts);
	r->pushDrawIndexed(drawQuads * 6, 0);

	return true;
}