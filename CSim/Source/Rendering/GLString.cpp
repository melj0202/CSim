#include "GLString.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "thirdparty/stb/stb_easy_font.h"
#include "IRenderWindow.h"
#include <iostream>
#include <vector>
#include "Logger.h"

GLString::GLString()
	: content(""), r(255), g(255), b(255), a(255), size_pt(12.0f), x(0), y(0)
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
uniform vec2 u_position;
void main() {
    vec2 pos = aPos.xy * u_scale + u_position;
    float x = (pos.x / u_resolution.x) * 2.0 - 1.0;
    float y = 1.0 - (pos.y / u_resolution.y) * 2.0;
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

	initGL();
	Logger::LogTrace("GLString initialized");
}

GLString::GLString(std::string content, int r, int g, int b, int a, int size_pt, int x, int y)
	: content(content), r(r), g(g), b(b), a(a), size_pt(static_cast<float>(size_pt)), x(x), y(y)
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
uniform vec2 u_position;
void main() {
    vec2 pos = aPos.xy * u_scale + u_position;
    float x = (pos.x / u_resolution.x) * 2.0 - 1.0;
    float y = 1.0 - (pos.y / u_resolution.y) * 2.0;
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


	initGL();
	Logger::LogTrace("GLString initialized");
}

GLString::~GLString()
{
	if (VAO != 0) glDeleteVertexArrays(1, VAO);
	if (VBO != 0) glDeleteBuffers(1, VBO);
	if (EBO != 0) glDeleteBuffers(1, EBO);
	glDeleteProgram(*shaderProgramID);
	Logger::LogTrace("GLString destroyed");
}

void GLString::initGL()
{
	glGenVertexArrays(1, VAO);
	glGenBuffers(1, VBO);
	glGenBuffers(1, EBO);
	glBindVertexArray(*VAO);

	glBindBuffer(GL_ARRAY_BUFFER, *VBO);
	glBufferData(GL_ARRAY_BUFFER, 2000 * 4 * sizeof(VertexData), nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
	std::vector<unsigned int> indices(2000 * 6);
	for (int i = 0; i < 2000; ++i)
	{
		indices[i * 6 + 0] = i * 4 + 0;
		indices[i * 6 + 1] = i * 4 + 1;
		indices[i * 6 + 2] = i * 4 + 2;
		indices[i * 6 + 3] = i * 4 + 2;
		indices[i * 6 + 4] = i * 4 + 3;
		indices[i * 6 + 5] = i * 4 + 0;
	}
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*) 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), (void*) 12);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	Logger::LogTrace("GLString added to render queue");
}

void GLString::setContent(std::string newContent)
{
	this->content = newContent;
}

void GLString::setR(int newR)
{
	this->r = newR;
}

void GLString::setG(int newG)
{
	this->g = newG;
}

void GLString::setB(int newB)
{
	this->b = newB;
}

void GLString::setA(int newA)
{
	this->a = newA;
}

void GLString::setSize(int newSize)
{
	this->size_pt = static_cast<float>(newSize);
}

void GLString::setX(int newX)
{
	this->x = newX;
}

void GLString::setY(int newY)
{
	this->y = newY;
}

std::string GLString::getContent()
{
	return content;
}

int GLString::getR()
{
	return r;
}

int GLString::getG()
{
	return g;
}

int GLString::getB()
{
	return b;
}

int GLString::getA()
{
	return a;
}

int GLString::getSize()
{
	return static_cast<int>(size_pt);
}

int GLString::getX()
{
	return x;
}

int GLString::getY()
{
	return y;
}

void GLString::DrawImpl()
{
	if (content.empty()) return;

	// Save OpenGL states
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
	GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(*shaderProgramID);
	glBindVertexArray(*VAO);

	// Get window dimensions
	auto dims = s_window->getWindowDimensions();
	float width = (float) dims[0];
	float height = (float) dims[1];

	glUniform2f(glGetUniformLocation(*shaderProgramID, "u_resolution"), width, height);
	glUniform2f(glGetUniformLocation(*shaderProgramID, "u_position"), (float) x, (float) y);

	// Font scale (default size is 12)
	float scale = size_pt / 12.0f;
	glUniform2f(glGetUniformLocation(*shaderProgramID, "u_scale"), scale, scale);

	// Generate vertices using stb_easy_font_print
	unsigned char color[4] = {(unsigned char) r, (unsigned char) g, (unsigned char) b, (unsigned char) a};
	int numQuads = stb_easy_font_print(0.0f, 0.0f, (char*) content.c_str(), color, vertices, sizeof(vertices));
	if (numQuads > 0)
	{
		if (numQuads > 2000) numQuads = 2000;

		glBindBuffer(GL_ARRAY_BUFFER, *VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, numQuads * 4 * sizeof(VertexData), vertices);

		glDrawElements(GL_TRIANGLES, numQuads * 6, GL_UNSIGNED_INT, nullptr);
	}

	// Restore states
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
