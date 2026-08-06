#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include "CommandLine.h"
#include "CellMain.h"
#include "Rendering/IMesh.h"
#include "Rendering/IShaderProgram.h"
#include "Rendering/Renderer.h"
#include "Services/Logger.h"
#include "thirdparty/stb/stb_easy_font.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
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

struct BuiltInCommandHelp
{
  const char* name;
  const char* usage;
  const char* description;
};

const BuiltInCommandHelp kBuiltInCommands[] = {
  { "alias", "alias [<name> <command>]", "Create or list command aliases" },
  { "clear", "clear", "Clear console output" },
  { "close", "close", "Close the console" },
  { "console_mode",
    "console_mode [floating|mounted|toggle]",
    "Toggle or set floating vs top-mounted console mode" },
  { "echo", "echo <text>", "Print text to the console" },
  { "fade", "fade [0..1000]", "Show or set cell fade speed" },
  { "fps", "fps [on|off|toggle]", "Show or change the FPS overlay" },
  { "fullscreen",
    "fullscreen [on|off|toggle]",
    "Show or change fullscreen mode" },
  { "get", "get <variable>", "Read an environment variable" },
  { "help", "help [command]", "Show commands or detailed help" },
  { "history", "history [filter|clear]", "Search or clear command history" },
  { "quit", "quit", "Exit Illumo" },
  { "repeat", "repeat <count> <command>", "Execute command multiple times" },
  { "set",
    "set <variable> <value>",
    "Create or update an environment variable" },
  { "speed",
    "speed [0.01..100]",
    "Show or set the simulation speed multiplier" },
  { "sysinfo", "sysinfo", "Display system telemetry and statistics" },
  { "toggle", "toggle <variable>", "Toggle a boolean environment variable" },
  { "tps", "tps [1..1000]", "Show or set simulation ticks per second" },
  { "unalias", "unalias <name>", "Remove a command alias" },
  { "vars", "vars [filter]", "List environment variables, optionally filtered" }
};

std::string
lowerCopy(const std::string& text)
{
  std::string lowered = text;
  for (std::size_t i = 0; i < lowered.size(); ++i) {
    lowered[i] =
      static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[i])));
  }
  return lowered;
}

std::string
joinArguments(const std::vector<std::string>& args, std::size_t first)
{
  std::string result;
  for (std::size_t i = first; i < args.size(); ++i) {
    if (!result.empty()) {
      result += " ";
    }
    result += args[i];
  }
  return result;
}

bool
parseLongStrict(const std::string& text, long* value)
{
  if (value == nullptr || text.empty()) {
    return false;
  }
  try {
    std::size_t consumed = 0;
    long parsed = std::stol(text, &consumed);
    if (consumed != text.size()) {
      return false;
    }
    *value = parsed;
    return true;
  } catch (...) {
    return false;
  }
}

bool
parseDoubleStrict(const std::string& text, double* value)
{
  if (value == nullptr || text.empty()) {
    return false;
  }
  try {
    std::size_t consumed = 0;
    double parsed = std::stod(text, &consumed);
    if (consumed != text.size()) {
      return false;
    }
    *value = parsed;
    return true;
  } catch (...) {
    return false;
  }
}

bool
parseBoolValue(const std::string& text, bool* value)
{
  if (value == nullptr) {
    return false;
  }
  const std::string lowered = lowerCopy(text);
  if (lowered == "on" || lowered == "true" || lowered == "yes" ||
      lowered == "1") {
    *value = true;
    return true;
  }
  if (lowered == "off" || lowered == "false" || lowered == "no" ||
      lowered == "0") {
    *value = false;
    return true;
  }
  return false;
}

const BuiltInCommandHelp*
findBuiltInCommand(const std::string& name)
{
  for (const BuiltInCommandHelp& command : kBuiltInCommands) {
    if (name == command.name) {
      return &command;
    }
  }
  return nullptr;
}

std::string
findEnvironmentKey(IEnvVars* envVars, const std::string& requested)
{
  if (envVars == nullptr) {
    return "";
  }
  const std::string loweredRequested = lowerCopy(requested);
  const std::unordered_map<std::string, EnvVar>& variables = envVars->getVars();
  for (const std::pair<const std::string, EnvVar>& variable : variables) {
    if (lowerCopy(variable.first) == loweredRequested) {
      return variable.first;
    }
  }
  return "";
}

static float
measureFontText(const std::string& text)
{
  std::string mutableText = text;
  return static_cast<float>(stb_easy_font_width(mutableText.data()) * 2);
}
}

CommandLine::CommandLine(IEnvVars* vars,
                         CommandRegistry* commandRegistry,
                         IRenderWindow* win,
                         Renderer* rendererIn)
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
  , isDraggingScrollbar(false)
  , dragStartY(0.0f)
  , dragStartScrollOffset(0)
  , isFloating(false)
  , floatingX(-1.0f)
  , floatingY(-1.0f)
  , isDraggingWindow(false)
  , dragWindowOffsetX(0.0f)
  , dragWindowOffsetY(0.0f)
  , lastHeaderClickTime()
{
  isOpen = false;
  currentInput = "";
  tempInput = "";
  completionHint = "";
  history = {
    { 240, 240, 240, 255, "Illumo Developer Console" },
    { 240, 240, 240, 255, "Press ` to toggle, type 'help' for commands" }
  };

  historyIndex = 0;
  scrollOffset = 0;
  consoleInitialized = false;
  enrollGpuResources();
}

