#include "CellCommandLine.h"
#include "CellMain.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "thirdparty/stb/stb_easy_font.h"
#include <sstream>
#include <iostream>
#include <cstdint>
#include <vector>

bool CellCommandLine::isOpen = false;
std::string CellCommandLine::currentInput = "";
std::vector<CellCommandLine::historyBuffer> CellCommandLine::history = {
    {240, 240, 240, 255, "CSim Developer Console"},
    {240, 240, 240, 255, "Press ` to toggle, type 'help' for commands"}
};

std::vector<std::string> CellCommandLine::commandHistory;
int CellCommandLine::historyIndex = 0;
int CellCommandLine::scrollOffset = 0;

static unsigned int consoleProgram = 0;
static unsigned int consoleVAO = 0;
static unsigned int consoleVBO = 0;
static unsigned int consoleEBO = 0;
static bool consoleInitialized = false;

struct ConsoleVertex {
    float x, y, z;
    uint8_t color[4];
};

static const char* vertexShaderSource = R"(
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

static const char* fragmentShaderSource = R"(
#version 330 core
in vec4 ourColor;
out vec4 FragColor;
void main() {
    FragColor = ourColor;
}
)";

static unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);
    
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        std::string message(length, ' ');
        glGetShaderInfoLog(id, length, &length, &message[0]);
        std::cerr << "Failed to compile shader (" << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << "): " << message << std::endl;
        glDeleteShader(id);
        return 0;
    }
    return id;
}

static unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    unsigned int program = glCreateProgram();
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    return program;
}

static void initConsoleGL() {
    consoleProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    CellCommandLine::historyIndex = 0;
    glGenVertexArrays(1, &consoleVAO);
    glGenBuffers(1, &consoleVBO);
    glGenBuffers(1, &consoleEBO);
    
    glBindVertexArray(consoleVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, consoleVBO);
    glBufferData(GL_ARRAY_BUFFER, 8000 * sizeof(ConsoleVertex), nullptr, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, consoleEBO);
    unsigned int indices[12000];
    for (int i = 0; i < 2000; ++i) {
        indices[i * 6 + 0] = i * 4 + 0;
        indices[i * 6 + 1] = i * 4 + 1;
        indices[i * 6 + 2] = i * 4 + 2;
        indices[i * 6 + 3] = i * 4 + 2;
        indices[i * 6 + 4] = i * 4 + 3;
        indices[i * 6 + 5] = i * 4 + 0;
    }
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ConsoleVertex), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ConsoleVertex), (void*)12);
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    consoleInitialized = true;
}

void CellCommandLine::Toggle() {
    isOpen = !isOpen;
    if (isOpen) {
        scrollOffset = 0;
    }
}

void CellCommandLine::AddCharacter(unsigned int codepoint) {
    if (currentInput.size() < MAX_CHARS_PER_LINE - 1) {
        if (codepoint >= 32 && codepoint <= 126) {
            currentInput += (char)codepoint;
        }
    }
}

void CellCommandLine::HandleBackspace() {
    if (!currentInput.empty()) {
        currentInput.pop_back();
    }
}

static void logNormal(const std::string& str) {
    CellCommandLine::AppendString(240, 240, 240, 255, str);
}
static void logError(const std::string& str) {
    CellCommandLine::AppendString(255, 100, 100, 255, str);
}
static void logWarning(const std::string& str) {
    CellCommandLine::AppendString(255, 220, 100, 255, str);
}
static void logSuccess(const std::string& str) {
    CellCommandLine::AppendString(100, 255, 100, 255, str);
}

std::vector<std::string> CellCommandLine::ParseCommandArgs(const std::string& text, const std::string& delim){
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = text.find(delim);

    while (end != std::string::npos) {
        result.push_back(text.substr(start, end - start));
        start = end + delim.length();
        end = text.find(delim, start);
    }

    // Don't forget the last token!
    result.push_back(text.substr(start)); 
    return result;
}


