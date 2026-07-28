// CommandLine + GLString token emission tests (MockBackend, no real GL window).

#include "Tests/TestHelpers.h"
#include "Tests/TestHarness.h"
#include "Services/CommandLine.h"
#include "Services/CommandRegistry.h"
#include "Rendering/GLString.h"
#include "Rendering/Scene.h"
#include "Rendering/RenderCommand.h"
#include "Rendering/PipelineState.h"
#include <chrono>
#include <thread>
#include <cstdio>

static TestCounters g;

static void testCommandLineClosedEmitsNoDraws()
{
	testSection("CommandLine: closed emits no UI draws");
	NullRenderWindow window(1280, 720);
	EnvVars env;
	env.setVar("WinX", 1280);
	env.setVar("WinY", 720);
	Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
	MockBackend mock;
	mock.Initialize();
	Renderer renderer(&window, &env, &camera, &mock, false);
	CommandRegistry registry;
	CommandLine console(&env, &registry, &window, &renderer);

	testTrue(g, !console.isOpen, "starts closed");

	Scene scene(&window, &camera);
	scene.AddDrawable(&console);

	renderer.BeginFrame();
	renderer.RenderScene(&scene, &camera);
	renderer.EndFrame();

	// Frame setup only (viewport / pipeline / clear) — no console geometry.
	testEqSize(g, mock.countNonEmptyOfType(CommandType::UpdateBuffer), 0u,
		"closed: no UpdateBuffer");
	testEqSize(g, mock.countNonEmptyOfType(CommandType::DrawIndexed), 0u,
		"closed: no DrawIndexed");
	testEqSize(g, mock.countNonEmptyOfType(CommandType::ClearScreen), 1u,
		"closed: still clears frame");
}

static void testCommandLineOpenEmitsPanelTokens()
{
	testSection("CommandLine: open emits panel/update/draw tokens");
	NullRenderWindow window(1280, 720);
	EnvVars env;
	env.setVar("WinX", 1280);
	env.setVar("WinY", 720);
	Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
	MockBackend mock;
	mock.Initialize();
	Renderer renderer(&window, &env, &camera, &mock, false);
	CommandRegistry registry;
	CommandLine console(&env, &registry, &window, &renderer);

	// Enroll happened in ctor
	testTrue(g, mock.getCreateCount() >= 2u, "console enrolled mesh+shader");

	// Advance open animation without recording those pumps on the mock frame
	// we care about — use a private pump that does not submit.
	console.Toggle();
	testTrue(g, console.isOpen, "toggled open");
	std::this_thread::sleep_for(std::chrono::milliseconds(40));
	// Call AppendCommands once to advance progress (tokens go into renderer queue;
	// we clear via BeginFrame next).
	console.AppendCommands(&renderer);

	// Full scene frame
	Scene scene(&window, &camera);
	scene.AddDrawable(&console);

	mock.resetCounters();
	// Keep creates from enroll; resetCounters wipes creates — re-check only submit.
	// Actually resetCounters clears creates; enroll already done. Fine for token asserts.

	renderer.BeginFrame();
	// Another small sleep so this frame's dt is non-zero while open
	std::this_thread::sleep_for(std::chrono::milliseconds(20));
	renderer.RenderScene(&scene, &camera);
	renderer.EndFrame();

	testTrue(g, mock.getLastNonEmptySubmittedCount() > 0, "open frame non-empty");
	// P2: entire console is one UpdateBuffer + one DrawIndexed
	testEqSize(g, mock.countNonEmptyOfType(CommandType::UpdateBuffer), 1u,
		"open: single batched UpdateBuffer");
	testEqSize(g, mock.countNonEmptyOfType(CommandType::DrawIndexed), 1u,
		"open: single batched DrawIndexed");
	testTrue(g, mock.countNonEmptyOfType(CommandType::SetShader) >= 1u,
		"open: SetShader");
	testTrue(g, mock.countNonEmptyOfType(CommandType::SetMesh) >= 1u,
		"open: SetMesh");
	testTrue(g, mock.countNonEmptyOfType(CommandType::SetUniformVec2) >= 1u,
		"open: resolution/scale uniforms");

	// Blend enabled for console UI
	bool foundBlend = false;
	for (size_t i = 0; i < mock.getLastNonEmptySubmittedCount(); ++i)
	{
		const RenderCommand& cmd = mock.getLastNonEmptySubmitted(i);
		if (cmd.commandType == CommandType::SetPipelineState && cmd.pipelineState.blendEnabled)
		{
			foundBlend = true;
			break;
		}
	}
	testTrue(g, foundBlend, "open: pipeline enables blend");
}

