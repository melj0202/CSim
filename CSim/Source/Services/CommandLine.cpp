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
	// Migrated to tokens.
}

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

	float panelHeight = height * 0.45f;
	if (panelHeight < 200.0f)
	{
		panelHeight = 200.0f;
	}
	float yOffset = -panelHeight * (1.0f - animationProgress);

	// 1. Panel
	panelVertices[0] = { 0.0f, yOffset, 0.0f, { 30, 30, 30, 200 } };
	panelVertices[1] = { width, yOffset, 0.0f, { 30, 30, 30, 200 } };
	panelVertices[2] = { width, yOffset + panelHeight, 0.0f, { 30, 30, 30, 200 } };
	panelVertices[3] = { 0.0f, yOffset + panelHeight, 0.0f, { 30, 30, 30, 200 } };
	r->pushUpdateBuffer(meshHandle, 0, static_cast<unsigned int>(4 * sizeof(ConsoleVertex)), panelVertices);
	r->pushUniformVec2("u_scale", 1.0f, 1.0f);
	r->pushDrawIndexed(6, 0);

	// 2. Separator
	float sepY = panelHeight - 12.0f;
	sepVertices[0] = { 0.0f, yOffset + sepY, 0.0f, { 80, 80, 80, 255 } };
	sepVertices[1] = { width, yOffset + sepY, 0.0f, { 80, 80, 80, 255 } };
	sepVertices[2] = { width, yOffset + sepY + 2.0f, 0.0f, { 80, 80, 80, 255 } };
	sepVertices[3] = { 0.0f, yOffset + sepY + 2.0f, 0.0f, { 80, 80, 80, 255 } };
	r->pushUpdateBuffer(meshHandle, 0, static_cast<unsigned int>(4 * sizeof(ConsoleVertex)), sepVertices);
	r->pushUniformVec2("u_scale", 1.0f, 1.0f);
	r->pushDrawIndexed(6, 0);

	float lineSpacing = 24.0f;
	int maxHistoryLines = static_cast<int>((panelHeight - 30.0f) / lineSpacing);
	if (maxHistoryLines < 1)
	{
		maxHistoryLines = 1;
	}

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

		trackVertices[0] = { barX1, trackTop, 0.0f, { 15, 15, 15, 150 } };
		trackVertices[1] = { barX2, trackTop, 0.0f, { 15, 15, 15, 150 } };
		trackVertices[2] = { barX2, trackBottom, 0.0f, { 15, 15, 15, 150 } };
		trackVertices[3] = { barX1, trackBottom, 0.0f, { 15, 15, 15, 150 } };
		r->pushUpdateBuffer(meshHandle, 0, static_cast<unsigned int>(4 * sizeof(ConsoleVertex)), trackVertices);
		r->pushDrawIndexed(6, 0);

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

		thumbVertices[0] = { barX1, thumbTop, 0.0f, { 120, 120, 120, 230 } };
		thumbVertices[1] = { barX2, thumbTop, 0.0f, { 120, 120, 120, 230 } };
		thumbVertices[2] = { barX2, thumbBottom, 0.0f, { 120, 120, 120, 230 } };
		thumbVertices[3] = { barX1, thumbBottom, 0.0f, { 120, 120, 120, 230 } };
		r->pushUpdateBuffer(meshHandle, 0, static_cast<unsigned int>(4 * sizeof(ConsoleVertex)), thumbVertices);
		r->pushDrawIndexed(6, 0);
	}

	// 3. History text — one UpdateBuffer+Draw per line.
	// Note: textQuads is reused; each push stores the pointer, so we must
	// submit line-by-line only after execute, OR buffer each line separately.
	// Pointers remain valid until SubmitCommandQueue; we overwrite textQuads
	// between pushes, so all but the last line would be wrong if we only
	// keep one buffer. Use sequential upload: execute path runs after all
	// AppendCommands, so concurrent pointers to the same buffer break.
	// Fix: push each line into queue with data that must still be correct at
	// submit — allocate temporary stack is wrong. Use a growable staging area
	// or draw one line with immediate... Best fix for multi-line: pack all
	// history into one big buffer and draw as multiple regions, OR copy each
	// line into a persistent ring of slots.
	//
	// Practical approach: store each line's quads into a staging vector of
	// unique allocations is heavy. Simpler: rebuild all visible text into
	// sequential regions of a large staging buffer (textStaging).
	//
	// textQuads holds 12000 verts; use contiguous packing.
	unsigned char inputColor[4] = {100, 200, 255, 255};
	float currentY = (yOffset + 6.0f) / 2.0f;
	float yOffsetStep = 12.0f;

	// Pack history lines into textQuads sequentially, record draw ranges.
	// Max 32 visible lines * ~200 quads is safe within 12000 verts.
	struct LineDraw {
		unsigned int vertexOffset; // in vertices
		unsigned int elementCount;
	};
	LineDraw lineDraws[64];
	int lineDrawCount = 0;
	unsigned int packedVertexCount = 0;

	r->pushUniformVec2("u_scale", 2.0f, 2.0f);

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
			if (lineDrawCount >= 64)
			{
				break;
			}
			const historyBuffer& item = history[static_cast<size_t>(i)];
			unsigned char itemColor[4] = { item.r, item.g, item.b, item.a };
			// Temporary into local area then copy into packed region
			ConsoleVertex tempQuads[2000 * 4];
			int numQuads = stb_easy_font_print(
				10.0f / 2.0f,
				currentY,
				const_cast<char*>(item.content.c_str()),
				itemColor,
				tempQuads,
				sizeof(tempQuads));
			if (numQuads > 0)
			{
				if (numQuads > 1990)
				{
					numQuads = 1990;
				}
				const unsigned int vCount = static_cast<unsigned int>(numQuads * 4);
				if (packedVertexCount + vCount > 12000)
				{
					break;
				}
				std::memcpy(
					&textQuads[packedVertexCount],
					tempQuads,
					vCount * sizeof(ConsoleVertex));
				lineDraws[lineDrawCount].vertexOffset = packedVertexCount;
				lineDraws[lineDrawCount].elementCount = static_cast<unsigned int>(numQuads * 6);
				++lineDrawCount;
				packedVertexCount += vCount;
			}
			currentY += yOffsetStep;
		}
	}

	// Input line packed after history
	float inputY = (yOffset + panelHeight - 25.0f) / 2.0f;
	std::string inputStr = "> " + currentInput + "_";
	{
		ConsoleVertex tempQuads[2000 * 4];
		int numQuads = stb_easy_font_print(
			10.0f / 2.0f,
			inputY,
			const_cast<char*>(inputStr.c_str()),
			inputColor,
			tempQuads,
			sizeof(tempQuads));
		if (numQuads > 0)
		{
			if (numQuads > 1990)
			{
				numQuads = 1990;
			}
			const unsigned int vCount = static_cast<unsigned int>(numQuads * 4);
			if (packedVertexCount + vCount <= 12000 && lineDrawCount < 64)
			{
				std::memcpy(
					&textQuads[packedVertexCount],
					tempQuads,
					vCount * sizeof(ConsoleVertex));
				lineDraws[lineDrawCount].vertexOffset = packedVertexCount;
				lineDraws[lineDrawCount].elementCount = static_cast<unsigned int>(numQuads * 6);
				++lineDrawCount;
				packedVertexCount += vCount;
			}
		}
	}

	// One upload of all packed text, then draw each line with base-vertex offset.
	// Our DrawIndexed only has firstIndex, not baseVertex — so we must upload
	// each line separately with correct firstIndex=0 after rewriting indices
	// OR re-upload one line at a time into VBO start.
	// Re-upload one line at a time from packed buffer (pointer stays valid).
	for (int li = 0; li < lineDrawCount; ++li)
	{
		const unsigned int vOff = lineDraws[li].vertexOffset;
		const unsigned int eCount = lineDraws[li].elementCount;
		const unsigned int vCount = eCount / 6 * 4;
		r->pushUpdateBuffer(
			meshHandle,
			0,
			static_cast<unsigned int>(vCount * sizeof(ConsoleVertex)),
			&textQuads[vOff]);
		r->pushDrawIndexed(eCount, 0);
	}

	return true;
}