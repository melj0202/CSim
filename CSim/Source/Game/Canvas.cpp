#include "Canvas.h"
#include "IRenderWindow.h"
#include "Logger.h"
#include "Rendering/Camera.h"
#include <GL/glew.h>
#include <array>
#include <cmath>
#include <fstream>
#include <glm/fwd.hpp>
#include <iostream>
#include <string>
#include <string.h>

//Init a new canvas 
void Canvas::initCanvas(const int& width, const int& height)
{


//Initialize canvas data

//Init the canvas
	canvasWidth = width;
	canvasHeight = height;
	fadeSpeed = 8.0f;
	lifeCanvas = new unsigned char[width * height];
	memset(lifeCanvas, 1, width * height);

	//Init the texture representation of the canvas
	Canvas::texCanvasBuffer = new unsigned char[width * height * 3];
	memset(Canvas::texCanvasBuffer, 255, width * height * 3);

	const int rgbCount = width * height * 3;
	displayRgb = new float[rgbCount];
	targetRgb = new float[rgbCount];
	for (int i = 0; i < rgbCount; ++i)
	{
		displayRgb[i] = 1.0f;
		targetRgb[i] = 1.0f;
	}

	//Initialize the OpenGL shader program/pipeline
	//Load in shader files
	float cellSize = 16.0f;
	float worldW = width * cellSize;
	float worldH = height * cellSize;
	std::array<float, 32> vertices = {
		// positions                  // colors           // texture coords
		 worldW,  worldH, 0.0f,       1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // bottom right
		 worldW,  0.0f,   0.0f,       0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // top right
		 0.0f,    0.0f,   0.0f,       0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // top left
		 0.0f,    worldH, 0.0f,       1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // bottom left
	};

	std::array<unsigned char, 32> indices = {
		1, 2, 3,
		0, 1, 3
	};

	std::ifstream fragShaderFile;
	fragShaderFile.open("Shader/triangle_frag.glsl");

	std::ifstream vertexShaderFile;
	vertexShaderFile.open("Shader/triangle_vertex.glsl");

	std::string fragShaderSource;
	std::string vertShaderSource;
	std::string line;

	if (fragShaderFile.is_open())
	{
		while (getline(fragShaderFile, line))
		{
			fragShaderSource.append(line);
			fragShaderSource.append("\n");
		}
	}

	if (vertexShaderFile.is_open())
	{
		while (getline(vertexShaderFile, line))
		{
			vertShaderSource.append(line);
			vertShaderSource.append("\n");
		}
	}

	fragShaderFile.close();
	vertexShaderFile.close();

	{
		std::ofstream log("camera_debug.log", std::ios::app);
		log << "[INIT] Loaded Vertex Shader Source:\n" << vertShaderSource << std::endl;
		log << "[INIT] Loaded Fragment Shader Source:\n" << fragShaderSource << std::endl;
	}

	shaderID = new unsigned int(0);
	//shaderProgramID = new unsigned int(0);
	VAO = new unsigned int(0);
	VBO = new unsigned int(0);
	EBO = new unsigned int(0);
	canvasID = new unsigned int(0);

	shaderProgramID = new unsigned int(createShaderProgram(vertShaderSource.c_str(), fragShaderSource.c_str()));

	//Setup life canvas texture
	glGenTextures(1, canvasID);
	glBindTexture(GL_TEXTURE_2D, *canvasID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, getDimensions()[0], getDimensions()[1], 0, GL_RGB, GL_UNSIGNED_BYTE, &Canvas::lifeCanvas[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	Logger::LogTrace("Canvas texture generated");

	glGenVertexArrays(1, VAO);
	glGenBuffers(1, VBO);
	glGenBuffers(1, EBO);
	Logger::LogTrace("Canvas VAO, VBO, and EBO generated");

	glBindVertexArray(*VAO);

	glBindBuffer(GL_ARRAY_BUFFER, *VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);
	Logger::LogTrace("Canvas VBO buffer data set");

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW);
	Logger::LogTrace("Canvas EBO buffer data set");

	GLint posAttrib = glGetAttribLocation(*shaderProgramID, "aPos");
	GLint texAttrib = glGetAttribLocation(*shaderProgramID, "aTexCoord");
	Logger::LogTrace("Canvas shader program attributes");

	if (posAttrib != -1)
	{
		glVertexAttribPointer(static_cast<GLuint>(posAttrib), 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
		glEnableVertexAttribArray(static_cast<GLuint>(posAttrib));
	}
	else
	{
		Logger::LogWarning("Canvas position attribute not found");
	}


	if (texAttrib != -1)
	{
		glVertexAttribPointer(static_cast<GLuint>(texAttrib), 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (6 * sizeof(float)));
		glEnableVertexAttribArray(static_cast<GLuint>(texAttrib));
	}
	else
	{
		Logger::LogWarning("Canvas texture attribute not found");
	}



	//window->getRenderQueue()->add(this);
	Logger::LogTrace("Canvas initialized");
};

void Canvas::setTargetColor(int cellIndex, unsigned char r, unsigned char g, unsigned char b)
{
	const int base = cellIndex * 3;
	targetRgb[base + 0] = static_cast<float>(r) / 255.0f;
	targetRgb[base + 1] = static_cast<float>(g) / 255.0f;
	targetRgb[base + 2] = static_cast<float>(b) / 255.0f;
}

void Canvas::snapVisualToTargets()
{
	const int rgbCount = canvasWidth * canvasHeight * 3;
	for (int i = 0; i < rgbCount; ++i)
	{
		displayRgb[i] = targetRgb[i];
		const float v = displayRgb[i] * 255.0f + 0.5f;
		texCanvasBuffer[i] = static_cast<unsigned char>(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
	}
}

void Canvas::tickVisual(float dt)
{
	if (dt < 0.0f)
	{
		dt = 0.0f;
	}
	// Exponential ease toward target — looks smoother than linear and is frame-rate friendly.
	// fadeSpeed ~8 => roughly settles in a few tenths of a second.
	float alpha = 1.0f - expf(-fadeSpeed * dt);
	if (alpha > 1.0f)
	{
		alpha = 1.0f;
	}
	if (fadeSpeed <= 0.0f)
	{
		alpha = 1.0f;
	}

	const int rgbCount = canvasWidth * canvasHeight * 3;
	for (int i = 0; i < rgbCount; ++i)
	{
		displayRgb[i] = displayRgb[i] + (targetRgb[i] - displayRgb[i]) * alpha;
		const float v = displayRgb[i] * 255.0f + 0.5f;
		texCanvasBuffer[i] = static_cast<unsigned char>(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
	}
}

void Canvas::DrawImpl()
{
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

	//Draw the canvas
	glBindVertexArray(*VAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *canvasID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, getDimensions()[0], getDimensions()[1], GL_RGB, GL_UNSIGNED_BYTE, &Canvas::texCanvasBuffer[0]);

	glUseProgram(*shaderProgramID);

	std::array<int, 2> dims = window->getWindowDimensions();
	float aspect = static_cast<float>(dims[0]) / static_cast<float>(dims[1]);
	glm::mat4 mvp = camera->GetMVPMatrix(aspect);

	// 2. Upload to uniforms
	GLint mvpLoc = glGetUniformLocation(*shaderProgramID, "uMVP");
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
	glUniform1i(glGetUniformLocation(*shaderProgramID, "ourTexture"), 0);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);

	// Restore state
	glUseProgram(lastProgram);
	glBindVertexArray(lastVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lastVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lastEBO);
	if (blendEnabled)
	{
		glEnable(GL_BLEND);
		glBlendFunc(static_cast<GLenum>(lastBlendSrc), static_cast<GLenum>(lastBlendDst));
	}
	else
	{
		glDisable(GL_BLEND);
	}


}