#include "DebugModule.h"
#include "InputManager.h"
#include "Logger.h"
#include <sstream>

DebugModule::DebugModule()
	: fpsLabel(nullptr)
	, fpsAccum(0.0)
	, fpsFrames(0)
	, fpsDisplay(0)
{
}

DebugModule::~DebugModule()
{
	Exit();
}

void DebugModule::Start(IllumoContext* context)
{
	// D-E5: fail loud if the frozen service bag is incomplete.
	if (!IllumoContextHasDebugCore(context))
	{
		Logger::LogError("DebugModule::Start: IllumoContext missing required services "
			"(envVars, window, renderer, inputManager, commandLine, commandRegistry)");
		ic = context;
		return;
	}
	ic = context;

	// Required for GLString / SplashText screen-space drawing
	GLString::setRenderWindow(ic->window);

	fpsLabel = new GLString("FPS: 0", 80, 255, 120, 255, 18, 12, 12, ic->renderer);
	fpsLabel->setVisible(isShowFpsEnabled());
}

bool DebugModule::isShowFpsEnabled() const
{
	if (!ic || !ic->envVars)
	{
		return false;
	}
	return ic->envVars->getVar("showFPS").valueAsBool;
}

void DebugModule::updateFpsCounter(double dt)
{
	if (!isShowFpsEnabled() || !fpsLabel)
	{
		if (fpsLabel)
		{
			fpsLabel->setVisible(false);
		}
		return;
	}

	fpsLabel->setVisible(true);
	fpsFrames += 1;
	fpsAccum += dt;

	// Refresh displayed FPS about once per second (stable, readable)
	if (fpsAccum >= 1.0)
	{
		fpsDisplay = static_cast<int>(static_cast<double>(fpsFrames) / fpsAccum + 0.5);
		fpsFrames = 0;
		fpsAccum = 0.0;

		std::ostringstream oss;
		oss << "FPS: " << fpsDisplay;
		fpsLabel->setContent(oss.str());
	}
}

void DebugModule::Update(double dt)
{
	ZoneNamed(DebugModuleUpdateZone, "DebugModule Update");

	updateFpsCounter(dt);

	// 1. Process Key Queue from InputManager
	auto& keyQueue = ic->inputManager->getKeyQueue();
	while (!keyQueue.empty())
	{
		auto event = keyQueue.front();
		keyQueue.pop();

		KeyCode key = event.key;
		InputAction action = event.action;

		// Toggle CommandLine with grave accent / tilde key
		if (key == KeyCode::Grave && action == InputAction::Press)
		{
			ic->commandLine->Toggle();
			// Clear character queue when toggling to avoid tilde character being typed
			ic->inputManager->clearCharQueue();
			continue;
		}

		if (ic->commandLine->isOpen)
		{
			if (action == InputAction::Press || action == InputAction::Hold)
			{
				if (key == KeyCode::Backspace)
				{
					ic->commandLine->HandleBackspace();
				}
				else if (key == KeyCode::Enter)
				{
					ic->commandLine->ExecuteCommand();
				}
				else if (key == KeyCode::Escape)
				{
					ic->commandLine->Toggle();
				}
				else if (key == KeyCode::Up)
				{
					ic->commandLine->HistoryUp();
				}
				else if (key == KeyCode::Down)
				{
					ic->commandLine->HistoryDown();
				}
				else if (key == KeyCode::PageUp)
				{
					ic->commandLine->ScrollUp();
				}
				else if (key == KeyCode::PageDown)
				{
					ic->commandLine->ScrollDown();
				}
			}
		}
		else
		{
			// Close window on Escape / Q when console is closed
			if (key == KeyCode::Escape && action == InputAction::Press)
				ic->window->requestClose();
			if (key == KeyCode::Q && action == InputAction::Press)
				ic->window->requestClose();
		}
	}

	// 2. Process Character Queue from InputManager
	auto& charQueue = ic->inputManager->getCharQueue();
	while (!charQueue.empty())
	{
		unsigned int codepoint = charQueue.front();
		charQueue.pop();

		if (ic->commandLine->isOpen)
		{
			if (codepoint != '`' && codepoint != '~')
			{
				ic->commandLine->AddCharacter(codepoint);
			}
		}
	}

	// 3. Execute command queue
	ic->commandRegistry->ExecuteQueue();
}

void DebugModule::Exit()
{
	if (fpsLabel)
	{
		delete fpsLabel;
		fpsLabel = nullptr;
	}
}

void DebugModule::DispatchDrawables(Scene* scene)
{
	// Skip fully closed console (no anim) — avoids chrono/lerp + empty token work.
	if (ic->commandLine && ic->commandLine->wantsDraw())
	{
		scene->AddDrawable(ic->commandLine);
	}
	if (fpsLabel && isShowFpsEnabled())
	{
		scene->AddDrawable(fpsLabel);
	}
}