void
CommandLine::enrollGpuResources()
{
  gpuReady = false;
  consoleInitialized = false;
  if (!renderer) {
    Logger::LogError("CommandLine: no Renderer — cannot enroll GPU resources");
    return;
  }

  const unsigned int maxQuads = kUiQuadCap;
  std::vector<unsigned int> indices(static_cast<size_t>(maxQuads * 6));
  for (unsigned int i = 0; i < maxQuads; ++i) {
    indices[static_cast<size_t>(i * 6 + 0)] =
      static_cast<unsigned int>(i * 4 + 0);
    indices[static_cast<size_t>(i * 6 + 1)] =
      static_cast<unsigned int>(i * 4 + 1);
    indices[static_cast<size_t>(i * 6 + 2)] =
      static_cast<unsigned int>(i * 4 + 2);
    indices[static_cast<size_t>(i * 6 + 3)] =
      static_cast<unsigned int>(i * 4 + 2);
    indices[static_cast<size_t>(i * 6 + 4)] =
      static_cast<unsigned int>(i * 4 + 3);
    indices[static_cast<size_t>(i * 6 + 5)] =
      static_cast<unsigned int>(i * 4 + 0);
  }

  const size_t vboBytes =
    static_cast<size_t>(maxQuads) * 4 * sizeof(ConsoleVertex);
  meshHandle = renderer->allocateHandle();
  renderer->enrollDynamicMesh(vboBytes,
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

void
CommandLine::Toggle()
{
  isOpen = !isOpen;
  if (isOpen) {
    scrollOffset = 0;
    completionHint =
      "Tab: complete  |  Ctrl+Arrows: words  |  Ctrl+A: select all";
  }
}

void
CommandLine::clearCompletionHint()
{
  completionHint.clear();
}

void
CommandLine::resetCursorToEnd()
{
  cursorPosition = currentInput.size();
  selectionAnchor = cursorPosition;
}

void
CommandLine::eraseSelection()
{
  if (!hasSelection()) {
    return;
  }

  std::size_t start = std::min(cursorPosition, selectionAnchor);
  std::size_t end = std::max(cursorPosition, selectionAnchor);
  currentInput.erase(start, end - start);
  cursorPosition = start;
  selectionAnchor = start;
}

std::size_t
CommandLine::findPreviousWordBoundary() const
{
  std::size_t position = cursorPosition;
  while (position > 0 &&
         std::isspace(static_cast<unsigned char>(currentInput[position - 1]))) {
    --position;
  }
  while (position > 0 && !std::isspace(static_cast<unsigned char>(
                           currentInput[position - 1]))) {
    --position;
  }
  return position;
}

std::size_t
CommandLine::findNextWordBoundary() const
{
  std::size_t position = cursorPosition;
  while (position < currentInput.size() &&
         !std::isspace(static_cast<unsigned char>(currentInput[position]))) {
    ++position;
  }
  while (position < currentInput.size() &&
         std::isspace(static_cast<unsigned char>(currentInput[position]))) {
    ++position;
  }
  return position;
}

void
CommandLine::AddCharacter(unsigned int codepoint)
{
  std::size_t selectedCharacters =
    hasSelection() ? std::max(cursorPosition, selectionAnchor) -
                       std::min(cursorPosition, selectionAnchor)
                   : 0;
  if (currentInput.size() - selectedCharacters < MAX_CHARS_PER_LINE - 1) {
    if (codepoint >= 32 && codepoint <= 126) {
      eraseSelection();
      currentInput.insert(cursorPosition, 1, static_cast<char>(codepoint));
      ++cursorPosition;
      selectionAnchor = cursorPosition;
      clearCompletionHint();
    }
  }
}

void
CommandLine::HandleBackspace(bool byWord)
{
  if (hasSelection()) {
    eraseSelection();
    clearCompletionHint();
    return;
  }
  if (cursorPosition == 0) {
    return;
  }

  std::size_t eraseFrom =
    byWord ? findPreviousWordBoundary() : cursorPosition - 1;
  currentInput.erase(eraseFrom, cursorPosition - eraseFrom);
  cursorPosition = eraseFrom;
  selectionAnchor = cursorPosition;
  clearCompletionHint();
}

void
CommandLine::HandleDelete(bool byWord)
{
  if (hasSelection()) {
    eraseSelection();
    clearCompletionHint();
    return;
  }
  if (cursorPosition >= currentInput.size()) {
    return;
  }

  std::size_t eraseTo = byWord ? findNextWordBoundary() : cursorPosition + 1;
  currentInput.erase(cursorPosition, eraseTo - cursorPosition);
  selectionAnchor = cursorPosition;
  clearCompletionHint();
}

void
CommandLine::MoveCursorLeft(bool byWord, bool select)
{
  if (!select && hasSelection()) {
    cursorPosition = std::min(cursorPosition, selectionAnchor);
    selectionAnchor = cursorPosition;
    return;
  }
  std::size_t newPosition = byWord
                              ? findPreviousWordBoundary()
                              : (cursorPosition > 0 ? cursorPosition - 1 : 0);
  if (!select) {
    selectionAnchor = newPosition;
  }
  cursorPosition = newPosition;
}

void
CommandLine::MoveCursorRight(bool byWord, bool select)
{
  if (!select && cursorPosition == currentInput.size()) {
    std::string ghost = getGhostSuggestion();
    if (!ghost.empty()) {
      currentInput += ghost;
      cursorPosition = currentInput.size();
      selectionAnchor = cursorPosition;
      return;
    }
  }
  if (!select && hasSelection()) {
    cursorPosition = std::max(cursorPosition, selectionAnchor);
    selectionAnchor = cursorPosition;
    return;
  }
  std::size_t newPosition =
    byWord ? findNextWordBoundary()
           : std::min(cursorPosition + 1, currentInput.size());
  if (!select) {
    selectionAnchor = newPosition;
  }
  cursorPosition = newPosition;
}

void
CommandLine::MoveCursorHome(bool select)
{
  if (!select) {
    selectionAnchor = 0;
  }
  cursorPosition = 0;
}

void
CommandLine::MoveCursorEnd(bool select)
{
  if (!select) {
    selectionAnchor = currentInput.size();
  }
  cursorPosition = currentInput.size();
}

void
CommandLine::SelectAll()
{
  selectionAnchor = 0;
  cursorPosition = currentInput.size();
}

void
CommandLine::CopySelection()
{
  std::string copyText;
  if (hasSelection()) {
    std::size_t start = std::min(cursorPosition, selectionAnchor);
    std::size_t end = std::max(cursorPosition, selectionAnchor);
    copyText = currentInput.substr(start, end - start);
  } else {
    copyText = currentInput;
  }
  if (!copyText.empty() && window && window->getWindowInstance()) {
    glfwSetClipboardString(window->getWindowInstance(), copyText.c_str());
  }
}

void
CommandLine::PasteClipboard()
{
  if (!window || !window->getWindowInstance()) {
    return;
  }
  const char* clipText = glfwGetClipboardString(window->getWindowInstance());
  if (clipText == nullptr || std::strlen(clipText) == 0) {
    return;
  }

  eraseSelection();
  std::string pasteStr(clipText);
  pasteStr.erase(std::remove(pasteStr.begin(), pasteStr.end(), '\r'),
                 pasteStr.end());
  pasteStr.erase(std::remove(pasteStr.begin(), pasteStr.end(), '\n'),
                 pasteStr.end());

  currentInput.insert(cursorPosition, pasteStr);
  cursorPosition += pasteStr.size();
  selectionAnchor = cursorPosition;
  clearCompletionHint();
}

void
CommandLine::CutSelection()
{
  if (hasSelection()) {
    CopySelection();
    eraseSelection();
    clearCompletionHint();
  }
}

void
CommandLine::ClearInput()
{
  currentInput.clear();
  resetCursorToEnd();
  clearCompletionHint();
}

void
CommandLine::logNormal(const std::string& str)
{
  AppendString(255, 255, 255, 255, str);
}
void
CommandLine::logError(const std::string& str)
{
  AppendString(255, 100, 100, 255, "ERROR: " + str);
  // AppendStringLn(255, 255, 255, 255, str);
}
void
CommandLine::logWarning(const std::string& str)
{
  AppendString(255, 220, 100, 255, "WARNING: " + str);
  // AppendStringLn(255, 255, 255, 255, str);
}
void
CommandLine::logSuccess(const std::string& str)
{
  AppendString(100, 255, 100, 255, "SUCCESS: " + str);
  // AppendStringLn(255, 255, 255, 255, str);
}

void
CommandLine::logTrace(const std::string& str)
{
  AppendString(206, 0, 252, 255, "TRACE: " + str);
  // AppendStringLn(255, 255, 255, 255, str);
}

std::vector<std::string>
CommandLine::ParseCommandArgs(const std::string& text,
                              const std::string& delim) const
{
  std::vector<std::string> args;
  std::string currentArg;
  char quote = '\0';
  bool escaping = false;
  bool tokenStarted = false;

  for (char character : text) {
    if (escaping) {
      currentArg += character;
      escaping = false;
      tokenStarted = true;
      continue;
    }
    if (character == '\\') {
      escaping = true;
      tokenStarted = true;
      continue;
    }
    if (quote != '\0') {
      if (character == quote) {
        quote = '\0';
      } else {
        currentArg += character;
      }
      tokenStarted = true;
      continue;
    }
    if (character == '\'' || character == '"') {
      quote = character;
      tokenStarted = true;
      continue;
    }
    if (delim.find(character) != std::string::npos) {
      if (tokenStarted) {
        args.push_back(currentArg);
        currentArg.clear();
        tokenStarted = false;
      }
      continue;
    }
    currentArg += character;
    tokenStarted = true;
  }

  if (escaping) {
    currentArg += '\\';
  }
  if (tokenStarted) {
    args.push_back(currentArg);
  }
  return args;
}

std::vector<std::string>
CommandLine::SplitCommandChain(const std::string& text) const
{
  std::vector<std::string> commands;
  std::string currentCmd;
  char quote = '\0';
  bool escaping = false;

  for (std::size_t i = 0; i < text.size(); ++i) {
    char character = text[i];
    if (escaping) {
      currentCmd += character;
      escaping = false;
      continue;
    }
    if (character == '\\') {
      escaping = true;
      currentCmd += character;
      continue;
    }
    if (quote != '\0') {
      if (character == quote) {
        quote = '\0';
      }
      currentCmd += character;
      continue;
    }
    if (character == '\'' || character == '"') {
      quote = character;
      currentCmd += character;
      continue;
    }
    if (character == ';') {
      std::size_t firstNonSpace = currentCmd.find_first_not_of(" \t\r\n");
      if (firstNonSpace != std::string::npos) {
        commands.push_back(currentCmd);
      }
      currentCmd.clear();
      continue;
    }
    currentCmd += character;
  }

  std::size_t firstNonSpace = currentCmd.find_first_not_of(" \t\r\n");
  if (firstNonSpace != std::string::npos) {
    commands.push_back(currentCmd);
  }
  return commands;
}

void
CommandLine::SetAlias(const std::string& name, const std::string& expansion)
{
  if (!name.empty()) {
    aliases[lowerCopy(name)] = expansion;
  }
}

void
CommandLine::RemoveAlias(const std::string& name)
{
  aliases.erase(lowerCopy(name));
}

bool
CommandLine::HasAlias(const std::string& name) const
{
  return aliases.find(lowerCopy(name)) != aliases.end();
}

std::string
CommandLine::GetAlias(const std::string& name) const
{
  std::unordered_map<std::string, std::string>::const_iterator it =
    aliases.find(lowerCopy(name));
  if (it != aliases.end()) {
    return it->second;
  }
  return "";
}

std::string
CommandLine::getGhostSuggestion() const
{
  if (currentInput.empty() || cursorPosition != currentInput.size() ||
      hasSelection()) {
    return "";
  }

  std::size_t tokenStart = cursorPosition;
  while (tokenStart > 0 && !std::isspace(static_cast<unsigned char>(
                             currentInput[tokenStart - 1]))) {
    --tokenStart;
  }
  const std::string prefix =
    currentInput.substr(tokenStart, cursorPosition - tokenStart);
  if (prefix.empty()) {
    return "";
  }

  const std::string leadingText = currentInput.substr(0, tokenStart);
  std::vector<std::string> candidates = getCompletionCandidates(leadingText);
  for (const std::string& candidate : candidates) {
    if (candidate.size() > prefix.size()) {
      bool matchesPrefix = true;
      for (std::size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(candidate[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i]))) {
          matchesPrefix = false;
          break;
        }
      }
      if (matchesPrefix) {
        return candidate.substr(prefix.size());
      }
    }
  }
  return "";
}

