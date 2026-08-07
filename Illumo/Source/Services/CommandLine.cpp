#include "CommandLine.h"
#include "CellMain.h"
#include "Rendering/IMesh.h"
#include "Rendering/IShaderProgram.h"
#include "Rendering/Renderer.h"
#include "Services/Logger.h"
#include "thirdparty/stb/stb_easy_font.h"
#include <algorithm>
#include <cctype>
#include <cmath>
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

// stb_easy_font only indexes charinfo for ASCII 32..126 (plus '\n'). Any
// other byte (UTF-8, tab, CR, high-bit) is a global-buffer-overflow under
// ASan. Map unsupported bytes to printable ASCII before measuring/printing.
static std::string
sanitizeEasyFontText(const std::string& text)
{
  std::string sanitized;
  sanitized.reserve(text.size());
  for (unsigned char byte : text) {
    if (byte == '\n') {
      sanitized.push_back('\n');
    } else if (byte >= 32 && byte <= 126) {
      sanitized.push_back(static_cast<char>(byte));
    } else if (byte == '\t') {
      sanitized.append("  ");
    } else if (byte == '\r') {
      // ignore CR; LF alone is enough for wrap/height
    } else {
      sanitized.push_back('?');
    }
  }
  return sanitized;
}

static float
measureFontText(const std::string& text)
{
  if (text.empty()) {
    return 0.0f;
  }
  std::string mutableText = sanitizeEasyFontText(text);
  if (mutableText.empty()) {
    return 0.0f;
  }
  return static_cast<float>(stb_easy_font_width(mutableText.data()) * 2);
}

static std::string
truncateTextToWidth(const std::string& text, float maxWidth)
{
  if (maxWidth <= 8.0f || text.empty()) {
    return "";
  }
  if (measureFontText(text) <= maxWidth) {
    return text;
  }
  std::string result = text;
  while (!result.empty() && measureFontText(result) > maxWidth) {
    result.pop_back();
  }
  return result;
}

// Strip trailing CR/LF so AppendStringLn does not draw a phantom second line.
static std::string
stripTrailingLineEndings(const std::string& text)
{
  std::string cleaned = text;
  while (!cleaned.empty() &&
         (cleaned.back() == '\n' || cleaned.back() == '\r')) {
    cleaned.pop_back();
  }
  return cleaned;
}

// Longest prefix of text[start..) that fits in maxWidth (at least 1 when
// possible). Binary search keeps wrap cost low for long help lines.
static std::size_t
findMaxFitLength(const std::string& text, std::size_t start, float maxWidth)
{
  if (start >= text.size() || maxWidth <= 0.0f) {
    return 0;
  }
  if (measureFontText(text.substr(start, 1)) > maxWidth) {
    return 1;
  }
  std::size_t lo = 1;
  std::size_t hi = text.size() - start;
  std::size_t best = 1;
  while (lo <= hi) {
    std::size_t mid = lo + (hi - lo) / 2;
    if (measureFontText(text.substr(start, mid)) <= maxWidth) {
      best = mid;
      lo = mid + 1;
    } else {
      if (mid == 0) {
        break;
      }
      hi = mid - 1;
    }
  }
  return best;
}

// Word-aware wrap so history is readable instead of hard-truncated.
static void
wrapTextToWidth(const std::string& text,
                float maxWidth,
                std::vector<std::string>& outLines)
{
  outLines.clear();
  const std::string cleaned = stripTrailingLineEndings(text);
  if (cleaned.empty()) {
    outLines.push_back("");
    return;
  }
  if (maxWidth <= 8.0f) {
    outLines.push_back(cleaned);
    return;
  }

  std::size_t paragraphStart = 0;
  while (paragraphStart <= cleaned.size()) {
    std::size_t paragraphEnd = cleaned.find('\n', paragraphStart);
    if (paragraphEnd == std::string::npos) {
      paragraphEnd = cleaned.size();
    }
    std::string paragraph =
      cleaned.substr(paragraphStart, paragraphEnd - paragraphStart);
    if (!paragraph.empty() && paragraph.back() == '\r') {
      paragraph.pop_back();
    }

    if (paragraph.empty()) {
      outLines.push_back("");
    } else if (measureFontText(paragraph) <= maxWidth) {
      outLines.push_back(paragraph);
    } else {
      std::size_t lineStart = 0;
      while (lineStart < paragraph.size()) {
        while (lineStart < paragraph.size() &&
               (paragraph[lineStart] == ' ' || paragraph[lineStart] == '\t')) {
          ++lineStart;
        }
        if (lineStart >= paragraph.size()) {
          break;
        }
        const std::size_t fitLen =
          findMaxFitLength(paragraph, lineStart, maxWidth);
        if (fitLen == 0) {
          break;
        }
        std::size_t lineEnd = lineStart + fitLen;
        std::size_t breakAt = lineEnd;
        if (lineEnd < paragraph.size()) {
          std::size_t spaceBreak = paragraph.find_last_of(" \t", lineEnd - 1);
          if (spaceBreak != std::string::npos && spaceBreak >= lineStart) {
            breakAt = spaceBreak;
          }
        }
        if (breakAt == lineStart) {
          breakAt = lineEnd;
        }
        outLines.push_back(paragraph.substr(lineStart, breakAt - lineStart));
        lineStart = breakAt;
        if (lineStart < paragraph.size() &&
            (paragraph[lineStart] == ' ' || paragraph[lineStart] == '\t')) {
          ++lineStart;
        }
      }
    }

    if (paragraphEnd >= cleaned.size()) {
      break;
    }
    paragraphStart = paragraphEnd + 1;
  }

  if (outLines.empty()) {
    outLines.push_back("");
  }
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
  , floatingY(20.0f)
  , floatingW(-1.0f)
  , floatingH(-1.0f)
  , isDraggingWindow(false)
  , isResizingWindow(false)
  , dragWindowOffsetX(0.0f)
  , dragWindowOffsetY(0.0f)
  , resizeStartW(0.0f)
  , resizeStartH(0.0f)
  , resizeStartMouseX(0.0f)
  , resizeStartMouseY(0.0f)
  , lastHeaderClickTime(std::chrono::high_resolution_clock::time_point{})
  , currentPanelX(-1.0f)
  , currentPanelY(-1.0f)
  , currentPanelW(-1.0f)
  , currentPanelH(-1.0f)
  , scanlinePhase(0.0f)
  , uiVerts(std::make_unique<ConsoleVertex[]>(kUiVertCap))
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

  if (commandRegistry) {
    commandRegistry->RegisterCommand(
      "console_mode",
      [this](const std::vector<std::string>& args) {
        if (args.empty()) {
          logNormal("Console mode: " +
                    std::string(isFloating ? "FLOATING" : "MOUNTED"));
          logNormal("Usage: console_mode [floating|mounted|toggle]");
          return;
        }
        std::string mode = lowerCopy(args[0]);
        if (mode == "floating" || mode == "float") {
          setFloatingMode(true);
        } else if (mode == "mounted" || mode == "mount") {
          setFloatingMode(false);
        } else if (mode == "toggle") {
          ToggleFloatingMode();
        } else {
          logError("Unknown console mode: " + args[0]);
        }
      },
      "console_mode [floating|mounted|toggle]",
      "Switch console between floating and top-mounted modes",
      { "floating", "mounted", "toggle" });

    commandRegistry->RegisterCommand(
      "console_size",
      [this](const std::vector<std::string>& args) {
        if (args.empty()) {
          if (floatingW > 0.0f && floatingH > 0.0f) {
            logNormal(
              "Console size: " + std::to_string(static_cast<int>(floatingW)) +
              "x" + std::to_string(static_cast<int>(floatingH)));
          } else {
            logNormal("Console size: default/auto");
          }
          logNormal("Usage: console_size [<width> <height> | reset]");
          return;
        }
        std::string first = lowerCopy(args[0]);
        if (first == "reset" || first == "default") {
          floatingW = -1.0f;
          floatingH = -1.0f;
          logSuccess("Console size reset to default");
          return;
        }
        if (args.size() >= 2) {
          long w = 0, h = 0;
          if (parseLongStrict(args[0], &w) && parseLongStrict(args[1], &h)) {
            setFloatingSize(static_cast<float>(w), static_cast<float>(h));
            return;
          }
        }
        logError("Usage: console_size [<width> <height> | reset]");
      },
      "console_size [<width> <height> | reset]",
      "Set or reset floating console window dimensions",
      { "800 500", "1000 600", "reset" });
  }

  enrollGpuResources();
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
    std::array<int, 2> windowDimensions =
      window ? window->getWindowDimensions() : std::array<int, 2>{ 1280, 720 };
    float width = static_cast<float>(windowDimensions[0]);
    float defaultMarginX = std::clamp(width * 0.08f, 20.0f, 100.0f);
    if (floatingX < 0.0f) {
      floatingX = defaultMarginX;
      floatingY = 20.0f;
    }
  } else {
    isDraggingWindow = false;
    isResizingWindow = false;
  }
  logSuccess(isFloating ? "Console mode set to FLOATING"
                        : "Console mode set to MOUNTED");
}