void CellCommandLine::ExecuteCommand() {
    if (currentInput.empty()) return;
    
    // Command echoes can be cyan for premium look
    AppendString(100, 200, 255, 255, "> " + currentInput);
    
    std::stringstream ss(currentInput);
    std::string cmd;
	std::vector<std::string> args;
    ss >> cmd;
    while(!ss.eof()) {
        std::string arg;
        ss >> arg;
        args.push_back(arg);
	}

    AddToHistory(currentInput);
    
    if (cmd == "help") {
        logNormal("Available commands:");
        logNormal("  help              - Show this help message");
        logNormal("  ruleset <name>    - Change CA ruleset (e.g. GAME_OF_LIFE, SEEDS, etc.)");
        logNormal("  speed <ms>        - Set simulation delay (1 to 128)");
        logNormal("  clear             - Clear all cells on the canvas");
        logNormal("  save <file>       - Save current state to filename");
        logNormal("  load <file>       - Load state from filename");
        logNormal("  close / quit      - Close console or exit app");
    }
    else if (cmd == "ruleset") {
        std::string newRuleset;
        if (ss >> newRuleset) {
            if (newRuleset == "GAME_OF_LIFE") {
                delete ruleSet;
                ruleSet = new GameOfLifeRuleSet();
                logSuccess("Ruleset set to GAME_OF_LIFE");
            } else if (newRuleset == "BRIANS_BRAIN") {
                delete ruleSet;
                ruleSet = new BrainsBrainRuleSet();
                logSuccess("Ruleset set to BRIANS_BRAIN");
            } else if (newRuleset == "DAY_AND_NIGHT") {
                delete ruleSet;
                ruleSet = new DayAndNightRuleSet();
                logSuccess("Ruleset set to DAY_AND_NIGHT");
            } else if (newRuleset == "HIGHLIFE") {
                delete ruleSet;
                ruleSet = new HighlifeRuleSet();
                logSuccess("Ruleset set to HIGHLIFE");
            } else if (newRuleset == "LIFE_WITHOUT_DEATH") {
                delete ruleSet;
                ruleSet = new LifeWithoutDeathRuleSet();
                logSuccess("Ruleset set to LIFE_WITHOUT_DEATH");
            } else if (newRuleset == "SEEDS") {
                delete ruleSet;
                ruleSet = new SeedsRuleSet();
                logSuccess("Ruleset set to SEEDS");
            } else {
                logError("Error: Unknown ruleset '" + newRuleset + "'");
            }
        } else {
            logNormal("Usage: ruleset <name>");
        }
    }
    else if (cmd == "speed") {
        int val;
        if (ss >> val) {
            if (val >= speedFactorMin && val <= speedFactorMax) {
                speedFactor = val;
                logSuccess("Speed set to " + std::to_string(speedFactor));
            } else {
                logError("Error: Speed must be between " + std::to_string(speedFactorMin) + " and " + std::to_string(speedFactorMax));
            }
        } else {
            logNormal("Usage: speed <value>");
        }
    }
    else if (cmd == "clear") {
        if (CellCanvas::lifeCanvas) {
            memset(CellCanvas::lifeCanvas, 1, CellCanvas::canvasWidth * CellCanvas::canvasHeight);
            logSuccess("Canvas cleared.");
        }
    }
    else if (cmd == "save") {
        std::string filename;
        if (ss >> filename) {
            canvasState.save->iterate(ruleSet, filename.c_str(), currentState);
            logSuccess("Canvas saved to: " + filename);
        } else {
            logNormal("Usage: save <filename>");
        }
    }
    else if (cmd == "load") {
        std::string filename;
        if (ss >> filename) {
            CellState* res = canvasState.load->iterate(ruleSet, filename.c_str(), currentState);
            if (res) {
                logSuccess("Canvas loaded from: " + filename);
            } else {
                logError("Failed to load: " + filename);
            }
        } else {
            logNormal("Usage: load <filename>");
        }
    }
    else if (cmd == "close") {
        isOpen = false;
    }
    else if (cmd == "quit") {
        glfwSetWindowShouldClose(RenderWindow::getWindowInstance(), GLFW_TRUE);
    }
    else if (cmd == "test") {
        logWarning("WARNING: test");
    }
    else {
        EnvVar tmp = ServiceLocator::get<EnvVars>("EnvVars")->getVar(cmd);
        std::vector<std::string> args = ParseCommandArgs(cmd, " ");
        if (!tmp.value.empty()) {
            if(args.size() == 1) {
                logNormal(cmd + " = " + tmp.value);
            } else if(args.size() == 2) {
                ServiceLocator::get<EnvVars>("EnvVars")->setVar(args[0], args[1]);
            }
        } 
        else {
            if(args.size() == 2) {
                ServiceLocator::get<EnvVars>("EnvVars")->setVar(args[0], args[1]);
            }
            logError("Unknown command/var: " + cmd);
        }
    }
    
    currentInput.clear();
    scrollOffset = 0;
}