std::string
CommandLine::getParameterHint(const std::string& inputLine) const
{
  if (inputLine.empty()) {
    return "";
  }
  std::vector<std::string> args = ParseCommandArgs(inputLine, " \t");
  if (args.empty()) {
    return "";
  }
  const std::string cmd = lowerCopy(args[0]);
  const BuiltInCommandHelp* builtIn = findBuiltInCommand(cmd);
  if (builtIn != nullptr) {
    return std::string("Usage: ") + builtIn->usage;
  }
  if (commandRegistry != nullptr && commandRegistry->HasCommand(cmd)) {
    std::string usage = commandRegistry->GetCommandUsage(cmd);
    if (!usage.empty()) {
      return std::string("Usage: ") + usage;
    }
  }
  return "";
}

std::vector<std::string>
CommandLine::getCompletionCandidates(const std::string& leadingText) const
{
  std::vector<std::string> candidates;
  std::vector<std::string> leadingArgs = ParseCommandArgs(leadingText, " \t");
  if (leadingArgs.empty()) {
    for (const BuiltInCommandHelp& command : kBuiltInCommands) {
      candidates.push_back(command.name);
    }

    if (commandRegistry != nullptr) {
      std::vector<std::string> registeredCommands =
        commandRegistry->GetCommandNames();
      candidates.insert(
        candidates.end(), registeredCommands.begin(), registeredCommands.end());
    }
    const std::unordered_map<std::string, EnvVar>& vars = envVars->getVars();
    for (const std::pair<const std::string, EnvVar>& variable : vars) {
      candidates.push_back(variable.first);
    }
    for (const std::pair<const std::string, std::string>& aliasItem : aliases) {
      candidates.push_back(aliasItem.first);
    }
  } else {
    const std::string command = lowerCopy(leadingArgs[0]);
    if (commandRegistry != nullptr && commandRegistry->HasCommand(command)) {
      candidates = commandRegistry->GetCommandCompletions(command);
    } else if (command == "get" || command == "set" || command == "toggle" ||
               command == "vars") {
      const std::unordered_map<std::string, EnvVar>& vars = envVars->getVars();
      for (const std::pair<const std::string, EnvVar>& variable : vars) {
        candidates.push_back(variable.first);
      }
    } else if (command == "fps" || command == "fullscreen") {
      candidates = { "off", "on", "toggle" };
    }
  }

  std::sort(candidates.begin(), candidates.end());
  candidates.erase(std::unique(candidates.begin(), candidates.end()),
                   candidates.end());
  return candidates;
}