void
CommandLine::setFloatingSize(float w, float h)
{
  floatingW = std::max(280.0f, w);
  floatingH = std::max(180.0f, h);
  if (!isFloating) {
    setFloatingMode(true);
  } else {
    logSuccess("Console size set to " +
               std::to_string(static_cast<int>(floatingW)) + "x" +
               std::to_string(static_cast<int>(floatingH)));
  }
}

CommandLine::PanelLayout
CommandLine::computePanelLayout(bool useSmoothedPanel) const
{
  std::array<int, 2> windowDimensions =
    window ? window->getWindowDimensions() : std::array<int, 2>{ 1280, 720 };
  float width = static_cast<float>(windowDimensions[0]);
  float height = static_cast<float>(windowDimensions[1]);

  float defaultMarginX = std::clamp(width * 0.08f, 20.0f, 100.0f);
  float defaultFloatW = std::max(280.0f, width - defaultMarginX * 2.0f);
  float defaultFloatH = height * 0.52f;
  if (defaultFloatH < 240.0f) {
    defaultFloatH = 240.0f;
  }
  if (defaultFloatH > height - 20.0f) {
    defaultFloatH = height - 20.0f;
  }

  float floatW = (isFloating && floatingW > 0.0f)
                   ? std::clamp(floatingW, 280.0f, width)
                   : defaultFloatW;
  float floatH = (isFloating && floatingH > 0.0f)
                   ? std::clamp(floatingH, 180.0f, height)
                   : defaultFloatH;

  float resolvedX =
    isFloating
      ? ((floatingX < 0.0f) ? defaultMarginX
                            : std::clamp(floatingX, 0.0f, width - floatW))
      : 0.0f;
  float resolvedY =
    isFloating
      ? ((floatingY < 0.0f) ? 20.0f
                            : std::clamp(floatingY, 0.0f, height - floatH))
      : 0.0f;
  float targetW = isFloating ? floatW : width;
  float targetH = isFloating ? floatH : defaultFloatH;

  float panelW = targetW;
  float panelH = targetH;
  float panelX = resolvedX;
  float panelY = resolvedY;
  if (useSmoothedPanel && currentPanelX >= 0.0f && currentPanelW > 0.0f &&
      currentPanelH > 0.0f) {
    panelX = currentPanelX;
    panelY = currentPanelY;
    panelW = currentPanelW;
    panelH = currentPanelH;
  }

  float effectiveAnim =
    (animationProgress > 0.0f) ? animationProgress : (isOpen ? 1.0f : 0.0f);
  // Match AppendCommands cubic ease so hit-tests track the drawn panel.
  float easeProgress = 1.0f - std::pow(1.0f - effectiveAnim, 3.0f);
  float ySlide =
    isFloating ? (panelY - (1.0f - easeProgress) * (panelY + panelH + 40.0f))
               : (-panelH * (1.0f - easeProgress));

  PanelLayout layout;
  layout.panelX0 = isFloating ? panelX : 0.0f;
  layout.panelY0 = ySlide;
  layout.panelX1 = layout.panelX0 + (isFloating ? panelW : width);
  layout.panelY1 = layout.panelY0 + panelH;
  layout.headerHeight = 34.0f;
  layout.inputRowHeight = 40.0f;
  layout.historyTop = layout.panelY0 + layout.headerHeight + 8.0f;
  layout.inputTop = layout.panelY1 - layout.inputRowHeight;
  layout.historyBottom = layout.inputTop - 8.0f;
  layout.lineSpacing = 24.0f;
  layout.maxHistoryLines = static_cast<int>(
    (layout.historyBottom - layout.historyTop) / layout.lineSpacing);
  if (layout.maxHistoryLines < 1) {
    layout.maxHistoryLines = 1;
  }
  layout.historyBaseWidth =
    std::max(20.0f, (layout.panelX1 - layout.panelX0) - 32.0f);
  return layout;
}

int
CommandLine::countWrappedHistoryLines(float availableWidth) const
{
  int total = 0;
  std::vector<std::string> wrapped;
  for (const historyBuffer& item : history) {
    wrapTextToWidth(item.content, availableWidth, wrapped);
    total += static_cast<int>(wrapped.size());
  }
  return total;
}