void CellCommandLine::AddToHistory(std::string command) {
    commandHistory.push_back(command);
    //imitate stack beahavior by setting historyIndex to top of stack and popping the first entry when full.
    if (commandHistory.size() > MAX_CMD_HISTORY) {
        commandHistory.erase(commandHistory.begin());
    }
    historyIndex = commandHistory.size() - 1;
}

void CellCommandLine::HistoryDown() {
    if (historyIndex <= commandHistory.size() - 1 && !commandHistory.empty()) {
        historyIndex++;
        currentInput = commandHistory[historyIndex];
    }
}

void CellCommandLine::HistoryUp() {
    if (historyIndex > 0  && !commandHistory.empty()) {
        historyIndex--;
        currentInput = commandHistory[historyIndex];
    }
}

void CellCommandLine::ScrollUp() {
    auto dims = RenderWindow::getWindowDimensions();
    float height = (float)dims[1];
    float panelHeight = height * 0.45f;
    if (panelHeight < 200.0f) panelHeight = 200.0f;
    float lineSpacing = 24.0f;
    int maxHistoryLines = (int)((panelHeight - 40.0f) / lineSpacing);
    if (maxHistoryLines < 1) maxHistoryLines = 1;

    int maxScroll = (int)history.size() - maxHistoryLines;
    if (maxScroll < 0) maxScroll = 0;

    if (scrollOffset < maxScroll) {
        scrollOffset++;
    }
}

void CellCommandLine::ScrollDown() {
    if (scrollOffset > 0) {
        scrollOffset--;
    }
}

void CellCommandLine::AppendString(unsigned char r, unsigned char g, unsigned char b, unsigned char a, std::string str) {
    history.push_back({r, g, b, a, str});
    if (history.size() > MAX_CMD_HISTORY) {
        history.erase(history.begin());
    }
}