void
CommandLine::Complete()
{
  std::size_t tokenStart = cursorPosition;
  while (tokenStart > 0 && !std::isspace(static_cast<unsigned char>(
                             currentInput[tokenStart - 1]))) {
    --tokenStart;
  }
  const std::string prefix =
    currentInput.substr(tokenStart, cursorPosition - tokenStart);
  const std::string leadingText = currentInput.substr(0, tokenStart);
  std::vector<std::string> candidates = getCompletionCandidates(leadingText);
  std::vector<std::string> matches;
  for (const std::string& candidate : candidates) {
    if (candidate.size() < prefix.size()) {
      continue;
    }

    bool matchesPrefix = true;
    for (std::size_t i = 0; i < prefix.size(); ++i) {
      if (std::tolower(static_cast<unsigned char>(candidate[i])) !=
          std::tolower(static_cast<unsigned char>(prefix[i]))) {
        matchesPrefix = false;
        break;
      }
    }
    if (matchesPrefix) {
      matches.push_back(candidate);
    }
  }

  if (matches.empty()) {
    completionHint = "No completion matches '" + prefix + "'";
    return;
  }

  std::string replacement = matches[0];
  for (std::size_t i = 1; i < matches.size(); ++i) {
    std::size_t commonLength = 0;
    while (commonLength < replacement.size() &&
           commonLength < matches[i].size() &&
           replacement[commonLength] == matches[i][commonLength]) {
      ++commonLength;
    }
    replacement.resize(commonLength);
  }

  if (replacement.size() > prefix.size() || matches.size() == 1) {
    currentInput.replace(tokenStart, cursorPosition - tokenStart, replacement);
    cursorPosition = tokenStart + replacement.size();
    selectionAnchor = cursorPosition;
  }

  if (matches.size() == 1) {
    if (tokenStart == 0 && cursorPosition == currentInput.size()) {
      currentInput += " ";
      ++cursorPosition;
      selectionAnchor = cursorPosition;
    }
    completionHint = "Completed: " + matches[0];
    return;
  }

  completionHint = "Matches: ";
  const std::size_t visibleMatches =
    std::min(matches.size(), static_cast<std::size_t>(4));
  for (std::size_t i = 0; i < visibleMatches; ++i) {
    if (i > 0) {
      completionHint += "  ";
    }
    completionHint += matches[i];
  }
  if (matches.size() > visibleMatches) {
    completionHint += "  ...";
  }
}
void
CommandLine::ExecuteCommand()
{
  if (currentInput.empty()) {
    return;
  }

  AddToHistory(currentInput);

  std::vector<std::string> subCommands = SplitCommandChain(currentInput);
  ClearInput();
  scrollOffset = 0;

  auto startTime = std::chrono::high_resolution_clock::now();

  for (const std::string& singleCmd : subCommands) {
    ExecuteSingleCommand(singleCmd, 0);
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  double ms =
    std::chrono::duration<double, std::milli>(endTime - startTime).count();
  if (!history.empty() &&
      history.back().content != "Illumo Developer Console") {
    std::ostringstream oss;
    oss << std::fixed;
    oss.precision(2);
    oss << "Executed in " << ms << "ms";
    logTrace(oss.str());
  }
}

void
CommandLine::ExecuteSingleCommand(const std::string& singleCmd,
                                  int expansionDepth)
{
  if (expansionDepth > 8) {
    logError("Alias expansion depth limit exceeded");
    return;
  }

  std::vector<std::string> commandParts = ParseCommandArgs(singleCmd, " \t");
  if (commandParts.empty()) {
    return;
  }

  if (expansionDepth == 0) {
    AppendString(100, 200, 255, 255, "> " + singleCmd);
  }

  const std::string rawCommand = commandParts[0];
  const std::string cmd = lowerCopy(rawCommand);
  std::vector<std::string> args(commandParts.begin() + 1, commandParts.end());

  if (HasAlias(cmd)) {
    std::string expanded = GetAlias(cmd);
    if (!args.empty()) {
      expanded += " " + joinArguments(args, 0);
    }
    std::vector<std::string> chained = SplitCommandChain(expanded);
    for (const std::string& subCmd : chained) {
      ExecuteSingleCommand(subCmd, expansionDepth + 1);
    }
    return;
  }

  if (cmd == "alias") {
    if (args.empty()) {
      if (aliases.empty()) {
        logNormal("No aliases defined.");
      } else {
        logNormal("Defined aliases:");
        std::vector<std::pair<std::string, std::string>> sortedAliases(
          aliases.begin(), aliases.end());
        std::sort(sortedAliases.begin(), sortedAliases.end());
        for (const std::pair<std::string, std::string>& aliasItem :
             sortedAliases) {
          logNormal("  " + aliasItem.first + " = \"" + aliasItem.second + "\"");
        }
      }
    } else if (args.size() == 1) {
      if (HasAlias(args[0])) {
        logNormal(args[0] + " = \"" + GetAlias(args[0]) + "\"");
      } else {
        logError("Unknown alias: " + args[0]);
      }
    } else {
      const std::string name = args[0];
      const std::string expansion = joinArguments(args, 1);
      SetAlias(name, expansion);
      logSuccess("Alias '" + name + "' set to: " + expansion);
    }
  } else if (cmd == "unalias") {
    if (args.size() != 1) {
      logNormal("Usage: unalias <name>");
    } else if (HasAlias(args[0])) {
      RemoveAlias(args[0]);
      logSuccess("Alias '" + args[0] + "' removed");
    } else {
      logError("Unknown alias: " + args[0]);
    }
  } else if (cmd == "repeat") {
    if (args.size() < 2) {
      logNormal("Usage: repeat <count> <command>");
    } else {
      long count = 0;
      if (!parseLongStrict(args[0], &count) || count < 1 || count > 1000) {
        logError("repeat count must be an integer from 1 to 1000");
      } else {
        const std::string repeatCmd = joinArguments(args, 1);
        for (long i = 0; i < count; ++i) {
          ExecuteSingleCommand(repeatCmd, expansionDepth + 1);
        }
      }
    }
  } else if (cmd == "history") {
    if (args.size() == 1 && lowerCopy(args[0]) == "clear") {
      commandHistory.clear();
      historyIndex = 0;
      logSuccess("Command history cleared");
    } else {
      const std::string filter = args.empty() ? "" : lowerCopy(args[0]);
      logNormal("Command history:");
      int count = 0;
      for (std::size_t i = 0; i < commandHistory.size(); ++i) {
        if (filter.empty() ||
            lowerCopy(commandHistory[i]).find(filter) != std::string::npos) {
          logNormal("  " + std::to_string(i + 1) + ": " + commandHistory[i]);
          ++count;
        }
      }
      if (count == 0) {
        logWarning("No history entries match '" + filter + "'");
      }
    }
  } else if (cmd == "sysinfo" || cmd == "status") {
    logNormal("=== Illumo System Telemetry ===");
    logNormal("Registered commands: " +
              std::to_string(commandRegistry
                               ? commandRegistry->GetCommandNames().size()
                               : 0));
    logNormal("Env variables:       " +
              std::to_string(envVars ? envVars->getVars().size() : 0));
    logNormal("Defined aliases:     " + std::to_string(aliases.size()));
    std::array<int, 2> dims =
      window ? window->getWindowDimensions() : std::array<int, 2>{ 0, 0 };
    logNormal("Window resolution:   " + std::to_string(dims[0]) + "x" +
              std::to_string(dims[1]));
    logNormal("FPS overlay:         " +
              std::string(envVars && envVars->getVar("showFPS").valueAsBool
                            ? "on"
                            : "off"));
    logNormal("TPS setting:         " +
              (envVars ? envVars->getVar("tps").value : "N/A"));
    logNormal("Fade speed:          " +
              (envVars ? envVars->getVar("cellFadeSpeed").value : "N/A"));
  } else if (cmd == "help") {
    if (args.empty()) {
      logNormal("Built-in commands:");
      for (const BuiltInCommandHelp& command : kBuiltInCommands) {
        logNormal("  " + std::string(command.usage) + " - " +
                  command.description);
      }

      if (commandRegistry != nullptr) {
        std::vector<std::string> registeredCommands =
          commandRegistry->GetCommandNames();
        if (!registeredCommands.empty()) {
          logNormal("Simulation commands:");
        }
        for (const std::string& commandName : registeredCommands) {
          std::string usage = commandRegistry->GetCommandUsage(commandName);
          std::string description =
            commandRegistry->GetCommandDescription(commandName);
          if (usage.empty()) {
            usage = commandName;
          }
          logNormal("  " + usage +
                    (description.empty() ? "" : " - " + description));
        }
      }
      logNormal("Use 'help <command>' for one command.");
    } else {
      const std::string requested = lowerCopy(args[0]);
      const BuiltInCommandHelp* builtIn = findBuiltInCommand(requested);
      if (builtIn != nullptr) {
        logNormal(std::string(builtIn->usage) + " - " + builtIn->description);
      } else if (commandRegistry != nullptr &&
                 commandRegistry->HasCommand(requested)) {
        std::string usage = commandRegistry->GetCommandUsage(requested);
        std::string description =
          commandRegistry->GetCommandDescription(requested);
        logNormal((usage.empty() ? requested : usage) +
                  (description.empty() ? "" : " - " + description));
      } else {
        logError("No help available for '" + args[0] + "'");
      }
    }
  } else if (cmd == "clear") {
    history.clear();
    AppendString(240, 240, 240, 255, "Illumo Developer Console");
  } else if (cmd == "echo") {
    logNormal(joinArguments(args, 0));
  } else if (cmd == "get") {
    if (args.size() != 1) {
      logNormal("Usage: get <variable>");
    } else {
      const std::string key = findEnvironmentKey(envVars, args[0]);
      if (key.empty()) {
        logError("Unknown variable: " + args[0]);
      } else {
        logNormal(key + " = " + envVars->getVar(key).value);
      }
    }
  } else if (cmd == "set") {
    if (args.size() < 2) {
      logNormal("Usage: set <variable> <value>");
    } else {
      std::string key = findEnvironmentKey(envVars, args[0]);
      if (key.empty()) {
        key = args[0];
      }
      const std::string value = joinArguments(args, 1);
      envVars->setVar(key, value);
      logSuccess(key + " = " + value);
    }
  } else if (cmd == "toggle") {
    if (args.size() != 1) {
      logNormal("Usage: toggle <variable>");
    } else {
      const std::string key = findEnvironmentKey(envVars, args[0]);
      if (key.empty()) {
        logError("Unknown variable: " + args[0]);
      } else {
        const bool value = !envVars->getVar(key).valueAsBool;
        envVars->setVar(key, value);
        logSuccess(key + " = " + (value ? "true" : "false"));
      }
    }
  } else if (cmd == "vars") {
    const std::string filter = args.empty() ? "" : lowerCopy(args[0]);
    std::vector<std::string> variableLines;
    const std::unordered_map<std::string, EnvVar>& variables =
      envVars->getVars();
    for (const std::pair<const std::string, EnvVar>& variable : variables) {
      if (filter.empty() ||
          lowerCopy(variable.first).find(filter) != std::string::npos) {
        variableLines.push_back(variable.first + " = " + variable.second.value);
      }
    }
    std::sort(variableLines.begin(), variableLines.end());
    if (variableLines.empty()) {
      logWarning("No variables match '" +
                 (args.empty() ? std::string("") : args[0]) + "'");
    }
    for (const std::string& line : variableLines) {
      logNormal(line);
    }
  } else if (cmd == "tps") {
    if (args.empty()) {
      logNormal("tps = " + envVars->getVar("tps").value);
    } else {
      long value = 0;
      if (args.size() != 1 || !parseLongStrict(args[0], &value) || value < 1 ||
          value > 1000) {
        logError("tps must be an integer from 1 to 1000");
      } else {
        envVars->setVar("tps", value);
        logSuccess("tps = " + std::to_string(value));
      }
    }
  } else if (cmd == "speed" || cmd == "speedfactor") {
    if (args.empty()) {
      logNormal("speedFactor = " + envVars->getVar("speedFactor").value);
    } else {
      double value = 0.0;
      if (args.size() != 1 || !parseDoubleStrict(args[0], &value) ||
          value < 0.01 || value > 100.0) {
        logError("speed must be a number from 0.01 to 100");
      } else {
        envVars->setVar("speedFactor", args[0]);
        logSuccess("speedFactor = " + args[0]);
      }
    }
  } else if (cmd == "fade" || cmd == "cellfadespeed") {
    if (args.empty()) {
      logNormal("cellFadeSpeed = " + envVars->getVar("cellFadeSpeed").value);
    } else {
      double value = 0.0;
      if (args.size() != 1 || !parseDoubleStrict(args[0], &value) ||
          value < 0.0 || value > 1000.0) {
        logError("fade must be a number from 0 to 1000");
      } else {
        envVars->setVar("cellFadeSpeed", args[0]);
        logSuccess("cellFadeSpeed = " + args[0]);
      }
    }
  } else if (cmd == "fps") {
    const bool currentValue = envVars->getVar("showFPS").valueAsBool;
    if (args.empty()) {
      logNormal(std::string("FPS overlay: ") + (currentValue ? "on" : "off"));
    } else {
      bool requestedValue = false;
      bool valid = false;
      if (args.size() == 1 && lowerCopy(args[0]) == "toggle") {
        requestedValue = !currentValue;
        valid = true;
      } else if (args.size() == 1) {
        valid = parseBoolValue(args[0], &requestedValue);
      }
      if (!valid) {
        logError("Usage: fps [on|off|toggle]");
      } else {
        envVars->setVar("showFPS", requestedValue);
        logSuccess(std::string("FPS overlay: ") +
                   (requestedValue ? "on" : "off"));
      }
    }
  } else if (cmd == "fullscreen") {
    const bool currentValue = envVars->getVar("fullscreen").valueAsBool;
    bool requestedValue = !currentValue;
    bool valid = args.empty();
    if (args.size() == 1 && lowerCopy(args[0]) == "toggle") {
      valid = true;
    } else if (args.size() == 1) {
      valid = parseBoolValue(args[0], &requestedValue);
    }
    if (!valid) {
      logError("Usage: fullscreen [on|off|toggle]");
    } else {
      if (requestedValue != currentValue) {
        window->toggleFullscreen();
      }
      envVars->setVar("fullscreen", requestedValue);
      logSuccess(std::string("Fullscreen: ") + (requestedValue ? "on" : "off"));
    }
  } else if (cmd == "console_mode") {
    if (args.empty()) {
      logNormal(std::string("Console mode: ") +
                (isFloating ? "floating" : "mounted"));
    } else {
      std::string arg = lowerCopy(args[0]);
      if (arg == "floating" || arg == "float") {
        setFloatingMode(true);
      } else if (arg == "mounted" || arg == "top" || arg == "docked") {
        setFloatingMode(false);
      } else if (arg == "toggle") {
        ToggleFloatingMode();
      } else {
        logError("Usage: console_mode [floating|mounted|toggle]");
      }
    }
  } else if (cmd == "close") {
    isOpen = false;
  } else if (cmd == "quit") {
    window->requestClose();
  } else if (cmd == "vid_restart") {
    logWarning("vid_restart is unavailable: safely rebuilding the OpenGL "
               "context requires resource re-enrollment");
  } else {
    if (commandRegistry != nullptr && commandRegistry->HasCommand(cmd)) {
      commandRegistry->QueueCommand(cmd, args);
    } else {
      const std::string key = findEnvironmentKey(envVars, rawCommand);
      if (!key.empty()) {
        if (args.empty()) {
          logNormal(key + " = " + envVars->getVar(key).value);
        } else if (args.size() == 1) {
          envVars->setVar(key, args[0]);
          logSuccess(key + " = " + args[0]);
        } else {
          logError("Variable assignment accepts one value; use set for text "
                   "with spaces");
        }
      } else {
        logError("Unknown command or variable: " + rawCommand);
      }
    }
  }
}

void
CommandLine::AddToHistory(std::string command)
{
  commandHistory.push_back(command);
  // imitate stack behavior by setting historyIndex to top of stack and popping
  // the first entry when full.
  if (commandHistory.size() > MAX_CMD_HISTORY) {
    commandHistory.erase(commandHistory.begin());
  }
  historyIndex = (int)commandHistory.size();
  tempInput = "";
  resetCursorToEnd();
}

void
CommandLine::HistoryDown()
{
  if (commandHistory.empty())
    return;
  if (historyIndex < (int)commandHistory.size()) {
    historyIndex++;
    if (historyIndex == (int)commandHistory.size()) {
      currentInput = tempInput;
    } else {
      currentInput = commandHistory[historyIndex];
    }
    resetCursorToEnd();
  }
}

void
CommandLine::HistoryUp()
{
  if (commandHistory.empty())
    return;
  if (historyIndex > 0) {
    if (historyIndex == (int)commandHistory.size()) {
      tempInput = currentInput;
    }
    historyIndex--;
    currentInput = commandHistory[historyIndex];
    resetCursorToEnd();
  }
}

void
CommandLine::ScrollUp()
{
  std::array<int, 2> windowDimensions = window->getWindowDimensions();
  int winHeight = windowDimensions[1];
  float panelHeight = winHeight * 0.52f;
  if (panelHeight < 240.0f)
    panelHeight = 240.0f;
  float lineSpacing = 24.0f;
  int maxHistoryLines = (int)((panelHeight - 90.0f) / lineSpacing);
  if (maxHistoryLines < 1)
    maxHistoryLines = 1;

  int maxScroll = (int)history.size() - maxHistoryLines;
  if (maxScroll < 0)
    maxScroll = 0;

  if (scrollOffset < maxScroll) {
    scrollOffset++;
  }
}

void
CommandLine::ScrollDown()
{
  if (scrollOffset > 0) {
    scrollOffset--;
  }
}

void
CommandLine::ToggleFloatingMode()
{
  setFloatingMode(!isFloating);
}

void
CommandLine::setFloatingMode(bool floating)
{
  isFloating = floating;
  if (isFloating) {
    std::array<int, 2> dims =
      window ? window->getWindowDimensions() : std::array<int, 2>{ 1280, 720 };
    float w = static_cast<float>(dims[0]);
    float h = static_cast<float>(dims[1]);
    float panelH = h * 0.52f;
    if (panelH < 240.0f) {
      panelH = 240.0f;
    }
    if (panelH > h - 20.0f) {
      panelH = h - 20.0f;
    }
    float margin = std::clamp(w * 0.08f, 20.0f, 100.0f);
    floatingX = margin;
    floatingY = 20.0f;
  }
  logSuccess(isFloating ? "Console mode set to FLOATING"
                        : "Console mode set to MOUNTED");
}

void
CommandLine::HandleScroll(double yOffset)
{
  if (!isOpen || history.empty()) {
    return;
  }
  std::array<int, 2> windowDimensions =
    window ? window->getWindowDimensions() : std::array<int, 2>{ 1280, 720 };
  float height = static_cast<float>(windowDimensions[1]);
  float panelHeight = height * 0.52f;
  if (panelHeight < 240.0f) {
    panelHeight = 240.0f;
  }
  if (panelHeight > height - 20.0f) {
    panelHeight = height - 20.0f;
  }
  const float headerHeight = 34.0f;
  const float inputRowHeight = 40.0f;
  const float historyTop = 8.0f + headerHeight;
  const float inputTop = panelHeight - inputRowHeight;
  const float historyBottom = inputTop - 8.0f;
  const float lineSpacing = 24.0f;
  int maxHistoryLines =
    static_cast<int>((historyBottom - historyTop) / lineSpacing);
  if (maxHistoryLines < 1) {
    maxHistoryLines = 1;
  }
  int maxScroll = static_cast<int>(history.size()) - maxHistoryLines;
  if (maxScroll < 0) {
    maxScroll = 0;
  }

  int delta = static_cast<int>(yOffset > 0.0 ? 3 : (yOffset < 0.0 ? -3 : 0));
  scrollOffset += delta;
  if (scrollOffset < 0) {
    scrollOffset = 0;
  }
  if (scrollOffset > maxScroll) {
    scrollOffset = maxScroll;
  }
}

void
CommandLine::HandleMousePress(double mouseX, double mouseY, bool isDrag)
{
  if (!isOpen) {
    return;
  }

  std::array<int, 2> windowDimensions =
    window ? window->getWindowDimensions() : std::array<int, 2>{ 1280, 720 };
  float width = static_cast<float>(windowDimensions[0]);
  float height = static_cast<float>(windowDimensions[1]);
  float panelHeight = height * 0.52f;
  if (panelHeight < 240.0f) {
    panelHeight = 240.0f;
  }
  if (panelHeight > height - 20.0f) {
    panelHeight = height - 20.0f;
  }
  float effectiveAnim =
    (animationProgress > 0.0f) ? animationProgress : (isOpen ? 1.0f : 0.0f);
  float yOffset = -panelHeight * (1.0f - effectiveAnim);
  float defaultMarginX = std::clamp(width * 0.08f, 20.0f, 100.0f);
  float floatW = std::max(280.0f, width - defaultMarginX * 2.0f);
  if (isFloating && floatingX < 0.0f) {
    floatingX = defaultMarginX;
    floatingY = 20.0f;
  }
  float panelX0 =
    isFloating ? std::clamp(floatingX, 0.0f, width - floatW) : 0.0f;
  float panelX1 = isFloating ? (panelX0 + floatW) : width;
  float panelY0 =
    isFloating ? std::clamp(floatingY, 0.0f, height - panelHeight) : yOffset;
  float panelY1 = panelY0 + panelHeight;

  const float headerHeight = 34.0f;
  const float inputRowHeight = 40.0f;
  const float historyTop = panelY0 + headerHeight + 8.0f;
  const float inputTop = panelY1 - inputRowHeight;
  const float historyBottom = inputTop - 8.0f;

  if (mouseX < panelX0 || mouseX > panelX1 || mouseY < panelY0 ||
      mouseY > panelY1) {
    return;
  }

  if (mouseY >= panelY0 && mouseY <= panelY0 + headerHeight) {
    if (!isDrag) {
      auto now = std::chrono::high_resolution_clock::now();
      auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - lastHeaderClickTime)
                         .count();
      if (elapsedMs > 0 && elapsedMs < 350) {
        ToggleFloatingMode();
        lastHeaderClickTime = std::chrono::high_resolution_clock::time_point{};
        return;
      }
      lastHeaderClickTime = now;
      if (isFloating) {
        isDraggingWindow = true;
        dragWindowOffsetX = static_cast<float>(mouseX) - panelX0;
        dragWindowOffsetY = static_cast<float>(mouseY) - panelY0;
        return;
      }
    }
  }

  int totalLines = static_cast<int>(history.size());
  float lineSpacing = 24.0f;
  int maxHistoryLines =
    static_cast<int>((historyBottom - historyTop) / lineSpacing);
  if (maxHistoryLines < 1) {
    maxHistoryLines = 1;
  }
  int maxScroll =
    (totalLines > maxHistoryLines) ? (totalLines - maxHistoryLines) : 0;

  if (totalLines > maxHistoryLines) {
    float scrollbarWidth = 5.0f;
    float scrollbarRightMargin = 9.0f;
    float barX1 = panelX1 - scrollbarWidth - scrollbarRightMargin;
    float barX2 = panelX1 - scrollbarRightMargin;
    if (mouseX >= barX1 - 12.0f && mouseX <= barX2 + 12.0f &&
        mouseY >= historyTop && mouseY <= historyBottom) {
      if (!isDrag) {
        isDraggingScrollbar = true;
        dragStartY = static_cast<float>(mouseY);
        dragStartScrollOffset = scrollOffset;
      }
      float trackHeight = historyBottom - historyTop;
      float clickRatio =
        1.0f - (static_cast<float>(mouseY) - historyTop) / trackHeight;
      if (clickRatio < 0.0f) {
        clickRatio = 0.0f;
      }
      if (clickRatio > 1.0f) {
        clickRatio = 1.0f;
      }
      scrollOffset = static_cast<int>(clickRatio * maxScroll);
      if (scrollOffset < 0) {
        scrollOffset = 0;
      }
      if (scrollOffset > maxScroll) {
        scrollOffset = maxScroll;
      }
      return;
    }
  }

  if (mouseY >= inputTop && mouseY <= panelY1) {
    const float inputTextX = panelX0 + 40.0f;
    const float inputAvailableWidth =
      std::max(48.0f, (panelX1 - panelX0) - 62.0f);
    std::size_t visibleStart = 0;
    while (visibleStart < cursorPosition) {
      std::string textThroughCursor =
        currentInput.substr(visibleStart, cursorPosition - visibleStart);
      if (measureFontText(textThroughCursor) <= inputAvailableWidth) {
        break;
      }
      ++visibleStart;
    }
    std::size_t visibleEnd = cursorPosition;
    while (visibleEnd < currentInput.size()) {
      std::string candidateText =
        currentInput.substr(visibleStart, visibleEnd + 1 - visibleStart);
      if (measureFontText(candidateText) > inputAvailableWidth) {
        break;
      }
      ++visibleEnd;
    }
    std::string visibleInput =
      currentInput.substr(visibleStart, visibleEnd - visibleStart);

    float relX = static_cast<float>(mouseX);
    std::size_t bestIdx = 0;
    float minDiff = 1e9f;
    for (std::size_t i = 0; i <= visibleInput.size(); ++i) {
      float charX = inputTextX + measureFontText(visibleInput.substr(0, i));
      float diff = std::abs(charX - relX);
      if (diff < minDiff) {
        minDiff = diff;
        bestIdx = i;
      }
    }
    std::size_t targetPos = visibleStart + bestIdx;
    if (targetPos > currentInput.size()) {
      targetPos = currentInput.size();
    }

    cursorPosition = targetPos;
    if (!isDrag) {
      selectionAnchor = cursorPosition;
    }
  }
}

