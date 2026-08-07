#include "IShaderProgram.h"
#include "Renderer.h"

// Built-in style shader sources and enrollment (D-R14). Owned by Renderer;
// production drawables only bind styles and emit content tokens.

static const char* kUiTextVertexShader = R"(
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

static const char* kUiTextFragmentShader = R"(
#version 330 core
in vec4 ourColor;
out vec4 FragColor;
void main() {
    FragColor = ourColor;
}
)";

// Console batch uses absolute pixel positions (no u_position offset).
static const char* kConsoleVertexShader = R"(
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

static const char* kConsoleFragmentShader = R"(
#version 330 core
in vec4 ourColor;
out vec4 FragColor;
void main() {
    FragColor = ourColor;
}
)";

// Shape primitives: pixel space (uUsePixels=1) or world MVP (uUsePixels=0).
static const char* kShapeVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
out vec4 ourColor;
uniform int uUsePixels;
uniform vec2 u_resolution;
uniform mat4 uMVP;
void main() {
    if (uUsePixels != 0) {
        float x = (aPos.x / u_resolution.x) * 2.0 - 1.0;
        float y = 1.0 - (aPos.y / u_resolution.y) * 2.0;
        gl_Position = vec4(x, y, aPos.z, 1.0);
    } else {
        gl_Position = uMVP * vec4(aPos, 1.0);
    }
    ourColor = aColor;
}
)";

static const char* kShapeFragmentShader = R"(
#version 330 core
in vec4 ourColor;
out vec4 FragColor;
void main() {
    FragColor = ourColor;
}
)";

// Sprite primitives: same spaces + texture sample with vertex tint.
static const char* kSpriteVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aUv;
out vec4 ourColor;
out vec2 ourUv;
uniform int uUsePixels;
uniform vec2 u_resolution;
uniform mat4 uMVP;
void main() {
    if (uUsePixels != 0) {
        float x = (aPos.x / u_resolution.x) * 2.0 - 1.0;
        float y = 1.0 - (aPos.y / u_resolution.y) * 2.0;
        gl_Position = vec4(x, y, aPos.z, 1.0);
    } else {
        gl_Position = uMVP * vec4(aPos, 1.0);
    }
    ourColor = aColor;
    ourUv = aUv;
}
)";

static const char* kSpriteFragmentShader = R"(
#version 330 core
in vec4 ourColor;
in vec2 ourUv;
out vec4 FragColor;
uniform sampler2D uTexture;
void main() {
    FragColor = texture(uTexture, ourUv) * ourColor;
}
)";

static void
fillCanvasPipeline(PipelineState& ps)
{
  ps.depthTestEnabled = false;
  ps.blendEnabled = false;
  ps.faceCullingEnabled = false;
  ps.primitives = Primitives::Triangles;
}

static void
fillUiBlendPipeline(PipelineState& ps)
{
  ps.depthTestEnabled = false;
  ps.blendEnabled = true;
  ps.blendSrc = BlendFactor::SrcAlpha;
  ps.blendDst = BlendFactor::OneMinusSrcAlpha;
  ps.faceCullingEnabled = false;
  ps.primitives = Primitives::Triangles;
}

void
Renderer::ensureBuiltinStyles()
{
  if (_builtinStylesReady || !_backend) {
    return;
  }

  // Canvas: file-backed shaders (same paths as historical Canvas enroll).
  {
    RenderStyle& style = _styles[renderStyleIndex(RenderStyleId::Canvas)];
    fillCanvasPipeline(style.pipeline);
    style.shaderHandle = allocateHandle();
    ShaderPaths paths;
    paths.vertexPath = "Shader/canvas_vertex.glsl";
    paths.fragmentPath = "Shader/canvas_frag.glsl";
    enrollShader(paths, style.shaderHandle);
    style.ready = true;
  }

  // UI text (GLString / SplashText / FPS overlay).
  {
    RenderStyle& style = _styles[renderStyleIndex(RenderStyleId::UiText)];
    fillUiBlendPipeline(style.pipeline);
    style.shaderHandle = allocateHandle();
    ShaderSources sources;
    sources.vertexSource = kUiTextVertexShader;
    sources.fragmentSource = kUiTextFragmentShader;
    enrollShader(sources, style.shaderHandle);
    style.ready = true;
  }

  // Console panel batch (CommandLine).
  {
    RenderStyle& style = _styles[renderStyleIndex(RenderStyleId::Console)];
    fillUiBlendPipeline(style.pipeline);
    style.shaderHandle = allocateHandle();
    ShaderSources sources;
    sources.vertexSource = kConsoleVertexShader;
    sources.fragmentSource = kConsoleFragmentShader;
    enrollShader(sources, style.shaderHandle);
    style.ready = true;
  }

  // Shape primitives (GameVisual).
  {
    RenderStyle& style = _styles[renderStyleIndex(RenderStyleId::Shape)];
    fillUiBlendPipeline(style.pipeline);
    style.shaderHandle = allocateHandle();
    ShaderSources sources;
    sources.vertexSource = kShapeVertexShader;
    sources.fragmentSource = kShapeFragmentShader;
    enrollShader(sources, style.shaderHandle);
    style.ready = true;
  }

  // Sprite primitives (GameVisual).
  {
    RenderStyle& style = _styles[renderStyleIndex(RenderStyleId::Sprite)];
    fillUiBlendPipeline(style.pipeline);
    style.shaderHandle = allocateHandle();
    ShaderSources sources;
    sources.vertexSource = kSpriteVertexShader;
    sources.fragmentSource = kSpriteFragmentShader;
    enrollShader(sources, style.shaderHandle);
    style.ready = true;
  }

  _builtinStylesReady = true;
}

const RenderStyle*
Renderer::getStyle(RenderStyleId id) const
{
  const unsigned index = renderStyleIndex(id);
  if (index >= renderStyleCount()) {
    return nullptr;
  }
  const RenderStyle& style = _styles[index];
  if (!style.ready) {
    return nullptr;
  }
  return &style;
}

RenderStyle*
Renderer::getStyle(RenderStyleId id)
{
  ensureBuiltinStyles();
  const unsigned index = renderStyleIndex(id);
  if (index >= renderStyleCount()) {
    return nullptr;
  }
  RenderStyle& style = _styles[index];
  if (!style.ready) {
    return nullptr;
  }
  return &style;
}

bool
Renderer::bindStyle(RenderStyleId id)
{
  ensureBuiltinStyles();
  const RenderStyle* style = getStyle(id);
  if (!style) {
    return false;
  }
  pushPipelineState(style->pipeline);
  pushSetShader(style->shaderHandle);
  return true;
}
