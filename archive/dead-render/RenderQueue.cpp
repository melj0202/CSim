#include "RenderQueue.h"

int RenderQueue::getDrawableCount() const
{
	return static_cast<int>(drawables.size());
}

DrawableBase* RenderQueue::getDrawable(int index) const
{
	return drawables[index];
}

void RenderQueue::add(DrawableBase* drawable)
{
	drawables.push_back(drawable);
}

void RenderQueue::remove(DrawableBase* drawable)
{
	for (auto it = drawables.begin(); it != drawables.end(); ++it)
	{
		if (*it == drawable)
		{
			drawables.erase(it);
			break;
		}
	}
}

void RenderQueue::draw()
{
	for (auto& drawable : drawables)
	{
		drawable->Draw();
	}
}