void
CommandLine::HandleMouseDrag(double mouseX, double mouseY)
{
  if (!isOpen) {
    return;
  }
  if (isDraggingWindow && isFloating) {
    std::array<int, 2> windowDimensions =
      window ? window->getWindowDimensions() : std::array<int, 2>{ 1280, 720 };
    float width = static_cast<float>(windowDimensions[0]);
    float height = static_cast<float>(windowDimensions[1]);
    float panelHeight = height * 0.52f;
    if (panelHeight < 240.0f) {
      panelHeight = 240.0f;
    }
    if (panelHeight > height - 20.0f) {
      panelHeight = height - 20.0f;
    }
    float defaultMarginX = std::clamp(width * 0.08f, 20.0f, 100.0f);
    float floatW = std::max(280.0f, width - defaultMarginX * 2.0f);

    floatingX = std::clamp(
      static_cast<float>(mouseX) - dragWindowOffsetX, 0.0f, width - floatW);
    floatingY = std::clamp(static_cast<float>(mouseY) - dragWindowOffsetY,
                           0.0f,
                           height - panelHeight);
    return;
  }
  if (isDraggingScrollbar) {
    std::array<int, 2> windowDimensions =
      window ? window->getWindowDimensions() : std::array<int, 2>{ 1280, 720 };
    float height = static_cast<float>(windowDimensions[1]);
    float panelHeight = height * 0.52f;
    if (panelHeight < 240.0f) {
      panelHeight = 240.0f;
    }
    if (panelHeight > height - 20.0f) {
      panelHeight = height - 20.0f;
    }
    float effectiveAnim =
      (animationProgress > 0.0f) ? animationProgress : (isOpen ? 1.0f : 0.0f);
    float yOffset = -panelHeight * (1.0f - effectiveAnim);
    const float headerHeight = 34.0f;
    const float inputRowHeight = 40.0f;
    const float historyTop = yOffset + headerHeight + 8.0f;
    const float inputTop = yOffset + panelHeight - inputRowHeight;
    const float historyBottom = inputTop - 8.0f;

    int totalLines = static_cast<int>(history.size());
    float lineSpacing = 24.0f;
    int maxHistoryLines =
      static_cast<int>((historyBottom - historyTop) / lineSpacing);
    if (maxHistoryLines < 1) {
      maxHistoryLines = 1;
    }
    int maxScroll =
      (totalLines > maxHistoryLines) ? (totalLines - maxHistoryLines) : 0;

    float trackHeight = historyBottom - historyTop;
    if (trackHeight > 0.0f && maxScroll > 0) {
      float deltaY = static_cast<float>(mouseY) - dragStartY;
      float deltaScrollRatio = -deltaY / trackHeight;
      int newScroll =
        dragStartScrollOffset + static_cast<int>(deltaScrollRatio * maxScroll);
      if (newScroll < 0) {
        newScroll = 0;
      }
      if (newScroll > maxScroll) {
        newScroll = maxScroll;
      }
      scrollOffset = newScroll;
    }
    return;
  }

  HandleMousePress(mouseX, mouseY, true);
}