static void testCommandLineInvisibleSkipsTokens()
{
	testSection("CommandLine: invisible skips tokens even if open");
	NullRenderWindow window(800, 600);
	EnvVars env;
	env.setVar("WinX", 800);
	env.setVar("WinY", 600);
	Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
	MockBackend mock;
	mock.Initialize();
	Renderer renderer(&window, &env, &camera, &mock, false);
	CommandRegistry registry;
	CommandLine console(&env, &registry, &window, &renderer);

	console.Toggle();
	std::this_thread::sleep_for(std::chrono::milliseconds(40));
	console.AppendCommands(&renderer); // advance animation
	console.setVisible(false);

	Scene scene(&window, &camera);
	scene.AddDrawable(&console);
	mock.resetCounters();

	renderer.BeginFrame();
	renderer.RenderScene(&scene, &camera);
	renderer.EndFrame();

	testEqSize(g, mock.countNonEmptyOfType(CommandType::UpdateBuffer), 0u,
		"invisible: no UpdateBuffer");
	testEqSize(g, mock.countNonEmptyOfType(CommandType::DrawIndexed), 0u,
		"invisible: no DrawIndexed");
}

static void testCommandLineHistoryScrollTokens()
{
	testSection("CommandLine: many history lines still emit text draws");
	NullRenderWindow window(1280, 720);
	EnvVars env;
	env.setVar("WinX", 1280);
	env.setVar("WinY", 720);
	Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
	MockBackend mock;
	mock.Initialize();
	Renderer renderer(&window, &env, &camera, &mock, false);
	CommandRegistry registry;
	CommandLine console(&env, &registry, &window, &renderer);

	for (int i = 0; i < 40; ++i)
	{
		console.logNormal("history line");
	}

	console.Toggle();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	// Warm animation fully toward open with several appends (not via scene)
	for (int i = 0; i < 15; ++i)
	{
		console.AppendCommands(&renderer);
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	Scene scene(&window, &camera);
	scene.AddDrawable(&console);
	mock.resetCounters();

	renderer.BeginFrame();
	renderer.RenderScene(&scene, &camera);
	renderer.EndFrame();

	// P2: still a single batch even with many history lines
	testEqSize(g, mock.countNonEmptyOfType(CommandType::UpdateBuffer), 1u,
		"history: one UpdateBuffer batch");
	testEqSize(g, mock.countNonEmptyOfType(CommandType::DrawIndexed), 1u,
		"history: one DrawIndexed batch");
	// Batched draw should cover many quads (panel + sep + lines + input)
	bool bigDraw = false;
	for (size_t i = 0; i < mock.getLastNonEmptySubmittedCount(); ++i)
	{
		const RenderCommand& cmd = mock.getLastNonEmptySubmitted(i);
		if (cmd.commandType == CommandType::DrawIndexed && cmd.drawIndexed.elementCount > 12)
		{
			bigDraw = true;
		}
	}
	testTrue(g, bigDraw, "history: DrawIndexed covers more than chrome alone");
}

static void testGLStringEmptyAndInvisible()
{
	testSection("GLString: empty / invisible emit no draws");
	NullRenderWindow window(640, 480);
	EnvVars env;
	env.setVar("WinX", 640);
	env.setVar("WinY", 480);
	Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
	MockBackend mock;
	mock.Initialize();
	Renderer renderer(&window, &env, &camera, &mock, false);
	GLString::setRenderWindow(&window);

	GLString emptyLabel("FPS: 0", 80, 255, 120, 255, 18, 12, 12, &renderer);
	emptyLabel.setContent("");
	Scene scene(&window, &camera);
	scene.AddDrawable(&emptyLabel);

	renderer.BeginFrame();
	renderer.RenderScene(&scene, &camera);
	renderer.EndFrame();
	testEqSize(g, mock.countNonEmptyOfType(CommandType::DrawIndexed), 0u,
		"empty content: no DrawIndexed");

	mock.resetCounters();
	emptyLabel.setContent("Hello");
	emptyLabel.setVisible(false);
	renderer.BeginFrame();
	renderer.RenderScene(&scene, &camera);
	renderer.EndFrame();
	testEqSize(g, mock.countNonEmptyOfType(CommandType::DrawIndexed), 0u,
		"invisible: no DrawIndexed");
}

static void testGLStringEmitsTextTokens()
{
	testSection("GLString: content emits UpdateBuffer + DrawIndexed");
	NullRenderWindow window(640, 480);
	EnvVars env;
	env.setVar("WinX", 640);
	env.setVar("WinY", 480);
	Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
	MockBackend mock;
	mock.Initialize();
	Renderer renderer(&window, &env, &camera, &mock, false);
	GLString::setRenderWindow(&window);

	GLString label("FPS: 60", 80, 255, 120, 255, 18, 12, 12, &renderer);
	testTrue(g, mock.getCreateCount() >= 2u, "GLString enrolled mesh+shader");

	Scene scene(&window, &camera);
	scene.AddDrawable(&label);

	mock.resetCounters();
	renderer.BeginFrame();
	renderer.RenderScene(&scene, &camera);
	renderer.EndFrame();

	testTrue(g, mock.getLastNonEmptySubmittedCount() > 0, "GLString frame non-empty");
	testEqSize(g, mock.countNonEmptyOfType(CommandType::UpdateBuffer), 1u,
		"GLString one UpdateBuffer");
	testEqSize(g, mock.countNonEmptyOfType(CommandType::DrawIndexed), 1u,
		"GLString one DrawIndexed");
	testEqSize(g, mock.countNonEmptyOfType(CommandType::SetShader), 1u,
		"GLString SetShader");
	testEqSize(g, mock.countNonEmptyOfType(CommandType::SetMesh), 1u,
		"GLString SetMesh");

	// Uniforms: resolution, position, scale
	testTrue(g, mock.countNonEmptyOfType(CommandType::SetUniformVec2) >= 3u,
		"GLString at least 3 vec2 uniforms");

	bool foundBlend = false;
	bool foundDrawElems = false;
	for (size_t i = 0; i < mock.getLastNonEmptySubmittedCount(); ++i)
	{
		const RenderCommand& cmd = mock.getLastNonEmptySubmitted(i);
		if (cmd.commandType == CommandType::SetPipelineState && cmd.pipelineState.blendEnabled)
		{
			foundBlend = true;
		}
		if (cmd.commandType == CommandType::DrawIndexed)
		{
			foundDrawElems = cmd.drawIndexed.elementCount > 0;
		}
	}
	testTrue(g, foundBlend, "GLString enables blend");
	testTrue(g, foundDrawElems, "GLString DrawIndexed elementCount > 0");
}

static void testGLStringAndCommandLineTogether()
{
	testSection("Scene: Canvas-order console + FPS tokens");
	NullRenderWindow window(1280, 720);
	EnvVars env;
	env.setVar("WinX", 1280);
	env.setVar("WinY", 720);
	Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
	MockBackend mock;
	mock.Initialize();
	Renderer renderer(&window, &env, &camera, &mock, false);
	GLString::setRenderWindow(&window);
	CommandRegistry registry;

	CommandLine console(&env, &registry, &window, &renderer);
	GLString fps("FPS: 99", 80, 255, 120, 255, 18, 12, 12, &renderer);

	console.Toggle();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	for (int i = 0; i < 12; ++i)
	{
		console.AppendCommands(&renderer);
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	Scene scene(&window, &camera);
	scene.AddDrawable(&console);
	scene.AddDrawable(&fps);

	mock.resetCounters();
	renderer.BeginFrame();
	renderer.RenderScene(&scene, &camera);
	renderer.EndFrame();

	// Console batch (1 draw) + FPS (1 draw)
	testTrue(g, mock.countNonEmptyOfType(CommandType::DrawIndexed) >= 2u,
		"console+FPS: at least two DrawIndexed");
	testTrue(g, mock.countNonEmptyOfType(CommandType::UpdateBuffer) >= 2u,
		"console+FPS: at least two UpdateBuffer (console + FPS first frame)");
	testEqSize(g, mock.countNonEmptyOfType(CommandType::ClearScreen), 1u,
		"single frame clear");
}

static void testGLStringCachesGeometry()
{
	testSection("GLString: second frame skips UpdateBuffer if content unchanged");
	NullRenderWindow window(640, 480);
	EnvVars env;
	env.setVar("WinX", 640);
	env.setVar("WinY", 480);
	Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
	MockBackend mock;
	mock.Initialize();
	Renderer renderer(&window, &env, &camera, &mock, false);
	GLString::setRenderWindow(&window);

	GLString label("FPS: 12", 80, 255, 120, 255, 18, 12, 12, &renderer);
	Scene scene(&window, &camera);
	scene.AddDrawable(&label);

	renderer.BeginFrame();
	renderer.RenderScene(&scene, &camera);
	renderer.EndFrame();
	testEqSize(g, mock.countNonEmptyOfType(CommandType::UpdateBuffer), 1u,
		"first frame uploads geometry");

	mock.resetCounters();
	renderer.BeginFrame();
	renderer.RenderScene(&scene, &camera);
	renderer.EndFrame();
	testEqSize(g, mock.countNonEmptyOfType(CommandType::UpdateBuffer), 0u,
		"second frame reuses cached VBO");
	testEqSize(g, mock.countNonEmptyOfType(CommandType::DrawIndexed), 1u,
		"second frame still draws");

	label.setContent("FPS: 99");
	mock.resetCounters();
	renderer.BeginFrame();
	renderer.RenderScene(&scene, &camera);
	renderer.EndFrame();
	testEqSize(g, mock.countNonEmptyOfType(CommandType::UpdateBuffer), 1u,
		"content change re-uploads");
}

int runUITokenTests()
{
	g.failures = 0;
	std::printf("\n======== UI token tests (CommandLine + GLString) ========\n");
	testCommandLineClosedEmitsNoDraws();
	testCommandLineOpenEmitsPanelTokens();
	testCommandLineInvisibleSkipsTokens();
	testCommandLineHistoryScrollTokens();
	testGLStringEmptyAndInvisible();
	testGLStringEmitsTextTokens();
	testGLStringCachesGeometry();
	testGLStringAndCommandLineTogether();
	std::printf("======== UI token done (%d failure(s)) ========\n", g.failures);
	return g.failures;
}
