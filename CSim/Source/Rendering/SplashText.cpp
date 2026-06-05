#include "SplashText.h"

void SplashText::Wake()
{
	this->setVisible(true);
	this->setA(255);
	startTime = std::chrono::high_resolution_clock::now();
}

void SplashText::Fade()
{
//lerp opacity until 0 then set IsVisible to false;
	auto Now = std::chrono::high_resolution_clock::now();
	auto durationElapsed = Now - startTime;
	if (durationElapsed < wakeDuration)
	{
		float elapsedSeconds = std::chrono::duration<float>(durationElapsed).count();
		float opacity = 255.0f * (1.0f - elapsedSeconds / wakeDuration.count());
		this->setA(static_cast<int>(opacity));
		if (opacity <= 0.0f) this->setVisible(false);
	}
	else
	{
		this->setA(0);
		this->setVisible(false);
	}
}