void
CommandLine::HandleMouseRelease()
{
  isDraggingScrollbar = false;
  isDraggingWindow = false;
}

void
CommandLine::AppendString(unsigned char r,
                          unsigned char g,
                          unsigned char b,
                          unsigned char a,
                          std::string str)
{
  history.push_back({ r, g, b, a, str });
  if (history.size() > MAX_CMD_HISTORY) {
    history.erase(history.begin());
  }
}
void
CommandLine::AppendStringLn(unsigned char r,
                            unsigned char g,
                            unsigned char b,
                            unsigned char a,
                            std::string str)
{
  history.push_back({ r, g, b, a, str + "\n" });
  if (history.size() > MAX_CMD_HISTORY) {
    history.erase(history.begin());
  }
}

void
CommandLine::DrawImpl()
{
  // Migrated to tokens.
}

namespace {

struct UiVert
{
  float x, y, z;
  uint8_t color[4];
};

static unsigned int
packSolidQuad(UiVert* dest,
              unsigned int destCap,
              unsigned int writeAt,
              float x0,
              float y0,
              float x1,
              float y1,
              unsigned char r,
              unsigned char g,
              unsigned char b,
              unsigned char a)
{
  if (writeAt + 4 > destCap) {
    return writeAt;
  }
  dest[writeAt + 0] = { x0, y0, 0.0f, { r, g, b, a } };
  dest[writeAt + 1] = { x1, y0, 0.0f, { r, g, b, a } };
  dest[writeAt + 2] = { x1, y1, 0.0f, { r, g, b, a } };
  dest[writeAt + 3] = { x0, y1, 0.0f, { r, g, b, a } };
  return writeAt + 4;
}

static unsigned int
packVerticalGradientQuad(UiVert* dest,
                         unsigned int destCap,
                         unsigned int writeAt,
                         float x0,
                         float y0,
                         float x1,
                         float y1,
                         unsigned char topR,
                         unsigned char topG,
                         unsigned char topB,
                         unsigned char topA,
                         unsigned char bottomR,
                         unsigned char bottomG,
                         unsigned char bottomB,
                         unsigned char bottomA)
{
  if (writeAt + 4 > destCap) {
    return writeAt;
  }
  dest[writeAt + 0] = { x0, y0, 0.0f, { topR, topG, topB, topA } };
  dest[writeAt + 1] = { x1, y0, 0.0f, { topR, topG, topB, topA } };
  dest[writeAt + 2] = { x1, y1, 0.0f, { bottomR, bottomG, bottomB, bottomA } };
  dest[writeAt + 3] = { x0, y1, 0.0f, { bottomR, bottomG, bottomB, bottomA } };
  return writeAt + 4;
}

// stb_easy_font half-scale coords; bake ×2 so shader uses u_scale=(1,1).
static unsigned int
packFontLine(UiVert* dest,
             unsigned int destCap,
             unsigned int writeAt,
             float x,
             float y,
             const char* text,
             unsigned char color[4])
{
  if (writeAt >= destCap || text == nullptr) {
    return writeAt;
  }
  const unsigned int remaining = destCap - writeAt;
  int numQuads =
    stb_easy_font_print(x * 0.5f,
                        y * 0.5f,
                        const_cast<char*>(text),
                        color,
                        &dest[writeAt],
                        static_cast<int>(remaining * sizeof(UiVert)));
  if (numQuads <= 0) {
    return writeAt;
  }
  if (numQuads > static_cast<int>(remaining / 4)) {
    numQuads = static_cast<int>(remaining / 4);
  }
  const unsigned int vCount = static_cast<unsigned int>(numQuads * 4);
  for (unsigned int i = 0; i < vCount; ++i) {
    dest[writeAt + i].x *= 2.0f;
    dest[writeAt + i].y *= 2.0f;
  }
  return writeAt + vCount;
}

} // namespace

