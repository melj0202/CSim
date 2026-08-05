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
#include <algorithm>

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

struct BuiltInCommandHelp {
	const char* name;
	const char* usage;
	const char* description;
};

const BuiltInCommandHelp kBuiltInCommands[] = {
	{"clear", "clear", "Clear console output"},
	{"close", "close", "Close the console"},
	{"echo", "echo <text>", "Print text to the console"},
	{"fade", "fade [0..1000]", "Show or set cell fade speed"},
	{"fps", "fps [on|off|toggle]", "Show or change the FPS overlay"},
	{"fullscreen", "fullscreen [on|off|toggle]", "Show or change fullscreen mode"},
	{"get", "get <variable>", "Read an environment variable"},
	{"help", "help [command]", "Show commands or detailed help"},
	{"quit", "quit", "Exit Illumo"},
	{"set", "set <variable> <value>", "Create or update an environment variable"},
	{"speed", "speed [0.01..100]", "Show or set the simulation speed multiplier"},
	{"toggle", "toggle <variable>", "Toggle a boolean environment variable"},
	{"tps", "tps [1..1000]", "Show or set simulation ticks per second"},
	{"vars", "vars [filter]", "List environment variables, optionally filtered"}
};

std::string lowerCopy(const std::string& text)
{
	std::string lowered = text;
	for (std::size_t i = 0; i < lowered.size(); ++i)
	{
		lowered[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[i])));
	}
	return lowered;
}

std::string joinArguments(const std::vector<std::string>& args, std::size_t first)
{
	std::string result;
	for (std::size_t i = first; i < args.size(); ++i)
	{
		if (!result.empty())
		{
			result += " ";
		}
		result += args[i];
	}
	return result;
}