void
CommandLine::computeHistoryScrollLimits(int* maxHistoryLines,
                                        int* maxScroll,
                                        float* historyWidth) const
{
  // Prefer the smoothed panel once it has been initialized so PageUp/wheel
  // limits match the drawn history viewport (including floating resize).
  const bool useSmoothed =
    currentPanelX >= 0.0f && currentPanelW > 0.0f && currentPanelH > 0.0f;
  PanelLayout layout = computePanelLayout(useSmoothed);
  if (maxHistoryLines != nullptr) {
    *maxHistoryLines = layout.maxHistoryLines;
  }

  float widthNoBar = layout.historyBaseWidth;
  float widthWithBar = std::max(20.0f, widthNoBar - 16.0f);
  int linesNoBar = countWrappedHistoryLines(widthNoBar);
  bool needsBar = linesNoBar > layout.maxHistoryLines;
  float usedWidth = needsBar ? widthWithBar : widthNoBar;
  int totalVisual = needsBar ? countWrappedHistoryLines(usedWidth) : linesNoBar;
  int scrollMax = totalVisual - layout.maxHistoryLines;
  if (scrollMax < 0) {
    scrollMax = 0;
  }

  if (maxScroll != nullptr) {
    *maxScroll = scrollMax;
  }
  if (historyWidth != nullptr) {
    *historyWidth = usedWidth;
  }
}

void
CommandLine::clampScrollOffset()
{
  int maxHistoryLines = 1;
  int maxScroll = 0;
  computeHistoryScrollLimits(&maxHistoryLines, &maxScroll, nullptr);
  if (scrollOffset < 0) {
    scrollOffset = 0;
  }
  if (scrollOffset > maxScroll) {
    scrollOffset = maxScroll;
  }
}