bool
CommandLine::AppendCommands(Renderer* r)
{
  if (!isVisible()) {
    return true;
  }
  if (!gpuReady || !r) {
    return false;
  }

  // Animation
  std::chrono::high_resolution_clock::time_point now =
    std::chrono::high_resolution_clock::now();
  float deltaTime = std::chrono::duration<float>(now - lastAnimTime).count();
  lastAnimTime = now;
  if (deltaTime > 0.1f) {
    deltaTime = 0.1f;
  }
  const float animationSpeed = 8.5f;
  if (isOpen) {
    animationProgress =
      Math::lerp(animationProgress, 1.0f, animationSpeed * deltaTime);
  } else {
    animationProgress =
      Math::lerp(animationProgress, 0.0f, animationSpeed * deltaTime);
  }
  if (animationProgress <= 0.0f) {
    return true;
  }

  std::array<int, 2> windowDimensions = window->getWindowDimensions();
  int winWidth = windowDimensions[0];
  int winHeight = windowDimensions[1];
  float width = static_cast<float>(winWidth);
  float height = static_cast<float>(winHeight);

  float panelHeight = height * 0.52f;
  if (panelHeight < 240.0f) {
    panelHeight = 240.0f;
  }
  if (panelHeight > height - 20.0f) {
    panelHeight = height - 20.0f;
  }
  float yOffset = -panelHeight * (1.0f - animationProgress);
  float defaultMarginX = std::clamp(width * 0.08f, 20.0f, 100.0f);
  float floatW = std::max(280.0f, width - defaultMarginX * 2.0f);
  if (isFloating && floatingX < 0.0f) {
    floatingX = defaultMarginX;
    floatingY = 20.0f;
  }
  float panelX0 =
    isFloating ? std::clamp(floatingX, 0.0f, width - floatW) : 0.0f;
  float panelX1 = isFloating ? (panelX0 + floatW) : width;
  float panelY0 =
    isFloating ? std::clamp(floatingY, 0.0f, height - panelHeight) : yOffset;
  float panelY1 = panelY0 + panelHeight;

  const float headerHeight = 34.0f;
  const float inputRowHeight = 40.0f;
  const float historyTop = panelY0 + headerHeight + 8.0f;
  const float inputTop = panelY1 - inputRowHeight;
  const float historyBottom = inputTop - 8.0f;
  float lineSpacing = 24.0f;
  int maxHistoryLines =
    static_cast<int>((historyBottom - historyTop) / lineSpacing);
  if (maxHistoryLines < 1) {
    maxHistoryLines = 1;
  }

  // Pack entire console (chrome + text) into one vertex buffer for a single
  // UpdateBuffer + DrawIndexed (P2). Index buffer is sequential quads.
  const unsigned int kCap = kUiVertCap;
  unsigned int packed = 0;
  UiVert* batch = reinterpret_cast<UiVert*>(uiVerts);

  long long caretMilliseconds =
    std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch())
      .count();
  float pulse =
    (std::sin(static_cast<float>(caretMilliseconds) * 0.005f) + 1.0f) * 0.5f;

  // Futuristic layered console chrome: dark space glass, pulsing cyan accent
  // trim, glowing title bar, and crisp input hierarchy.
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 4.0f,
                         panelY0 + 6.0f,
                         panelX1 + 4.0f,
                         panelY1 + 6.0f,
                         0,
                         0,
                         0,
                         140);
  packed = packVerticalGradientQuad(batch,
                                    kCap,
                                    packed,
                                    panelX0,
                                    panelY0,
                                    panelX1,
                                    panelY1,
                                    12,
                                    22,
                                    38,
                                    248,
                                    6,
                                    11,
                                    20,
                                    242);
  packed = packVerticalGradientQuad(batch,
                                    kCap,
                                    packed,
                                    panelX0,
                                    panelY0,
                                    panelX1,
                                    panelY0 + headerHeight,
                                    20,
                                    48,
                                    75,
                                    255,
                                    14,
                                    32,
                                    52,
                                    255);

  // Pulsing cyber neon top border line and left accent bar
  unsigned char neonR = static_cast<unsigned char>(30.0f + pulse * 40.0f);
  unsigned char neonG = static_cast<unsigned char>(200.0f + pulse * 55.0f);
  unsigned char borderAlpha =
    static_cast<unsigned char>(160.0f + pulse * 75.0f);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0,
                         panelY0,
                         panelX1,
                         panelY0 + 2.0f,
                         neonR,
                         neonG,
                         255,
                         255);
  packed = packVerticalGradientQuad(batch,
                                    kCap,
                                    packed,
                                    panelX0,
                                    panelY0,
                                    panelX0 + 4.0f,
                                    panelY1,
                                    0,
                                    240,
                                    255,
                                    255,
                                    140,
                                    80,
                                    255,
                                    240);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0,
                         panelY0 + headerHeight,
                         panelX1,
                         panelY0 + headerHeight + 1.0f,
                         0,
                         210,
                         255,
                         borderAlpha);

  // In floating mode, add right-side and bottom border lines for a full frame.
  if (isFloating) {
    packed = packVerticalGradientQuad(batch,
                                      kCap,
                                      packed,
                                      panelX1 - 4.0f,
                                      panelY0,
                                      panelX1,
                                      panelY1,
                                      0,
                                      240,
                                      255,
                                      255,
                                      140,
                                      80,
                                      255,
                                      240);
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           panelX0,
                           panelY1 - 2.0f,
                           panelX1,
                           panelY1,
                           neonR,
                           neonG,
                           255,
                           200);
  }

  // Input row trough background
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 8.0f,
                         inputTop,
                         panelX1 - 8.0f,
                         panelY1 - 6.0f,
                         5,
                         11,
                         20,
                         245);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 8.0f,
                         inputTop,
                         panelX0 + 11.0f,
                         panelY1 - 6.0f,
                         0,
                         220,
                         255,
                         255);

  int totalLines = static_cast<int>(history.size());
  if (totalLines > maxHistoryLines) {
    float trackTop = historyTop;
    float trackBottom = historyBottom;
    float trackHeight = trackBottom - trackTop;
    float scrollbarWidth = 5.0f;
    float scrollbarRightMargin = 9.0f;
    float barX1 = panelX1 - scrollbarWidth - scrollbarRightMargin;
    float barX2 = panelX1 - scrollbarRightMargin;

    packed = packSolidQuad(
      batch, kCap, packed, barX1, trackTop, barX2, trackBottom, 3, 8, 15, 180);

    float thumbHeight = trackHeight * (static_cast<float>(maxHistoryLines) /
                                       static_cast<float>(totalLines));
    if (thumbHeight < 15.0f) {
      thumbHeight = 15.0f;
    }
    int maxScroll = totalLines - maxHistoryLines;
    float scrollPercent =
      (maxScroll > 0)
        ? (static_cast<float>(scrollOffset) / static_cast<float>(maxScroll))
        : 0.0f;
    float thumbTop =
      (trackBottom - thumbHeight) - scrollPercent * (trackHeight - thumbHeight);
    float thumbBottom = thumbTop + thumbHeight;
    packed = packVerticalGradientQuad(batch,
                                      kCap,
                                      packed,
                                      barX1,
                                      thumbTop,
                                      barX2,
                                      thumbBottom,
                                      0,
                                      230,
                                      255,
                                      255,
                                      70,
                                      130,
                                      220,
                                      255);
  }

  unsigned char titleColor[4] = { 180, 245, 255, 255 };
  unsigned char promptColor[4] = { 0, 240, 255, 255 };
  unsigned char inputColor[4] = { 240, 250, 255, 255 };

  std::string paramHint = getParameterHint(currentInput);
  std::string statusText;
  unsigned char statusColor[4];

  if (!completionHint.empty()) {
    statusText = completionHint;
    statusColor[0] = 180;
    statusColor[1] = 210;
    statusColor[2] = 255;
    statusColor[3] = 255;
  } else if (!paramHint.empty()) {
    statusText = paramHint;
    statusColor[0] = 255;
    statusColor[1] = 220;
    statusColor[2] = 100;
    statusColor[3] = 255;
  } else {
    statusText = "Tab complete  |  Ctrl+Arrows words  |  Ctrl+A select all";
    statusColor[0] = 130;
    statusColor[1] = 195;
    statusColor[2] = 230;
    statusColor[3] = 255;
  }

  const char* titleStr =
    isFloating ? "[ILLUMO // FLOATING CONSOLE]" : "[ILLUMO // DEV CONSOLE]";
  packed = packFontLine(
    batch, kCap, packed, panelX0 + 14.0f, panelY0 + 9.0f, titleStr, titleColor);

  const float titleWidth = measureFontText(titleStr);
  const float statusWidth = measureFontText(statusText);
  float statusX = panelX1 - statusWidth - 16.0f;
  if (statusX < panelX0 + 14.0f + titleWidth + 20.0f) {
    statusX = panelX0 + 14.0f + titleWidth + 20.0f;
  }

  packed = packFontLine(batch,
                        kCap,
                        packed,
                        statusX,
                        panelY0 + 9.0f,
                        statusText.c_str(),
                        statusColor);

  // History text is drawn after chrome, but before the input row, so its
  // clipping and scroll thumb agree with the available space.
  float currentY = historyTop;
  int endIdx = static_cast<int>(history.size()) - 1 - scrollOffset;
  if (endIdx >= 0) {
    int startIdx = endIdx - (maxHistoryLines - 1);
    if (startIdx < 0) {
      startIdx = 0;
    }
    for (int i = startIdx; i <= endIdx; ++i) {
      const historyBuffer& item = history[static_cast<size_t>(i)];
      unsigned char itemColor[4] = { item.r, item.g, item.b, item.a };
      packed = packFontLine(batch,
                            kCap,
                            packed,
                            panelX0 + 14.0f,
                            currentY,
                            item.content.c_str(),
                            itemColor);
      currentY += lineSpacing;
    }
  }

  const float inputTextX = panelX0 + 40.0f;
  const float inputAvailableWidth =
    std::max(48.0f, (panelX1 - panelX0) - 62.0f);
  std::size_t visibleStart = 0;
  while (visibleStart < cursorPosition) {
    std::string textThroughCursor =
      currentInput.substr(visibleStart, cursorPosition - visibleStart);
    if (measureFontText(textThroughCursor) <= inputAvailableWidth) {
      break;
    }
    ++visibleStart;
  }
  std::size_t visibleEnd = cursorPosition;
  while (visibleEnd < currentInput.size()) {
    std::string candidateText =
      currentInput.substr(visibleStart, visibleEnd + 1 - visibleStart);
    if (measureFontText(candidateText) > inputAvailableWidth) {
      break;
    }
    ++visibleEnd;
  }
  std::string visibleInput =
    currentInput.substr(visibleStart, visibleEnd - visibleStart);
  float inputY = inputTop + 12.0f;
  packed = packFontLine(
    batch, kCap, packed, panelX0 + 16.0f, inputY, ">", promptColor);

  if (hasSelection()) {
    std::size_t selectionStart = std::min(cursorPosition, selectionAnchor);
    std::size_t selectionEnd = std::max(cursorPosition, selectionAnchor);
    std::size_t highlightStart = std::max(selectionStart, visibleStart);
    std::size_t highlightEnd = std::min(selectionEnd, visibleEnd);
    if (highlightStart < highlightEnd) {
      std::string beforeSelection =
        visibleInput.substr(0, highlightStart - visibleStart);
      std::string selectedText = visibleInput.substr(
        highlightStart - visibleStart, highlightEnd - highlightStart);
      float highlightX0 = inputTextX + measureFontText(beforeSelection);
      float highlightX1 = highlightX0 + measureFontText(selectedText);
      packed = packSolidQuad(batch,
                             kCap,
                             packed,
                             highlightX0,
                             inputY - 3.0f,
                             highlightX1,
                             inputY + 16.0f,
                             38,
                             123,
                             181,
                             190);
    }
  }
  packed = packFontLine(
    batch, kCap, packed, inputTextX, inputY, visibleInput.c_str(), inputColor);

  // Futuristic Ghost Text Auto-Suggestion
  if (cursorPosition == currentInput.size() &&
      visibleEnd == currentInput.size()) {
    std::string ghostText = getGhostSuggestion();
    if (!ghostText.empty()) {
      float ghostX = inputTextX + measureFontText(visibleInput);
      if (ghostX < inputTextX + inputAvailableWidth) {
        unsigned char ghostColor[4] = { 0, 190, 230, 140 };
        packed = packFontLine(
          batch, kCap, packed, ghostX, inputY, ghostText.c_str(), ghostColor);
      }
    }
  }

  bool caretVisible = (caretMilliseconds % 1000) < 560;
  if (caretVisible && cursorPosition >= visibleStart &&
      cursorPosition <= visibleEnd) {
    std::string textBeforeCaret =
      visibleInput.substr(0, cursorPosition - visibleStart);
    float caretX = inputTextX + measureFontText(textBeforeCaret);
    // Glow backlight + sharp cursor stick
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           caretX - 2.0f,
                           inputY - 3.0f,
                           caretX + 4.0f,
                           inputY + 16.0f,
                           0,
                           200,
                           255,
                           80);
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           caretX,
                           inputY - 3.0f,
                           caretX + 2.0f,
                           inputY + 16.0f,
                           180,
                           245,
                           255,
                           255);
  }

  // Visual Autocomplete Dropdown Popup Overlay
  if (!currentInput.empty()) {
    std::vector<std::string> candidates = getCompletionCandidates(currentInput);
    if (!candidates.empty()) {
      float dropWidth = std::min(width - 32.0f, 400.0f);
      float dropX = 40.0f;
      int showCount = std::min(static_cast<int>(candidates.size()), 4);
      float dropItemHeight = 20.0f;
      float dropHeight = showCount * dropItemHeight + 6.0f;
      float dropY = inputTop - dropHeight - 4.0f;
      if (dropY >= yOffset + headerHeight) {
        packed = packSolidQuad(batch,
                               kCap,
                               packed,
                               dropX,
                               dropY,
                               dropX + dropWidth,
                               dropY + dropHeight,
                               10,
                               22,
                               38,
                               245);
        packed = packSolidQuad(batch,
                               kCap,
                               packed,
                               dropX,
                               dropY,
                               dropX + dropWidth,
                               dropY + 2.0f,
                               0,
                               220,
                               255,
                               255);

        for (int i = 0; i < showCount; ++i) {
          float itemY = dropY + 4.0f + i * dropItemHeight;
          unsigned char itemCol[4] = { 180, 225, 255, 255 };
          if (i == 0) {
            packed = packSolidQuad(batch,
                                   kCap,
                                   packed,
                                   dropX + 2.0f,
                                   itemY - 1.0f,
                                   dropX + dropWidth - 2.0f,
                                   itemY + 17.0f,
                                   20,
                                   70,
                                   110,
                                   220);
            itemCol[0] = 255;
            itemCol[1] = 255;
            itemCol[2] = 255;
            itemCol[3] = 255;
          }
          std::string itemText =
            std::string(i == 0 ? " -> " : "    ") + candidates[i];
          packed = packFontLine(batch,
                                kCap,
                                packed,
                                dropX + 6.0f,
                                itemY,
                                itemText.c_str(),
                                itemCol);
        }
      }
    }
  }

  if (packed < 4) {
    return true;
  }

  const unsigned int totalQuads = packed / 4;
  // Dynamic mesh index buffer covers kUiQuadCap quads; clamp draw if somehow
  // larger.
  unsigned int drawQuads = totalQuads;
  if (drawQuads > kUiQuadCap) {
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