bool parseLongStrict(const std::string& text, long* value)
{
	if (value == nullptr || text.empty())
	{
		return false;
	}
	try
	{
		std::size_t consumed = 0;
		long parsed = std::stol(text, &consumed);
		if (consumed != text.size())
		{
			return false;
		}
		*value = parsed;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool parseDoubleStrict(const std::string& text, double* value)
{
	if (value == nullptr || text.empty())
	{
		return false;
	}
	try
	{
		std::size_t consumed = 0;
		double parsed = std::stod(text, &consumed);
		if (consumed != text.size())
		{
			return false;
		}
		*value = parsed;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool parseBoolValue(const std::string& text, bool* value)
{
	if (value == nullptr)
	{
		return false;
	}
	const std::string lowered = lowerCopy(text);
	if (lowered == "on" || lowered == "true" || lowered == "yes" || lowered == "1")
	{
		*value = true;
		return true;
	}
	if (lowered == "off" || lowered == "false" || lowered == "no" || lowered == "0")
	{
		*value = false;
		return true;
	}
	return false;
}

const BuiltInCommandHelp* findBuiltInCommand(const std::string& name)
{
	for (const BuiltInCommandHelp& command : kBuiltInCommands)
	{
		if (name == command.name)
		{
			return &command;
		}
	}
	return nullptr;
}

std::string findEnvironmentKey(IEnvVars* envVars, const std::string& requested)
{
	if (envVars == nullptr)
	{
		return "";
	}
	const std::string loweredRequested = lowerCopy(requested);
	const std::unordered_map<std::string, EnvVar>& variables = envVars->getVars();
	for (const std::pair<const std::string, EnvVar>& variable : variables)
	{
		if (lowerCopy(variable.first) == loweredRequested)
		{
			return variable.first;
		}
	}
	return "";
}
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
	, cursorPosition(0)
	, selectionAnchor(0)
{
	isOpen = false;
	currentInput = "";
	tempInput = "";
	completionHint = "";
	history = {
		{240, 240, 240, 255, "Illumo Developer Console"},
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

	const unsigned int maxQuads = kUiQuadCap;
	std::vector<unsigned int> indices(static_cast<size_t>(maxQuads * 6));
	for (unsigned int i = 0; i < maxQuads; ++i)
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
		completionHint = "Tab: complete  |  Ctrl+Arrows: words  |  Ctrl+A: select all";
	}
}

void CommandLine::clearCompletionHint()
{
	completionHint.clear();
}

void CommandLine::resetCursorToEnd()
{
	cursorPosition = currentInput.size();
	selectionAnchor = cursorPosition;
}

void CommandLine::eraseSelection()
{
	if (!hasSelection())
	{
		return;
	}

	std::size_t start = std::min(cursorPosition, selectionAnchor);
	std::size_t end = std::max(cursorPosition, selectionAnchor);
	currentInput.erase(start, end - start);
	cursorPosition = start;
	selectionAnchor = start;
}

std::size_t CommandLine::findPreviousWordBoundary() const
{
	std::size_t position = cursorPosition;
	while (position > 0 && std::isspace(static_cast<unsigned char>(currentInput[position - 1])))
	{
		--position;
	}
	while (position > 0 && !std::isspace(static_cast<unsigned char>(currentInput[position - 1])))
	{
		--position;
	}
	return position;
}

std::size_t CommandLine::findNextWordBoundary() const
{
	std::size_t position = cursorPosition;
	while (position < currentInput.size() && !std::isspace(static_cast<unsigned char>(currentInput[position])))
	{
		++position;
	}
	while (position < currentInput.size() && std::isspace(static_cast<unsigned char>(currentInput[position])))
	{
		++position;
	}
	return position;
}

void CommandLine::AddCharacter(unsigned int codepoint)
{
	std::size_t selectedCharacters = hasSelection()
		? std::max(cursorPosition, selectionAnchor) - std::min(cursorPosition, selectionAnchor)
		: 0;
	if (currentInput.size() - selectedCharacters < MAX_CHARS_PER_LINE - 1)
	{
		if (codepoint >= 32 && codepoint <= 126)
		{
			eraseSelection();
			currentInput.insert(cursorPosition, 1, static_cast<char>(codepoint));
			++cursorPosition;
			selectionAnchor = cursorPosition;
			clearCompletionHint();
		}
	}
}

void CommandLine::HandleBackspace(bool byWord)
{
	if (hasSelection())
	{
		eraseSelection();
		clearCompletionHint();
		return;
	}
	if (cursorPosition == 0)
	{
		return;
	}

	std::size_t eraseFrom = byWord ? findPreviousWordBoundary() : cursorPosition - 1;
	currentInput.erase(eraseFrom, cursorPosition - eraseFrom);
	cursorPosition = eraseFrom;
	selectionAnchor = cursorPosition;
	clearCompletionHint();
}

void CommandLine::HandleDelete(bool byWord)
{
	if (hasSelection())
	{
		eraseSelection();
		clearCompletionHint();
		return;
	}
	if (cursorPosition >= currentInput.size())
	{
		return;
	}

	std::size_t eraseTo = byWord ? findNextWordBoundary() : cursorPosition + 1;
	currentInput.erase(cursorPosition, eraseTo - cursorPosition);
	selectionAnchor = cursorPosition;
	clearCompletionHint();
}

void CommandLine::MoveCursorLeft(bool byWord, bool select)
{
	if (!select && hasSelection())
	{
		cursorPosition = std::min(cursorPosition, selectionAnchor);
		selectionAnchor = cursorPosition;
		return;
	}
	std::size_t newPosition = byWord ? findPreviousWordBoundary() : (cursorPosition > 0 ? cursorPosition - 1 : 0);
	if (!select)
	{
		selectionAnchor = newPosition;
	}
	cursorPosition = newPosition;
}

void CommandLine::MoveCursorRight(bool byWord, bool select)
{
	if (!select && hasSelection())
	{
		cursorPosition = std::max(cursorPosition, selectionAnchor);
		selectionAnchor = cursorPosition;
		return;
	}
	std::size_t newPosition = byWord ? findNextWordBoundary() : std::min(cursorPosition + 1, currentInput.size());
	if (!select)
	{
		selectionAnchor = newPosition;
	}
	cursorPosition = newPosition;
}

void CommandLine::MoveCursorHome(bool select)
{
	if (!select)
	{
		selectionAnchor = 0;
	}
	cursorPosition = 0;
}

void CommandLine::MoveCursorEnd(bool select)
{
	if (!select)
	{
		selectionAnchor = currentInput.size();
	}
	cursorPosition = currentInput.size();
}

void CommandLine::SelectAll()
{
	selectionAnchor = 0;
	cursorPosition = currentInput.size();
}

void CommandLine::ClearInput()
{
	currentInput.clear();
	resetCursorToEnd();
	clearCompletionHint();
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

std::vector<std::string> CommandLine::ParseCommandArgs(const std::string& text, const std::string& delim) const
{
	std::vector<std::string> args;
	std::string currentArg;
	char quote = '\0';
	bool escaping = false;
	bool tokenStarted = false;

	for (char character : text)
	{
		if (escaping)
		{
			currentArg += character;
			escaping = false;
			tokenStarted = true;
			continue;
		}
		if (character == '\\')
		{
			escaping = true;
			tokenStarted = true;
			continue;
		}
		if (quote != '\0')
		{
			if (character == quote)
			{
				quote = '\0';
			}
			else
			{
				currentArg += character;
			}
			tokenStarted = true;
			continue;
		}
		if (character == '\'' || character == '"')
		{
			quote = character;
			tokenStarted = true;
			continue;
		}
		if (delim.find(character) != std::string::npos)
		{
			if (tokenStarted)
			{
				args.push_back(currentArg);
				currentArg.clear();
				tokenStarted = false;
			}
			continue;
		}
		currentArg += character;
		tokenStarted = true;
	}

	if (escaping)
	{
		currentArg += '\\';
	}
	if (tokenStarted)
	{
		args.push_back(currentArg);
	}
	return args;
}

std::vector<std::string> CommandLine::getCompletionCandidates(const std::string& leadingText) const
{
	std::vector<std::string> candidates;
	std::vector<std::string> leadingArgs = ParseCommandArgs(leadingText, " \t");
	if (leadingArgs.empty())
	{
		for (const BuiltInCommandHelp& command : kBuiltInCommands)
		{
			candidates.push_back(command.name);
		}

		if (commandRegistry != nullptr)
		{
			std::vector<std::string> registeredCommands = commandRegistry->GetCommandNames();
			candidates.insert(candidates.end(), registeredCommands.begin(), registeredCommands.end());
		}
		const std::unordered_map<std::string, EnvVar>& vars = envVars->getVars();
		for (const std::pair<const std::string, EnvVar>& variable : vars)
		{
			candidates.push_back(variable.first);
		}
	}
	else
	{
		const std::string command = lowerCopy(leadingArgs[0]);
		if (commandRegistry != nullptr && commandRegistry->HasCommand(command))
		{
			candidates = commandRegistry->GetCommandCompletions(command);
		}
		else if (command == "get" || command == "set" || command == "toggle" || command == "vars")
		{
			const std::unordered_map<std::string, EnvVar>& vars = envVars->getVars();
			for (const std::pair<const std::string, EnvVar>& variable : vars)
			{
				candidates.push_back(variable.first);
			}
		}
		else if (command == "fps" || command == "fullscreen")
		{
			candidates = {"off", "on", "toggle"};
		}
	}

	std::sort(candidates.begin(), candidates.end());
	candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
	return candidates;
}

void CommandLine::Complete()
{
	std::size_t tokenStart = cursorPosition;
	while (tokenStart > 0 && !std::isspace(static_cast<unsigned char>(currentInput[tokenStart - 1])))
	{
		--tokenStart;
	}
	const std::string prefix = currentInput.substr(tokenStart, cursorPosition - tokenStart);
	const std::string leadingText = currentInput.substr(0, tokenStart);
	std::vector<std::string> candidates = getCompletionCandidates(leadingText);
	std::vector<std::string> matches;
	for (const std::string& candidate : candidates)
	{
		if (candidate.size() < prefix.size())
		{
			continue;
		}

		bool matchesPrefix = true;
		for (std::size_t i = 0; i < prefix.size(); ++i)
		{
			if (std::tolower(static_cast<unsigned char>(candidate[i])) != std::tolower(static_cast<unsigned char>(prefix[i])))
			{
				matchesPrefix = false;
				break;
			}
		}
		if (matchesPrefix)
		{
			matches.push_back(candidate);
		}
	}

	if (matches.empty())
	{
		completionHint = "No completion matches '" + prefix + "'";
		return;
	}

	std::string replacement = matches[0];
	for (std::size_t i = 1; i < matches.size(); ++i)
	{
		std::size_t commonLength = 0;
		while (commonLength < replacement.size() && commonLength < matches[i].size() && replacement[commonLength] == matches[i][commonLength])
		{
			++commonLength;
		}
		replacement.resize(commonLength);
	}

	if (replacement.size() > prefix.size() || matches.size() == 1)
	{
		currentInput.replace(tokenStart, cursorPosition - tokenStart, replacement);
		cursorPosition = tokenStart + replacement.size();
		selectionAnchor = cursorPosition;
	}

	if (matches.size() == 1)
	{
		if (tokenStart == 0 && cursorPosition == currentInput.size())
		{
			currentInput += " ";
			++cursorPosition;
			selectionAnchor = cursorPosition;
		}
		completionHint = "Completed: " + matches[0];
		return;
	}

	completionHint = "Matches: ";
	const std::size_t visibleMatches = std::min(matches.size(), static_cast<std::size_t>(4));
	for (std::size_t i = 0; i < visibleMatches; ++i)
	{
		if (i > 0)
		{
			completionHint += "  ";
		}
		completionHint += matches[i];
	}
	if (matches.size() > visibleMatches)
	{
		completionHint += "  ...";
	}
}

void CommandLine::ExecuteCommand()
{
	std::vector<std::string> commandParts = ParseCommandArgs(currentInput, " \t");
	if (commandParts.empty())
	{
		return;
	}

	AppendString(100, 200, 255, 255, "> " + currentInput);

	const std::string rawCommand = commandParts[0];
	const std::string cmd = lowerCopy(rawCommand);
	std::vector<std::string> args(commandParts.begin() + 1, commandParts.end());

	AddToHistory(currentInput);

	if (cmd == "help")
	{
		if (args.empty())
		{
			logNormal("Built-in commands:");
			for (const BuiltInCommandHelp& command : kBuiltInCommands)
			{
				logNormal("  " + std::string(command.usage) + " - " + command.description);
			}

			if (commandRegistry != nullptr)
			{
				std::vector<std::string> registeredCommands = commandRegistry->GetCommandNames();
				if (!registeredCommands.empty())
				{
					logNormal("Simulation commands:");
				}
				for (const std::string& commandName : registeredCommands)
				{
					std::string usage = commandRegistry->GetCommandUsage(commandName);
					std::string description = commandRegistry->GetCommandDescription(commandName);
					if (usage.empty())
					{
						usage = commandName;
					}
					logNormal("  " + usage + (description.empty() ? "" : " - " + description));
				}
			}
			logNormal("Use 'help <command>' for one command.");
		}
		else
		{
			const std::string requested = lowerCopy(args[0]);
			const BuiltInCommandHelp* builtIn = findBuiltInCommand(requested);
			if (builtIn != nullptr)
			{
				logNormal(std::string(builtIn->usage) + " - " + builtIn->description);
			}
			else if (commandRegistry != nullptr && commandRegistry->HasCommand(requested))
			{
				std::string usage = commandRegistry->GetCommandUsage(requested);
				std::string description = commandRegistry->GetCommandDescription(requested);
				logNormal((usage.empty() ? requested : usage) + (description.empty() ? "" : " - " + description));
			}
			else
			{
				logError("No help available for '" + args[0] + "'");
			}
		}
	}
	else if (cmd == "clear")
	{
		history.clear();
		AppendString(240, 240, 240, 255, "Illumo Developer Console");
	}
	else if (cmd == "echo")
	{
		logNormal(joinArguments(args, 0));
	}
	else if (cmd == "get")
	{
		if (args.size() != 1)
		{
			logNormal("Usage: get <variable>");
		}
		else
		{
			const std::string key = findEnvironmentKey(envVars, args[0]);
			if (key.empty())
			{
				logError("Unknown variable: " + args[0]);
			}
			else
			{
				logNormal(key + " = " + envVars->getVar(key).value);
			}
		}
	}
	else if (cmd == "set")
	{
		if (args.size() < 2)
		{
			logNormal("Usage: set <variable> <value>");
		}
		else
		{
			std::string key = findEnvironmentKey(envVars, args[0]);
			if (key.empty())
			{
				key = args[0];
			}
			const std::string value = joinArguments(args, 1);
			envVars->setVar(key, value);
			logSuccess(key + " = " + value);
		}
	}
	else if (cmd == "toggle")
	{
		if (args.size() != 1)
		{
			logNormal("Usage: toggle <variable>");
		}
		else
		{
			const std::string key = findEnvironmentKey(envVars, args[0]);
			if (key.empty())
			{
				logError("Unknown variable: " + args[0]);
			}
			else
			{
				const bool value = !envVars->getVar(key).valueAsBool;
				envVars->setVar(key, value);
				logSuccess(key + " = " + (value ? "true" : "false"));
			}
		}
	}
	else if (cmd == "vars")
	{
		const std::string filter = args.empty() ? "" : lowerCopy(args[0]);
		std::vector<std::string> variableLines;
		const std::unordered_map<std::string, EnvVar>& variables = envVars->getVars();
		for (const std::pair<const std::string, EnvVar>& variable : variables)
		{
			if (filter.empty() || lowerCopy(variable.first).find(filter) != std::string::npos)
			{
				variableLines.push_back(variable.first + " = " + variable.second.value);
			}
		}
		std::sort(variableLines.begin(), variableLines.end());
		if (variableLines.empty())
		{
			logWarning("No variables match '" + (args.empty() ? std::string("") : args[0]) + "'");
		}
		for (const std::string& line : variableLines)
		{
			logNormal(line);
		}
	}
	else if (cmd == "tps")
	{
		if (args.empty())
		{
			logNormal("tps = " + envVars->getVar("tps").value);
		}
		else
		{
			long value = 0;
			if (args.size() != 1 || !parseLongStrict(args[0], &value) || value < 1 || value > 1000)
			{
				logError("tps must be an integer from 1 to 1000");
			}
			else
			{
				envVars->setVar("tps", value);
				logSuccess("tps = " + std::to_string(value));
			}
		}
	}
	else if (cmd == "speed" || cmd == "speedfactor")
	{
		if (args.empty())
		{
			logNormal("speedFactor = " + envVars->getVar("speedFactor").value);
		}
		else
		{
			double value = 0.0;
			if (args.size() != 1 || !parseDoubleStrict(args[0], &value) || value < 0.01 || value > 100.0)
			{
				logError("speed must be a number from 0.01 to 100");
			}
			else
			{
				envVars->setVar("speedFactor", args[0]);
				logSuccess("speedFactor = " + args[0]);
			}
		}
	}
	else if (cmd == "fade" || cmd == "cellfadespeed")
	{
		if (args.empty())
		{
			logNormal("cellFadeSpeed = " + envVars->getVar("cellFadeSpeed").value);
		}
		else
		{
			double value = 0.0;
			if (args.size() != 1 || !parseDoubleStrict(args[0], &value) || value < 0.0 || value > 1000.0)
			{
				logError("fade must be a number from 0 to 1000");
			}
			else
			{
				envVars->setVar("cellFadeSpeed", args[0]);
				logSuccess("cellFadeSpeed = " + args[0]);
			}
		}
	}
	else if (cmd == "fps")
	{
		const bool currentValue = envVars->getVar("showFPS").valueAsBool;
		if (args.empty())
		{
			logNormal(std::string("FPS overlay: ") + (currentValue ? "on" : "off"));
		}
		else
		{
			bool requestedValue = false;
			bool valid = false;
			if (args.size() == 1 && lowerCopy(args[0]) == "toggle")
			{
				requestedValue = !currentValue;
				valid = true;
			}
			else if (args.size() == 1)
			{
				valid = parseBoolValue(args[0], &requestedValue);
			}
			if (!valid)
			{
				logError("Usage: fps [on|off|toggle]");
			}
			else
			{
				envVars->setVar("showFPS", requestedValue);
				logSuccess(std::string("FPS overlay: ") + (requestedValue ? "on" : "off"));
			}
		}
	}
	else if (cmd == "fullscreen")
	{
		const bool currentValue = envVars->getVar("fullscreen").valueAsBool;
		bool requestedValue = !currentValue;
		bool valid = args.empty();
		if (args.size() == 1 && lowerCopy(args[0]) == "toggle")
		{
			valid = true;
		}
		else if (args.size() == 1)
		{
			valid = parseBoolValue(args[0], &requestedValue);
		}
		if (!valid)
		{
			logError("Usage: fullscreen [on|off|toggle]");
		}
		else
		{
			if (requestedValue != currentValue)
			{
				window->toggleFullscreen();
			}
			envVars->setVar("fullscreen", requestedValue);
			logSuccess(std::string("Fullscreen: ") + (requestedValue ? "on" : "off"));
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
	else if (cmd == "vid_restart")
	{
		logWarning("vid_restart is unavailable: safely rebuilding the OpenGL context requires resource re-enrollment");
	}
	else
	{
		if (commandRegistry != nullptr && commandRegistry->HasCommand(cmd))
		{
			commandRegistry->QueueCommand(cmd, args);
		}
		else
		{
			const std::string key = findEnvironmentKey(envVars, rawCommand);
			if (!key.empty())
			{
				if (args.empty())
				{
					logNormal(key + " = " + envVars->getVar(key).value);
				}
				else if (args.size() == 1)
				{
					envVars->setVar(key, args[0]);
					logSuccess(key + " = " + args[0]);
				}
				else
				{
					logError("Variable assignment accepts one value; use set for text with spaces");
				}
			}
			else
			{
				logError("Unknown command or variable: " + rawCommand);
			}
		}
	}

	ClearInput();
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
	resetCursorToEnd();
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
		resetCursorToEnd();
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
		resetCursorToEnd();
	}
}

void CommandLine::ScrollUp()
{
	std::array<int, 2> windowDimensions = window->getWindowDimensions();
	int winHeight = windowDimensions[1];
	float panelHeight = winHeight * 0.52f;
	if (panelHeight < 240.0f) panelHeight = 240.0f;
	float lineSpacing = 24.0f;
	int maxHistoryLines = (int) ((panelHeight - 90.0f) / lineSpacing);
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

static unsigned int packVerticalGradientQuad(
	UiVert* dest,
	unsigned int destCap,
	unsigned int writeAt,
	float x0, float y0, float x1, float y1,
	unsigned char topR, unsigned char topG, unsigned char topB, unsigned char topA,
	unsigned char bottomR, unsigned char bottomG, unsigned char bottomB, unsigned char bottomA)
{
	if (writeAt + 4 > destCap)
	{
		return writeAt;
	}
	dest[writeAt + 0] = { x0, y0, 0.0f, { topR, topG, topB, topA } };
	dest[writeAt + 1] = { x1, y0, 0.0f, { topR, topG, topB, topA } };
	dest[writeAt + 2] = { x1, y1, 0.0f, { bottomR, bottomG, bottomB, bottomA } };
	dest[writeAt + 3] = { x0, y1, 0.0f, { bottomR, bottomG, bottomB, bottomA } };
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

static float measureFontText(const std::string& text)
{
	std::string mutableText = text;
	return static_cast<float>(stb_easy_font_width(mutableText.data()) * 2);
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

	std::array<int, 2> windowDimensions = window->getWindowDimensions();
	int winWidth = windowDimensions[0];
	int winHeight = windowDimensions[1];
	float width = static_cast<float>(winWidth);
	float height = static_cast<float>(winHeight);

	float panelHeight = height * 0.52f;
	if (panelHeight < 240.0f)
	{
		panelHeight = 240.0f;
	}
	if (panelHeight > height - 20.0f)
	{
		panelHeight = height - 20.0f;
	}
	float yOffset = -panelHeight * (1.0f - animationProgress);
	const float headerHeight = 34.0f;
	const float inputRowHeight = 40.0f;
	const float historyTop = yOffset + headerHeight + 8.0f;
	const float inputTop = yOffset + panelHeight - inputRowHeight;
	const float historyBottom = inputTop - 8.0f;
	float lineSpacing = 24.0f;
	int maxHistoryLines = static_cast<int>((historyBottom - historyTop) / lineSpacing);
	if (maxHistoryLines < 1)
	{
		maxHistoryLines = 1;
	}

	// Pack entire console (chrome + text) into one vertex buffer for a single
	// UpdateBuffer + DrawIndexed (P2). Index buffer is sequential quads.
	const unsigned int kCap = kUiVertCap;
	unsigned int packed = 0;
	UiVert* batch = reinterpret_cast<UiVert*>(uiVerts);

	// Layered console chrome: a soft shadow, blue-black gradient, and a clear
	// title/input hierarchy keep the developer UI readable over a busy canvas.
	packed = packSolidQuad(batch, kCap, packed, 4.0f, yOffset + 6.0f, width, yOffset + panelHeight + 6.0f, 0, 0, 0, 110);
	packed = packVerticalGradientQuad(batch, kCap, packed, 0.0f, yOffset, width, yOffset + panelHeight,
		23, 35, 52, 244, 8, 13, 23, 238);
	packed = packVerticalGradientQuad(batch, kCap, packed, 0.0f, yOffset, width, yOffset + headerHeight,
		31, 62, 94, 255, 22, 41, 67, 255);
	packed = packSolidQuad(batch, kCap, packed, 0.0f, yOffset, 4.0f, yOffset + panelHeight, 82, 205, 255, 255);
	packed = packSolidQuad(batch, kCap, packed, 0.0f, yOffset + headerHeight, width, yOffset + headerHeight + 1.0f, 95, 210, 255, 155);
	packed = packSolidQuad(batch, kCap, packed, 8.0f, inputTop, width - 8.0f, yOffset + panelHeight - 6.0f, 5, 10, 18, 230);
	packed = packSolidQuad(batch, kCap, packed, 8.0f, inputTop, 11.0f, yOffset + panelHeight - 6.0f, 82, 205, 255, 255);

	int totalLines = static_cast<int>(history.size());
	if (totalLines > maxHistoryLines)
	{
		float trackTop = historyTop;
		float trackBottom = historyBottom;
		float trackHeight = trackBottom - trackTop;
		float scrollbarWidth = 5.0f;
		float scrollbarRightMargin = 9.0f;
		float barX1 = width - scrollbarWidth - scrollbarRightMargin;
		float barX2 = width - scrollbarRightMargin;

		packed = packSolidQuad(batch, kCap, packed, barX1, trackTop, barX2, trackBottom, 3, 8, 15, 180);

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
		packed = packVerticalGradientQuad(batch, kCap, packed, barX1, thumbTop, barX2, thumbBottom,
			119, 218, 255, 255, 57, 139, 205, 255);
	}

	unsigned char titleColor[4] = { 232, 247, 255, 255 };
	unsigned char statusColor[4] = { 152, 199, 224, 255 };
	unsigned char promptColor[4] = { 91, 216, 255, 255 };
	unsigned char inputColor[4] = { 238, 247, 255, 255 };
	std::string status = completionHint.empty()
		? "Tab complete  |  Ctrl+Arrows words  |  Ctrl+A select all"
		: completionHint;
	packed = packFontLine(batch, kCap, packed, 14.0f, yOffset + 9.0f, "Illumo Console", titleColor);
	packed = packFontLine(batch, kCap, packed, 174.0f, yOffset + 9.0f, status.c_str(), statusColor);

	// History text is drawn after chrome, but before the input row, so its
	// clipping and scroll thumb agree with the available space.
	float currentY = historyTop;
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
			packed = packFontLine(batch, kCap, packed, 14.0f, currentY, item.content.c_str(), itemColor);
			currentY += lineSpacing;
		}
	}

	// The input row has a fixed-width text viewport. This keeps a long command
	// editable: the cursor remains on-screen, selection is visible, and the
	// caret is a real rendered bar rather than an appended underscore.
	const float inputTextX = 40.0f;
	const float inputAvailableWidth = std::max(48.0f, width - inputTextX - 22.0f);
	std::size_t visibleStart = 0;
	while (visibleStart < cursorPosition)
	{
		std::string textThroughCursor = currentInput.substr(visibleStart, cursorPosition - visibleStart);
		if (measureFontText(textThroughCursor) <= inputAvailableWidth)
		{
			break;
		}
		++visibleStart;
	}
	std::size_t visibleEnd = cursorPosition;
	while (visibleEnd < currentInput.size())
	{
		std::string candidateText = currentInput.substr(visibleStart, visibleEnd + 1 - visibleStart);
		if (measureFontText(candidateText) > inputAvailableWidth)
		{
			break;
		}
		++visibleEnd;
	}
	std::string visibleInput = currentInput.substr(visibleStart, visibleEnd - visibleStart);
	float inputY = inputTop + 12.0f;
	packed = packFontLine(batch, kCap, packed, 16.0f, inputY, ">", promptColor);

	if (hasSelection())
	{
		std::size_t selectionStart = std::min(cursorPosition, selectionAnchor);
		std::size_t selectionEnd = std::max(cursorPosition, selectionAnchor);
		std::size_t highlightStart = std::max(selectionStart, visibleStart);
		std::size_t highlightEnd = std::min(selectionEnd, visibleEnd);
		if (highlightStart < highlightEnd)
		{
			std::string beforeSelection = visibleInput.substr(0, highlightStart - visibleStart);
			std::string selectedText = visibleInput.substr(highlightStart - visibleStart, highlightEnd - highlightStart);
			float highlightX0 = inputTextX + measureFontText(beforeSelection);
			float highlightX1 = highlightX0 + measureFontText(selectedText);
			packed = packSolidQuad(batch, kCap, packed, highlightX0, inputY - 3.0f, highlightX1, inputY + 16.0f, 38, 123, 181, 190);
		}
	}
	packed = packFontLine(batch, kCap, packed, inputTextX, inputY, visibleInput.c_str(), inputColor);

	long long caretMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	bool caretVisible = (caretMilliseconds % 1000) < 560;
	if (caretVisible && cursorPosition >= visibleStart && cursorPosition <= visibleEnd)
	{
		std::string textBeforeCaret = visibleInput.substr(0, cursorPosition - visibleStart);
		float caretX = inputTextX + measureFontText(textBeforeCaret);
		packed = packSolidQuad(batch, kCap, packed, caretX, inputY - 3.0f, caretX + 2.0f, inputY + 16.0f, 103, 224, 255, 255);
	}

	if (packed < 4)
	{
		return true;
	}

	const unsigned int totalQuads = packed / 4;
	// Dynamic mesh index buffer covers kUiQuadCap quads; clamp draw if somehow larger.
	unsigned int drawQuads = totalQuads;
	if (drawQuads > kUiQuadCap)
	{
		drawQuads = kUiQuadCap;
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
