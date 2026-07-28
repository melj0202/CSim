#include "GLString.h"
#include "thirdparty/stb/stb_easy_font.h"
#include "IRenderWindow.h"
#include "Renderer.h"
#include "IShaderProgram.h"
#include "IMesh.h"
#include "Logger.h"
#include "PipelineState.h"
#include <array>
#include <vector>

namespace {
const char* kUiVertexShader = R"(
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
)";

const char* kUiFragmentShader = R"(
#version 330 core
in vec4 ourColor;
out vec4 FragColor;
void main() {
    FragColor = ourColor;
}
)";
}

GLString::GLString()
	: content("")
	, r(255)
	, g(255)
	, b(255)
	, a(255)
	, size_pt(12.0f)
	, x(0)
	, y(0)
	, renderer(nullptr)
	, meshHandle(0)
	, shaderHandle(0)
	, gpuReady(false)
{
}

GLString::GLString(std::string content, int r, int g, int b, int a, int size_pt, int x, int y, Renderer* renderer)
	: content(content)
	, r(r)
	, g(g)
	, b(b)
	, a(a)
	, size_pt(static_cast<float>(size_pt))
	, x(x)
	, y(y)
	, renderer(renderer)
	, meshHandle(0)
	, shaderHandle(0)
	, gpuReady(false)
{
	enrollGpuResources();
}

GLString::~GLString()
{
	// GPU resources destroyed at backend shutdown (v1).
	gpuReady = false;
}

void GLString::setRenderer(Renderer* rend)
{
	renderer = rend;
	if (!gpuReady && renderer)
	{
		enrollGpuResources();
	}
}

void GLString::enrollGpuResources()
{
	gpuReady = false;
	if (!renderer)
	{
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

	const size_t vboBytes = static_cast<size_t>(maxQuads) * 4 * sizeof(VertexData);
	meshHandle = renderer->allocateHandle();
	renderer->enrollDynamicMesh(
		vboBytes,
		indices.data(),
		indices.size() * sizeof(unsigned int),
		meshHandle,
		MeshVertexLayout::Pos3Color4U8);

	ShaderSources sources;
	sources.vertexSource = kUiVertexShader;
	sources.fragmentSource = kUiFragmentShader;
	shaderHandle = renderer->allocateHandle();
	renderer->enrollShader(sources, shaderHandle);

	gpuReady = true;
	Logger::LogTrace("GLString enrolled (token path)");
}

void GLString::setContent(std::string newContent)
{
	this->content = newContent;
}

void GLString::setR(int newR) { this->r = newR; }
void GLString::setG(int newG) { this->g = newG; }
void GLString::setB(int newB) { this->b = newB; }
void GLString::setA(int newA) { this->a = newA; }
void GLString::setSize(int newSize) { this->size_pt = static_cast<float>(newSize); }
void GLString::setX(int newX) { this->x = newX; }
void GLString::setY(int newY) { this->y = newY; }

std::string GLString::getContent() { return content; }
int GLString::getR() { return r; }
int GLString::getG() { return g; }
int GLString::getB() { return b; }
int GLString::getA() { return a; }
int GLString::getSize() { return static_cast<int>(size_pt); }
int GLString::getX() { return x; }
int GLString::getY() { return y; }

void GLString::DrawImpl()
{
	// Migrated to tokens.
}

bool GLString::AppendCommands(Renderer* rend)
{
	if (!isVisible())
	{
		return true;
	}
	if (content.empty())
	{
		return true;
	}
	if (!gpuReady || !rend)
	{
		return false;
	}
	if (!s_window)
	{
		return true;
	}

	unsigned char color[4] = {
		static_cast<unsigned char>(this->r),
		static_cast<unsigned char>(this->g),
		static_cast<unsigned char>(this->b),
		static_cast<unsigned char>(this->a)
	};
	int numQuads = stb_easy_font_print(
		0.0f,
		0.0f,
		const_cast<char*>(content.c_str()),
		color,
		vertices,
		sizeof(vertices));
	if (numQuads <= 0)
	{
		return true;
	}
	if (numQuads > 2000)
	{
		numQuads = 2000;
	}

	std::array<int, 2> dims = s_window->getWindowDimensions();
	float width = static_cast<float>(dims[0]);
	float height = static_cast<float>(dims[1]);
	float scale = size_pt / 12.0f;

	PipelineState ps;
	ps.depthTestEnabled = false;
	ps.blendEnabled = true;
	ps.blendSrc = BlendFactor::SrcAlpha;
	ps.blendDst = BlendFactor::OneMinusSrcAlpha;
	ps.faceCullingEnabled = false;
	ps.primitives = Primitives::Triangles;
	rend->pushPipelineState(ps);

	rend->pushSetShader(shaderHandle);
	rend->pushSetMesh(meshHandle);

	const unsigned int uploadBytes = static_cast<unsigned int>(numQuads * 4 * sizeof(VertexData));
	rend->pushUpdateBuffer(meshHandle, 0, uploadBytes, vertices);

	rend->pushUniformVec2("u_resolution", width, height);
	rend->pushUniformVec2("u_position", static_cast<float>(x), static_cast<float>(y));
	rend->pushUniformVec2("u_scale", scale, scale);
	rend->pushDrawIndexed(static_cast<unsigned int>(numQuads * 6), 0);

	return true;
}