void CellCommandLine::Draw() {
    static float animationProgress = 0.0f;
    static double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    float deltaTime = (float)(currentTime - lastTime);
    lastTime = currentTime;
    if (deltaTime > 0.1f) deltaTime = 0.1f;

    // Update animation progress (slide duration: ~0.15s at speed 6.5f)
    float animationSpeed = 6.5f;
    if (isOpen) {
        if (animationProgress < 1.0f) {
            animationProgress += deltaTime * animationSpeed;
            if (animationProgress > 1.0f) animationProgress = 1.0f;
        }
    } else {
        if (animationProgress > 0.0f) {
            animationProgress -= deltaTime * animationSpeed;
            if (animationProgress < 0.0f) animationProgress = 0.0f;
        }
    }

    if (animationProgress <= 0.0f) return;
    
    auto dims = RenderWindow::getWindowDimensions();
    float width = (float)dims[0];
    float height = (float)dims[1];
    
    if (!consoleInitialized) {
        initConsoleGL();
    }
    
    // Save state
    int lastProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &lastProgram);
    int lastVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &lastVAO);
    int lastVBO;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &lastVBO);
    int lastEBO;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &lastEBO);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    int lastBlendSrc, lastBlendDst;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &lastBlendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &lastBlendDst);
    
    // ADD THIS: Save and disable depth testing for UI rendering
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(consoleProgram);
    glBindVertexArray(consoleVAO);
    
    // Set resolution uniform
    glUniform2f(glGetUniformLocation(consoleProgram, "u_resolution"), width, height);
    
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
    
    glBindBuffer(GL_ARRAY_BUFFER, consoleVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(ConsoleVertex), panelVertices);
    
    glUniform2f(glGetUniformLocation(consoleProgram, "u_scale"), 1.0f, 1.0f);
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
    glUniform2f(glGetUniformLocation(consoleProgram, "u_scale"), 1.0f, 1.0f);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    float lineSpacing = 24.0f; 
    int maxHistoryLines = (int)((panelHeight - 30.0f) / lineSpacing);
    if (maxHistoryLines < 1) maxHistoryLines = 1;

    // 2.5 Draw Scroll Bar (if history overflows the viewport)
    int totalLines = (int)history.size();
    if (totalLines > maxHistoryLines) {
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
        float thumbHeight = trackHeight * ((float)maxHistoryLines / (float)totalLines);
        if (thumbHeight < 15.0f) thumbHeight = 15.0f;
        
        int maxScroll = totalLines - maxHistoryLines;
        float scrollPercent = (float)scrollOffset / (float)maxScroll;
        
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
    unsigned char textColor[4] = { 240, 240, 240, 255 };
    unsigned char inputColor[4] = { 100, 200, 255, 255 };

    float currentY = (yOffset + 6.0f) / 2.0f; 
    float yOffsetStep = 12.0f; // 12px * 2.0 scale = 24px step size on-screen
    
    glUniform2f(glGetUniformLocation(consoleProgram, "u_scale"), 2.0f, 2.0f);
    
    int endIdx = (int)history.size() - 1 - scrollOffset;
    if (endIdx >= 0) {
        int startIdx = endIdx - (maxHistoryLines - 1);
        if (startIdx < 0) startIdx = 0;
        for (int i = startIdx; i <= endIdx; ++i) {
            const auto& item = history[i];
            unsigned char itemColor[4] = { item.r, item.g, item.b, item.a };
            int numQuads = stb_easy_font_print(10.0f / 2.0f, currentY, (char*)item.content.c_str(), itemColor, textQuads, sizeof(textQuads));
            
            if (numQuads > 0) {
                if (numQuads > 1990) numQuads = 1990;
                glBufferSubData(GL_ARRAY_BUFFER, 0, numQuads * 4 * sizeof(ConsoleVertex), textQuads);
                glDrawElements(GL_TRIANGLES, numQuads * 6, GL_UNSIGNED_INT, nullptr);
            }
            currentY += yOffsetStep;
        }
    }

    // 4. Draw Input Line at the bottom
    float inputY = (yOffset + panelHeight - 25.0f) / 2.0f;
    std::string inputStr = "> " + currentInput + "_";
    int numQuads = stb_easy_font_print(10.0f / 2.0f, inputY, (char*)inputStr.c_str(), inputColor, textQuads, sizeof(textQuads));
    if (numQuads > 0) {
        if (numQuads > 1990) numQuads = 1990;
        glBufferSubData(GL_ARRAY_BUFFER, 0, numQuads * 4 * sizeof(ConsoleVertex), textQuads);
        glDrawElements(GL_TRIANGLES, numQuads * 6, GL_UNSIGNED_INT, nullptr);
    }
    
    // Restore state
    glUseProgram(lastProgram);
    glBindVertexArray(lastVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lastVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lastEBO);
    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    }
    if (!blendEnabled) {
        glDisable(GL_BLEND);
    } else {
        glBlendFunc(lastBlendSrc, lastBlendDst);
    }
}