void
CommandLine::enrollGpuResources()
{
  gpuReady = false;
  consoleInitialized = false;
  if (!renderer) {
    Logger::LogError("CommandLine: no Renderer - cannot enroll GPU resources");
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
    animationProgress = 0.0f;
    lastAnimTime = std::chrono::high_resolution_clock::now();
    scrollOffset = 0;
    completionHint = "Tab complete | Ctrl+Arrows words | Ctrl+A select";
  } else {
    isDraggingWindow = false;
    isResizingWindow = false;
    isDraggingScrollbar = false;
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

  for (const std::string& singleCmd : subCommands) {
    ExecuteSingleCommand(singleCmd, 0);
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
    scrollOffset = 0;
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
  int maxHistoryLines = 1;
  int maxScroll = 0;
  computeHistoryScrollLimits(&maxHistoryLines, &maxScroll, nullptr);
  if (scrollOffset < maxScroll) {
    ++scrollOffset;
  }
}

void
CommandLine::ScrollDown()
{
  if (scrollOffset > 0) {
    --scrollOffset;
  }
}

void
CommandLine::HandleScroll(double yOffset)
{
  if (!isOpen || history.empty()) {
    return;
  }
  int maxHistoryLines = 1;
  int maxScroll = 0;
  computeHistoryScrollLimits(&maxHistoryLines, &maxScroll, nullptr);
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
  float defaultMarginX = std::clamp(width * 0.08f, 20.0f, 100.0f);
  float defaultFloatW = std::max(280.0f, width - defaultMarginX * 2.0f);
  float defaultFloatH = height * 0.52f;
  if (defaultFloatH < 240.0f) {
    defaultFloatH = 240.0f;
  }
  if (defaultFloatH > height - 20.0f) {
    defaultFloatH = height - 20.0f;
  }

  float floatW = (isFloating && floatingW > 0.0f)
                   ? std::clamp(floatingW, 280.0f, width)
                   : defaultFloatW;
  float floatH = (isFloating && floatingH > 0.0f)
                   ? std::clamp(floatingH, 180.0f, height)
                   : defaultFloatH;
  float panelHeight = isFloating ? floatH : defaultFloatH;

  float effectiveAnim =
    (animationProgress > 0.0f) ? animationProgress : (isOpen ? 1.0f : 0.0f);
  float yOffset = -panelHeight * (1.0f - effectiveAnim);

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

  // Corner resize grip (bottom-right 18x18 region)
  if (isFloating && mouseX >= panelX1 - 18.0f && mouseX <= panelX1 &&
      mouseY >= panelY1 - 18.0f && mouseY <= panelY1) {
    if (!isDrag) {
      isResizingWindow = true;
      resizeStartW = panelX1 - panelX0;
      resizeStartH = panelY1 - panelY0;
      resizeStartMouseX = static_cast<float>(mouseX);
      resizeStartMouseY = static_cast<float>(mouseY);
      return;
    }
  }

  if (mouseY >= panelY0 && mouseY <= panelY0 + headerHeight) {
    if (!isDrag) {
      if (isFloating && mouseX >= panelX1 - 32.0f && mouseX <= panelX1 - 6.0f) {
        Toggle();
        return;
      }
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

  int maxHistoryLines = 1;
  int maxScroll = 0;
  computeHistoryScrollLimits(&maxHistoryLines, &maxScroll, nullptr);

  if (maxScroll > 0) {
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
      scrollOffset =
        static_cast<int>(clickRatio * static_cast<float>(maxScroll));
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
      std::max(48.0f, (panelX1 - panelX0) - 40.0f - 22.0f);
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
  if (isResizingWindow && isFloating) {
    std::array<int, 2> windowDimensions =
      window ? window->getWindowDimensions() : std::array<int, 2>{ 1280, 720 };
    float width = static_cast<float>(windowDimensions[0]);
    float height = static_cast<float>(windowDimensions[1]);

    float deltaX = static_cast<float>(mouseX) - resizeStartMouseX;
    float deltaY = static_cast<float>(mouseY) - resizeStartMouseY;

    floatingW = std::clamp(resizeStartW + deltaX, 280.0f, width - floatingX);
    floatingH = std::clamp(resizeStartH + deltaY, 180.0f, height - floatingY);
    return;
  }
  if (isDraggingWindow && isFloating) {
    std::array<int, 2> windowDimensions =
      window ? window->getWindowDimensions() : std::array<int, 2>{ 1280, 720 };
    float width = static_cast<float>(windowDimensions[0]);
    float height = static_cast<float>(windowDimensions[1]);
    float defaultMarginX = std::clamp(width * 0.08f, 20.0f, 100.0f);
    float defaultFloatW = std::max(280.0f, width - defaultMarginX * 2.0f);
    float defaultFloatH = height * 0.52f;
    if (defaultFloatH < 240.0f) {
      defaultFloatH = 240.0f;
    }
    if (defaultFloatH > height - 20.0f) {
      defaultFloatH = height - 20.0f;
    }
    float floatW = (floatingW > 0.0f) ? floatingW : defaultFloatW;
    float floatH = (floatingH > 0.0f) ? floatingH : defaultFloatH;

    floatingX = std::clamp(
      static_cast<float>(mouseX) - dragWindowOffsetX, 0.0f, width - floatW);
    floatingY = std::clamp(
      static_cast<float>(mouseY) - dragWindowOffsetY, 0.0f, height - floatH);
    return;
  }
  if (isDraggingScrollbar) {
    PanelLayout layout = computePanelLayout(true);
    int maxHistoryLines = 1;
    int maxScroll = 0;
    computeHistoryScrollLimits(&maxHistoryLines, &maxScroll, nullptr);

    float trackHeight = layout.historyBottom - layout.historyTop;
    if (trackHeight > 0.0f && maxScroll > 0) {
      float deltaY = static_cast<float>(mouseY) - dragStartY;
      float deltaScrollRatio = -deltaY / trackHeight;
      int newScroll =
        dragStartScrollOffset +
        static_cast<int>(deltaScrollRatio * static_cast<float>(maxScroll));
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
  isResizingWindow = false;
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
  clampScrollOffset();
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
  clampScrollOffset();
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

static unsigned int
packHorizontalGradientQuad(UiVert* dest,
                           unsigned int destCap,
                           unsigned int writeAt,
                           float x0,
                           float y0,
                           float x1,
                           float y1,
                           unsigned char leftR,
                           unsigned char leftG,
                           unsigned char leftB,
                           unsigned char leftA,
                           unsigned char rightR,
                           unsigned char rightG,
                           unsigned char rightB,
                           unsigned char rightA)
{
  if (writeAt + 4 > destCap) {
    return writeAt;
  }
  dest[writeAt + 0] = { x0, y0, 0.0f, { leftR, leftG, leftB, leftA } };
  dest[writeAt + 1] = { x1, y0, 0.0f, { rightR, rightG, rightB, rightA } };
  dest[writeAt + 2] = { x1, y1, 0.0f, { rightR, rightG, rightB, rightA } };
  dest[writeAt + 3] = { x0, y1, 0.0f, { leftR, leftG, leftB, leftA } };
  return writeAt + 4;
}

// Small L-bracket used for HUD corner accents without heavy CRT clutter.
static unsigned int
packCornerBracket(UiVert* dest,
                  unsigned int destCap,
                  unsigned int writeAt,
                  float x,
                  float y,
                  float arm,
                  float thickness,
                  int corner,
                  unsigned char r,
                  unsigned char g,
                  unsigned char b,
                  unsigned char a)
{
  // corner: 0=TL, 1=TR, 2=BL, 3=BR
  float xSign = (corner == 1 || corner == 3) ? -1.0f : 1.0f;
  float ySign = (corner == 2 || corner == 3) ? -1.0f : 1.0f;
  float hx0 = x;
  float hx1 = x + xSign * arm;
  float hy0 = y;
  float hy1 = y + ySign * thickness;
  if (hx0 > hx1) {
    float tmp = hx0;
    hx0 = hx1;
    hx1 = tmp;
  }
  if (hy0 > hy1) {
    float tmp = hy0;
    hy0 = hy1;
    hy1 = tmp;
  }
  writeAt =
    packSolidQuad(dest, destCap, writeAt, hx0, hy0, hx1, hy1, r, g, b, a);

  float vx0 = x;
  float vx1 = x + xSign * thickness;
  float vy0 = y;
  float vy1 = y + ySign * arm;
  if (vx0 > vx1) {
    float tmp = vx0;
    vx0 = vx1;
    vx1 = tmp;
  }
  if (vy0 > vy1) {
    float tmp = vy0;
    vy0 = vy1;
    vy1 = tmp;
  }
  return packSolidQuad(dest, destCap, writeAt, vx0, vy0, vx1, vy1, r, g, b, a);
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
  if (writeAt >= destCap || text == nullptr || text[0] == '\0') {
    return writeAt;
  }
  // Sanitize into a stable buffer: stb_easy_font mutates nothing but still
  // requires ASCII-only code points for its glyph table.
  std::string sanitized = sanitizeEasyFontText(text);
  if (sanitized.empty()) {
    return writeAt;
  }
  const unsigned int remaining = destCap - writeAt;
  int numQuads =
    stb_easy_font_print(x * 0.5f,
                        y * 0.5f,
                        sanitized.data(),
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
  const float animationSpeed = 12.0f;
  if (isOpen) {
    animationProgress =
      Math::lerp(animationProgress, 1.0f, animationSpeed * deltaTime);
    if (animationProgress > 0.999f) {
      animationProgress = 1.0f;
    }
  } else {
    animationProgress =
      Math::lerp(animationProgress, 0.0f, animationSpeed * deltaTime);
    if (animationProgress < 0.01f) {
      animationProgress = 0.0f;
    }
  }
  if (!isOpen && animationProgress <= 0.0f) {
    return true;
  }
  // Cubic Ease-Out for ultra-smooth decelerating open/close animations
  float easeProgress = 1.0f - std::pow(1.0f - animationProgress, 3.0f);

  std::array<int, 2> windowDimensions = window->getWindowDimensions();
  int winWidth = windowDimensions[0];
  int winHeight = windowDimensions[1];
  float width = static_cast<float>(winWidth);
  float height = static_cast<float>(winHeight);

  float defaultMarginX = std::clamp(width * 0.08f, 20.0f, 100.0f);
  float defaultFloatW = std::max(280.0f, width - defaultMarginX * 2.0f);
  float defaultFloatH = height * 0.52f;
  if (defaultFloatH < 240.0f) {
    defaultFloatH = 240.0f;
  }
  if (defaultFloatH > height - 20.0f) {
    defaultFloatH = height - 20.0f;
  }

  float floatW = (isFloating && floatingW > 0.0f)
                   ? std::clamp(floatingW, 280.0f, width)
                   : defaultFloatW;
  float floatH = (isFloating && floatingH > 0.0f)
                   ? std::clamp(floatingH, 180.0f, height)
                   : defaultFloatH;

  float targetX0 =
    isFloating ? std::clamp(floatingX, 0.0f, width - floatW) : 0.0f;
  float targetY0 =
    isFloating ? std::clamp(floatingY, 0.0f, height - floatH) : 0.0f;
  float targetW = isFloating ? floatW : width;
  float targetH = isFloating ? floatH : defaultFloatH;

  if (isFloating && floatingX < 0.0f) {
    floatingX = defaultMarginX;
    floatingY = 20.0f;
    targetX0 = defaultMarginX;
    targetY0 = 20.0f;
  }

  if (currentPanelX < 0.0f) {
    currentPanelX = targetX0;
    currentPanelY = targetY0;
    currentPanelW = targetW;
    currentPanelH = targetH;
  } else {
    float smoothSpeed = (isDraggingWindow || isResizingWindow) ? 35.0f : 16.0f;
    currentPanelX =
      Math::lerp(currentPanelX, targetX0, smoothSpeed * deltaTime);
    currentPanelY =
      Math::lerp(currentPanelY, targetY0, smoothSpeed * deltaTime);
    currentPanelW = Math::lerp(currentPanelW, targetW, smoothSpeed * deltaTime);
    currentPanelH = Math::lerp(currentPanelH, targetH, smoothSpeed * deltaTime);
  }

  float panelHeight = currentPanelH;
  float ySlide = isFloating
                   ? (currentPanelY - (1.0f - easeProgress) *
                                        (currentPanelY + panelHeight + 40.0f))
                   : (-panelHeight * (1.0f - easeProgress));

  float panelX0 = currentPanelX;
  float panelX1 = currentPanelX + currentPanelW;
  float panelY0 = ySlide;
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
  UiVert* batch = reinterpret_cast<UiVert*>(uiVerts.get());

  long long caretMilliseconds =
    std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch())
      .count();
  float pulse =
    (std::sin(static_cast<float>(caretMilliseconds) * 0.0045f) + 1.0f) * 0.5f;
  float breath =
    (std::sin(static_cast<float>(caretMilliseconds) * 0.0022f) + 1.0f) * 0.5f;

  // Soft multi-layer drop shadow (floating gets a deeper cast).
  const float shadowLift = isFloating ? 10.0f : 5.0f;
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + shadowLift,
                         panelY0 + shadowLift + 2.0f,
                         panelX1 + shadowLift,
                         panelY1 + shadowLift + 2.0f,
                         0,
                         0,
                         0,
                         isFloating ? 90u : 55u);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 3.0f,
                         panelY0 + 4.0f,
                         panelX1 + 3.0f,
                         panelY1 + 4.0f,
                         0,
                         8,
                         16,
                         isFloating ? 110u : 70u);

  // Outer neon halo
  unsigned char haloA = static_cast<unsigned char>(18.0f + breath * 22.0f);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 - 2.0f,
                         panelY0 - 2.0f,
                         panelX1 + 2.0f,
                         panelY1 + 2.0f,
                         0,
                         190,
                         255,
                         haloA);

  // Main glass body — deep navy with a subtle top-lit gradient
  packed = packVerticalGradientQuad(batch,
                                    kCap,
                                    packed,
                                    panelX0,
                                    panelY0,
                                    panelX1,
                                    panelY1,
                                    10,
                                    18,
                                    32,
                                    250,
                                    4,
                                    8,
                                    16,
                                    248);

  // Inner panel fill slightly inset for a bezel feel
  packed = packVerticalGradientQuad(batch,
                                    kCap,
                                    packed,
                                    panelX0 + 2.0f,
                                    panelY0 + 2.0f,
                                    panelX1 - 2.0f,
                                    panelY1 - 2.0f,
                                    14,
                                    24,
                                    40,
                                    255,
                                    7,
                                    12,
                                    22,
                                    255);

  // Header bar with horizontal sheen
  packed = packVerticalGradientQuad(batch,
                                    kCap,
                                    packed,
                                    panelX0 + 2.0f,
                                    panelY0 + 2.0f,
                                    panelX1 - 2.0f,
                                    panelY0 + headerHeight,
                                    24,
                                    52,
                                    78,
                                    255,
                                    12,
                                    28,
                                    48,
                                    255);
  packed = packHorizontalGradientQuad(batch,
                                      kCap,
                                      packed,
                                      panelX0 + 2.0f,
                                      panelY0 + 2.0f,
                                      panelX1 - 2.0f,
                                      panelY0 + 4.0f,
                                      120,
                                      230,
                                      255,
                                      40,
                                      40,
                                      120,
                                      200,
                                      8);

  // Accent palette (calm cyan, mild pulse)
  unsigned char neonR = static_cast<unsigned char>(20.0f + pulse * 30.0f);
  unsigned char neonG = static_cast<unsigned char>(210.0f + pulse * 40.0f);
  unsigned char neonB = 255;
  unsigned char borderAlpha =
    static_cast<unsigned char>(150.0f + pulse * 70.0f);

  // Thin outer frame
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0,
                         panelY0,
                         panelX1,
                         panelY0 + 2.0f,
                         neonR,
                         neonG,
                         neonB,
                         255);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0,
                         panelY1 - 2.0f,
                         panelX1,
                         panelY1,
                         neonR,
                         neonG,
                         neonB,
                         static_cast<unsigned char>(160.0f + breath * 50.0f));
  packed = packVerticalGradientQuad(batch,
                                    kCap,
                                    packed,
                                    panelX0,
                                    panelY0,
                                    panelX0 + 3.0f,
                                    panelY1,
                                    0,
                                    245,
                                    255,
                                    255,
                                    0,
                                    140,
                                    220,
                                    200);
  packed = packVerticalGradientQuad(batch,
                                    kCap,
                                    packed,
                                    panelX1 - 3.0f,
                                    panelY0,
                                    panelX1,
                                    panelY1,
                                    0,
                                    245,
                                    255,
                                    220,
                                    0,
                                    140,
                                    220,
                                    180);

  // Header separator with soft glow
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 4.0f,
                         panelY0 + headerHeight,
                         panelX1 - 4.0f,
                         panelY0 + headerHeight + 1.0f,
                         0,
                         220,
                         255,
                         borderAlpha);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 8.0f,
                         panelY0 + headerHeight + 1.0f,
                         panelX1 - 8.0f,
                         panelY0 + headerHeight + 3.0f,
                         0,
                         180,
                         255,
                         30);

  // HUD corner brackets (clean L shapes)
  unsigned char bracketA = static_cast<unsigned char>(190.0f + pulse * 50.0f);
  packed = packCornerBracket(batch,
                             kCap,
                             packed,
                             panelX0 + 1.0f,
                             panelY0 + 1.0f,
                             16.0f,
                             2.0f,
                             0,
                             0,
                             245,
                             255,
                             bracketA);
  packed = packCornerBracket(batch,
                             kCap,
                             packed,
                             panelX1 - 1.0f,
                             panelY0 + 1.0f,
                             16.0f,
                             2.0f,
                             1,
                             0,
                             245,
                             255,
                             bracketA);
  packed = packCornerBracket(batch,
                             kCap,
                             packed,
                             panelX0 + 1.0f,
                             panelY1 - 1.0f,
                             16.0f,
                             2.0f,
                             2,
                             0,
                             245,
                             255,
                             bracketA);
  packed = packCornerBracket(batch,
                             kCap,
                             packed,
                             panelX1 - 1.0f,
                             panelY1 - 1.0f,
                             16.0f,
                             2.0f,
                             3,
                             0,
                             245,
                             255,
                             bracketA);

  // History viewport: recessed well with quiet scanlines + sweep
  packed = packVerticalGradientQuad(batch,
                                    kCap,
                                    packed,
                                    panelX0 + 6.0f,
                                    historyTop - 4.0f,
                                    panelX1 - 6.0f,
                                    historyBottom + 4.0f,
                                    6,
                                    12,
                                    22,
                                    230,
                                    3,
                                    7,
                                    14,
                                    235);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 6.0f,
                         historyTop - 4.0f,
                         panelX1 - 6.0f,
                         historyTop - 3.0f,
                         0,
                         160,
                         220,
                         40);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 6.0f,
                         historyBottom + 3.0f,
                         panelX1 - 6.0f,
                         historyBottom + 4.0f,
                         0,
                         160,
                         220,
                         30);

  // Sparse scanlines — one every 6px keeps the CRT cue without muddying text
  for (float scanY = historyTop; scanY < historyBottom; scanY += 6.0f) {
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           panelX0 + 8.0f,
                           scanY,
                           panelX1 - 8.0f,
                           scanY + 1.0f,
                           40,
                           160,
                           220,
                           8);
  }

  // Slow vertical beam sweep
  scanlinePhase += deltaTime * 0.35f;
  float historySpan = std::max(1.0f, historyBottom - historyTop);
  float laserSweepY = historyTop + std::fmod(scanlinePhase, 1.0f) * historySpan;
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 8.0f,
                         laserSweepY - 1.0f,
                         panelX1 - 8.0f,
                         laserSweepY + 2.0f,
                         0,
                         230,
                         255,
                         28);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 8.0f,
                         laserSweepY,
                         panelX1 - 8.0f,
                         laserSweepY + 1.0f,
                         180,
                         250,
                         255,
                         55);

  // Soft top-left glass highlight on the history well
  packed = packHorizontalGradientQuad(batch,
                                      kCap,
                                      packed,
                                      panelX0 + 10.0f,
                                      historyTop,
                                      panelX0 + 160.0f,
                                      historyTop + 36.0f,
                                      200,
                                      240,
                                      255,
                                      12,
                                      200,
                                      240,
                                      255,
                                      0);

  // Top / bottom vignette so text eases into the chrome
  packed = packVerticalGradientQuad(batch,
                                    kCap,
                                    packed,
                                    panelX0 + 8.0f,
                                    historyTop,
                                    panelX1 - 8.0f,
                                    historyTop + 14.0f,
                                    0,
                                    0,
                                    0,
                                    90,
                                    0,
                                    0,
                                    0,
                                    0);
  packed = packVerticalGradientQuad(batch,
                                    kCap,
                                    packed,
                                    panelX0 + 8.0f,
                                    historyBottom - 14.0f,
                                    panelX1 - 8.0f,
                                    historyBottom,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    100);

  // Floating-only: stronger right edge + refined resize grip
  if (isFloating) {
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           panelX0 + 4.0f,
                           panelY1 - 2.0f,
                           panelX1 - 4.0f,
                           panelY1,
                           neonR,
                           neonG,
                           neonB,
                           200);

    // Resize grip — dark plate with three diagonal ticks
    float gripX0 = panelX1 - 20.0f;
    float gripY0 = panelY1 - 20.0f;
    float gripX1 = panelX1 - 3.0f;
    float gripY1 = panelY1 - 3.0f;
    packed = packSolidQuad(
      batch, kCap, packed, gripX0, gripY0, gripX1, gripY1, 8, 28, 48, 230);
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           gripX0,
                           gripY0,
                           gripX1,
                           gripY0 + 1.0f,
                           0,
                           240,
                           255,
                           200);
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           gripX0,
                           gripY0,
                           gripX0 + 1.0f,
                           gripY1,
                           0,
                           240,
                           255,
                           200);
    for (int tick = 0; tick < 3; ++tick) {
      float t = static_cast<float>(tick);
      packed = packSolidQuad(batch,
                             kCap,
                             packed,
                             gripX1 - 12.0f + t * 3.0f,
                             gripY1 - 5.0f - t * 3.0f,
                             gripX1 - 4.0f + t * 1.0f,
                             gripY1 - 3.0f - t * 3.0f,
                             0,
                             245,
                             255,
                             220);
    }
  }

  // Command dock (input row) — elevated trough with neon lip
  packed = packVerticalGradientQuad(batch,
                                    kCap,
                                    packed,
                                    panelX0 + 6.0f,
                                    inputTop - 2.0f,
                                    panelX1 - 6.0f,
                                    panelY1 - 5.0f,
                                    8,
                                    16,
                                    28,
                                    250,
                                    4,
                                    10,
                                    18,
                                    252);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 6.0f,
                         inputTop - 2.0f,
                         panelX1 - 6.0f,
                         inputTop - 1.0f,
                         0,
                         220,
                         255,
                         static_cast<unsigned char>(80.0f + pulse * 60.0f));
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 6.0f,
                         inputTop - 1.0f,
                         panelX1 - 6.0f,
                         inputTop,
                         0,
                         180,
                         240,
                         35);
  // Left live-edge bar on the dock
  packed = packVerticalGradientQuad(batch,
                                    kCap,
                                    packed,
                                    panelX0 + 6.0f,
                                    inputTop - 1.0f,
                                    panelX0 + 9.0f,
                                    panelY1 - 5.0f,
                                    0,
                                    255,
                                    255,
                                    255,
                                    0,
                                    160,
                                    230,
                                    200);

  // Build wrapped visual lines once so scrollbar metrics and text agree.
  float historyWidthNoBar = std::max(20.0f, (panelX1 - panelX0) - 32.0f);
  float historyWidthWithBar = std::max(20.0f, historyWidthNoBar - 16.0f);
  struct VisualHistoryLine
  {
    unsigned char r, g, b, a;
    std::string text;
  };
  std::vector<VisualHistoryLine> visualHistory;
  visualHistory.reserve(history.size() * 2);
  std::vector<std::string> wrappedScratch;

  // Two-pass width: assume no scrollbar first; if wrapping overflows the
  // viewport, rebuild with the gutter reserved for the thumb track.
  float wrapWidth = historyWidthNoBar;
  for (int pass = 0; pass < 2; ++pass) {
    visualHistory.clear();
    for (const historyBuffer& item : history) {
      wrapTextToWidth(item.content, wrapWidth, wrappedScratch);
      unsigned char itemColor[4] = { item.r, item.g, item.b, item.a };
      if (item.content.rfind("SUCCESS:", 0) == 0) {
        itemColor[0] = 60;
        itemColor[1] = 255;
        itemColor[2] = 160;
        itemColor[3] = 255;
      } else if (item.content.rfind("ERROR:", 0) == 0) {
        itemColor[0] = 255;
        itemColor[1] = 75;
        itemColor[2] = 95;
        itemColor[3] = 255;
      } else if (item.content.rfind("WARNING:", 0) == 0) {
        itemColor[0] = 255;
        itemColor[1] = 210;
        itemColor[2] = 50;
        itemColor[3] = 255;
      }
      for (const std::string& line : wrappedScratch) {
        VisualHistoryLine visualLine;
        visualLine.r = itemColor[0];
        visualLine.g = itemColor[1];
        visualLine.b = itemColor[2];
        visualLine.a = itemColor[3];
        visualLine.text = line;
        visualHistory.push_back(visualLine);
      }
    }
    bool needsBar = static_cast<int>(visualHistory.size()) > maxHistoryLines;
    if (!needsBar || wrapWidth == historyWidthWithBar) {
      break;
    }
    wrapWidth = historyWidthWithBar;
  }
  bool showScrollbar = static_cast<int>(visualHistory.size()) > maxHistoryLines;

  int totalVisualLines = static_cast<int>(visualHistory.size());
  int maxScroll = totalVisualLines - maxHistoryLines;
  if (maxScroll < 0) {
    maxScroll = 0;
  }
  if (scrollOffset < 0) {
    scrollOffset = 0;
  }
  if (scrollOffset > maxScroll) {
    scrollOffset = maxScroll;
  }

  if (showScrollbar && maxScroll > 0) {
    float trackTop = historyTop + 2.0f;
    float trackBottom = historyBottom - 2.0f;
    float trackHeight = trackBottom - trackTop;
    float scrollbarWidth = 5.0f;
    float scrollbarRightMargin = isFloating ? 14.0f : 12.0f;
    float barX1 = panelX1 - scrollbarWidth - scrollbarRightMargin;
    float barX2 = panelX1 - scrollbarRightMargin;

    // Recessed track + soft glow thumb
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           barX1 - 1.0f,
                           trackTop,
                           barX2 + 1.0f,
                           trackBottom,
                           2,
                           6,
                           12,
                           160);
    packed = packSolidQuad(
      batch, kCap, packed, barX1, trackTop, barX2, trackBottom, 6, 14, 24, 180);

    float thumbHeight =
      trackHeight * (static_cast<float>(maxHistoryLines) /
                     static_cast<float>(std::max(1, totalVisualLines)));
    if (thumbHeight < 22.0f) {
      thumbHeight = 22.0f;
    }
    float scrollPercent =
      (maxScroll > 0)
        ? (static_cast<float>(scrollOffset) / static_cast<float>(maxScroll))
        : 0.0f;
    float thumbTop =
      (trackBottom - thumbHeight) - scrollPercent * (trackHeight - thumbHeight);
    thumbTop =
      std::clamp(thumbTop, trackTop + 2.0f, trackBottom - thumbHeight - 2.0f);
    float thumbBottom = thumbTop + thumbHeight;
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           barX1 - 1.0f,
                           thumbTop,
                           barX2 + 1.0f,
                           thumbBottom,
                           0,
                           200,
                           255,
                           50);
    packed = packVerticalGradientQuad(batch,
                                      kCap,
                                      packed,
                                      barX1,
                                      thumbTop,
                                      barX2,
                                      thumbBottom,
                                      40,
                                      230,
                                      255,
                                      220,
                                      0,
                                      170,
                                      240,
                                      200);
  }

  unsigned char titleColor[4] = { 200, 248, 255, 255 };
  unsigned char promptColor[4] = { 0, 245, 255, 255 };
  unsigned char inputColor[4] = { 235, 248, 255, 255 };

  std::string paramHint = getParameterHint(currentInput);
  std::string statusText;
  unsigned char statusColor[4];

  if (!completionHint.empty()) {
    statusText = completionHint;
    statusColor[0] = 170;
    statusColor[1] = 220;
    statusColor[2] = 255;
    statusColor[3] = 255;
  } else if (!paramHint.empty()) {
    statusText = paramHint;
    statusColor[0] = 255;
    statusColor[1] = 214;
    statusColor[2] = 110;
    statusColor[3] = 255;
  } else {
    statusText =
      isFloating ? "Drag title | Resize corner" : "Double-click title to float";
    statusColor[0] = 120;
    statusColor[1] = 185;
    statusColor[2] = 220;
    statusColor[3] = 255;
  }

  float headerWidth = panelX1 - panelX0;
  std::string titleStr =
    isFloating ? "ILLUMO  //  FLOAT" : "ILLUMO  //  DEV CONSOLE";
  if (headerWidth < 420.0f) {
    titleStr = isFloating ? "ILLUMO FLOAT" : "ILLUMO";
  }
  if (headerWidth < 260.0f) {
    titleStr = "ILLUMO";
  }

  float closeBtnWidth = 0.0f;
  float closeX0 = 0.0f;
  float closeY0 = 0.0f;
  float closeY1 = 0.0f;
  if (isFloating) {
    closeBtnWidth = 26.0f;
    closeX0 = panelX1 - 32.0f;
    closeY0 = panelY0 + 6.0f;
    closeY1 = panelY0 + 26.0f;
  }

  // Title badge background
  std::string visibleTitle =
    truncateTextToWidth(titleStr, std::max(0.0f, headerWidth * 0.45f));
  float titleTextWidth = measureFontText(visibleTitle);
  float titleBadgeX0 = panelX0 + 10.0f;
  float titleBadgeX1 = titleBadgeX0 + titleTextWidth + 18.0f;
  float titleBadgeY0 = panelY0 + 6.0f;
  float titleBadgeY1 = panelY0 + 26.0f;
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         titleBadgeX0,
                         titleBadgeY0,
                         titleBadgeX1,
                         titleBadgeY1,
                         6,
                         22,
                         40,
                         220);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         titleBadgeX0,
                         titleBadgeY0,
                         titleBadgeX0 + 3.0f,
                         titleBadgeY1,
                         0,
                         240,
                         255,
                         255);
  if (!visibleTitle.empty()) {
    packed = packFontLine(batch,
                          kCap,
                          packed,
                          titleBadgeX0 + 8.0f,
                          panelY0 + 9.0f,
                          visibleTitle.c_str(),
                          titleColor);
  }

  if (isFloating) {
    // Close button — soft red plate with neon rim
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           closeX0,
                           closeY0,
                           closeX0 + closeBtnWidth,
                           closeY1,
                           150,
                           36,
                           48,
                           235);
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           closeX0,
                           closeY0,
                           closeX0 + closeBtnWidth,
                           closeY0 + 1.0f,
                           255,
                           120,
                           140,
                           200);
    unsigned char closeTextColor[4] = { 255, 230, 235, 255 };
    packed = packFontLine(
      batch, kCap, packed, closeX0 + 8.0f, closeY0 + 3.0f, "X", closeTextColor);
  }

  // Status chip on the right side of the header (never overlaps the title
  // badge)
  float statusRight = panelX1 - closeBtnWidth - (isFloating ? 14.0f : 12.0f);
  float statusLeft = titleBadgeX1 + 12.0f;
  float statusAvailableWidth = statusRight - statusLeft;
  if (statusAvailableWidth >= 48.0f) {
    std::string truncatedStatus =
      truncateTextToWidth(statusText, statusAvailableWidth - 12.0f);
    if (!truncatedStatus.empty()) {
      float statusTextW = measureFontText(truncatedStatus);
      float statusX0 = statusRight - statusTextW - 12.0f;
      float statusX1 = statusRight;
      packed = packSolidQuad(batch,
                             kCap,
                             packed,
                             statusX0,
                             panelY0 + 7.0f,
                             statusX1,
                             panelY0 + 25.0f,
                             8,
                             20,
                             36,
                             200);
      packed = packFontLine(batch,
                            kCap,
                            packed,
                            statusX0 + 6.0f,
                            panelY0 + 9.0f,
                            truncatedStatus.c_str(),
                            statusColor);
    }
  }

  // History text is drawn after chrome, but before the input row, so its
  // clipping and scroll thumb agree with the available space. Lines are
  // word-wrapped; scrollOffset indexes visual lines from the newest end so
  // scrolling fully to the top always reveals the oldest characters.
  float currentY = historyTop;
  if (totalVisualLines > 0) {
    int endIdx = totalVisualLines - 1 - scrollOffset;
    if (endIdx >= totalVisualLines) {
      endIdx = totalVisualLines - 1;
    }
    if (endIdx >= 0) {
      int startIdx = endIdx - (maxHistoryLines - 1);
      if (startIdx < 0) {
        startIdx = 0;
      }
      // Top-align the visible window, but when fewer lines than the viewport
      // remain at the start of history, still begin at index 0 so nothing is
      // clipped above the history region.
      for (int i = startIdx; i <= endIdx; ++i) {
        const VisualHistoryLine& item = visualHistory[static_cast<size_t>(i)];
        unsigned char itemColor[4] = { item.r, item.g, item.b, item.a };
        if (!item.text.empty() && currentY >= historyTop - 12.0f &&
            currentY + lineSpacing <= historyBottom + 4.0f) {
          packed = packFontLine(batch,
                                kCap,
                                packed,
                                panelX0 + 14.0f,
                                currentY,
                                item.text.c_str(),
                                itemColor);
        }
        currentY += lineSpacing;
      }
    }
  }

  // The input row has a fixed-width text viewport. This keeps a long command
  // editable: the cursor remains on-screen, selection is visible, and the
  // caret is a real rendered bar rather than an appended underscore.
  const float inputTextX = panelX0 + 42.0f;
  const float inputAvailableWidth =
    std::max(48.0f, (panelX1 - panelX0) - 42.0f - 22.0f);
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

  // Prompt badge — compact live chip with cyan edge
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 12.0f,
                         inputY - 4.0f,
                         panelX0 + 36.0f,
                         inputY + 17.0f,
                         10,
                         34,
                         58,
                         240);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 12.0f,
                         inputY - 4.0f,
                         panelX0 + 14.0f,
                         inputY + 17.0f,
                         0,
                         245,
                         255,
                         255);
  packed = packSolidQuad(batch,
                         kCap,
                         packed,
                         panelX0 + 12.0f,
                         inputY - 4.0f,
                         panelX0 + 36.0f,
                         inputY - 3.0f,
                         0,
                         220,
                         255,
                         90);
  packed = packFontLine(
    batch, kCap, packed, panelX0 + 19.0f, inputY, ">", promptColor);

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
                             highlightX1 + 1.0f,
                             inputY + 16.0f,
                             28,
                             110,
                             170,
                             200);
    }
  }
  packed = packFontLine(
    batch, kCap, packed, inputTextX, inputY, visibleInput.c_str(), inputColor);

  // Ghost suggestion — dim cyan continuation of the command
  if (cursorPosition == currentInput.size() &&
      visibleEnd == currentInput.size()) {
    std::string ghostText = getGhostSuggestion();
    if (!ghostText.empty()) {
      float ghostX = inputTextX + measureFontText(visibleInput);
      if (ghostX < inputTextX + inputAvailableWidth) {
        unsigned char ghostColor[4] = { 70, 180, 220, 120 };
        packed = packFontLine(
          batch, kCap, packed, ghostX, inputY, ghostText.c_str(), ghostColor);
      }
    }
  }

  // Breathing laser caret
  bool caretVisible = (caretMilliseconds % 1000) < 580;
  if (caretVisible && cursorPosition >= visibleStart &&
      cursorPosition <= visibleEnd) {
    std::string textBeforeCaret =
      visibleInput.substr(0, cursorPosition - visibleStart);
    float caretX = inputTextX + measureFontText(textBeforeCaret);
    unsigned char glowA = static_cast<unsigned char>(40.0f + breath * 50.0f);
    unsigned char midA = static_cast<unsigned char>(150.0f + pulse * 60.0f);
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           caretX - 3.0f,
                           inputY - 4.0f,
                           caretX + 5.0f,
                           inputY + 17.0f,
                           0,
                           210,
                           255,
                           glowA);
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           caretX - 1.0f,
                           inputY - 3.0f,
                           caretX + 3.0f,
                           inputY + 16.0f,
                           0,
                           240,
                           255,
                           midA);
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           caretX,
                           inputY - 3.0f,
                           caretX + 2.0f,
                           inputY + 16.0f,
                           230,
                           255,
                           255,
                           255);
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           caretX - 2.0f,
                           inputY - 4.0f,
                           caretX + 4.0f,
                           inputY - 2.0f,
                           0,
                           250,
                           255,
                           230);
    packed = packSolidQuad(batch,
                           kCap,
                           packed,
                           caretX - 2.0f,
                           inputY + 15.0f,
                           caretX + 4.0f,
                           inputY + 17.0f,
                           0,
                           250,
                           255,
                           230);
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
    uiVerts.get());
  r->pushDrawIndexed(drawQuads * 6, 0);

  return true;
}
