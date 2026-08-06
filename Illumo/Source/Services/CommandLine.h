#pragma once
#include "CommandRegistry.h"
#include "Drawable.h"
#include "Foundation/MathTypes.h"
#include "IEnvVars.h"
#include "InputContext.h"
#include "Rendering/IRenderWindow.h"
#include <chrono>
#include <cstdint>

#include <string>
#include <unordered_map>
#include <vector>

#define MAX_CHARS_PER_LINE 1024
#define MAX_CMD_HISTORY 256

class Renderer;

// Console UI drawable (token path). No longer inherits SceneObject (D-E4).
class CommandLine : public Drawable<CommandLine>
{
public:
  struct historyBuffer
  {
    unsigned char r, g, b, a;
    std::string content;
  };
  CommandLine(IEnvVars* vars,
              CommandRegistry* commandRegistry,
              IRenderWindow* win,
              Renderer* renderer = nullptr);
  void Toggle();
  void AddCharacter(unsigned int codepoint);
  void HandleBackspace(bool byWord = false);
  void HandleDelete(bool byWord = false);
  void MoveCursorLeft(bool byWord = false, bool select = false);
  void MoveCursorRight(bool byWord = false, bool select = false);
  void MoveCursorHome(bool select = false);
  void MoveCursorEnd(bool select = false);
  void SelectAll();
  void CopySelection();
  void PasteClipboard();
  void CutSelection();
  void ClearInput();
  void Complete();
  void ExecuteCommand();
  void HistoryUp();
  void HistoryDown();
  void AddToHistory(std::string command);
  void ScrollUp();
  void ScrollDown();
  void ToggleFloatingMode();
  void setFloatingMode(bool floating);
  bool isFloatingMode() const { return isFloating; }
  void HandleMousePress(double mouseX, double mouseY, bool isDrag = false);
  void HandleMouseDrag(double mouseX, double mouseY);
  void HandleMouseRelease();
  void HandleScroll(double yOffset);
  void DrawImpl();
  bool AppendCommands(Renderer* renderer) override;
  bool isOpen;
  // True while open or still sliding (avoid dispatch when fully closed).
  bool wantsDraw() const
  {
    return isVisible() && (isOpen || animationProgress > 0.0f);
  }
  void logNormal(const std::string& str);
  void logError(const std::string& str);
  void logWarning(const std::string& str);
  void logSuccess(const std::string& str);
  void logTrace(const std::string& str);
  void AppendStringLn(unsigned char r,
                      unsigned char g,
                      unsigned char b,
                      unsigned char a,
                      std::string str);
  void AppendString(unsigned char r,
                    unsigned char g,
                    unsigned char b,
                    unsigned char a,
                    std::string str);
  std::vector<std::string> ParseCommandArgs(const std::string& text,
                                            const std::string& delim) const;
  std::vector<std::string> SplitCommandChain(const std::string& text) const;
  void SetAlias(const std::string& name, const std::string& expansion);
  void RemoveAlias(const std::string& name);
  bool HasAlias(const std::string& name) const;
  std::string GetAlias(const std::string& name) const;
  const std::unordered_map<std::string, std::string>& GetAliases() const
  {
    return aliases;
  }
  std::string getGhostSuggestion() const;
  const std::string& getCurrentInput() const { return currentInput; }
  const std::string& getCompletionHint() const { return completionHint; }
  std::size_t getCursorPosition() const { return cursorPosition; }
  bool hasSelection() const { return cursorPosition != selectionAnchor; }
  const std::vector<historyBuffer>& getHistory() const { return history; }

private:
  struct ConsoleVertex
  {
    float x, y, z;
    uint8_t color[4];
  };

  std::string currentInput;
  std::string tempInput;
  std::string completionHint;
  std::vector<historyBuffer> history;
  std::vector<std::string> commandHistory;
  std::unordered_map<std::string, std::string> aliases;
  std::size_t cursorPosition;
  std::size_t selectionAnchor;
  int historyIndex;
  int scrollOffset;
  bool consoleInitialized;
  bool isDraggingScrollbar;
  float dragStartY;
  int dragStartScrollOffset;
  bool isFloating;
  float floatingX;
  float floatingY;
  bool isDraggingWindow;
  float dragWindowOffsetX;
  float dragWindowOffsetY;
  std::chrono::high_resolution_clock::time_point lastHeaderClickTime;

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
  // Easy-font glyphs use several quads each, so a full help page needs more
  // than one quad per visible character.
  static const unsigned int kUiQuadCap = 6000;
  static const unsigned int kUiVertCap = kUiQuadCap * 4;
  ConsoleVertex uiVerts[kUiVertCap];

  friend void CellMain(const std::string&);
  void enrollGpuResources();
  void clearCompletionHint();
  void eraseSelection();
  void resetCursorToEnd();
  std::size_t findPreviousWordBoundary() const;
  std::size_t findNextWordBoundary() const;
  std::vector<std::string> getCompletionCandidates(
    const std::string& leadingText) const;
  void ExecuteSingleCommand(const std::string& singleCmd,
                            int expansionDepth = 0);
  std::string getParameterHint(const std::string& inputLine) const;
};
