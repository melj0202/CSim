#include "DebugModule.h"

DebugModule::DebugModule()
{}

#include "InputManager.h"

void DebugModule::Start(IllumoContext* context)
{
	ic = context;
}

void DebugModule::Update(double dt)
{
// 1. Process Key Queue from InputManager
	ZoneNamed(DebugModuleUpdateZone, "DebugModule Update");
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
//delete glString;
	glString = nullptr;
}

void DebugModule::DispatchDrawables(Scene* scene)
{
	scene->AddDrawable(ic->commandLine);
}