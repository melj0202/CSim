#pragma once
#include "Foundation/MacroDefs.h"

class Renderer;

// Scene list entry: token emitter and/or legacy immediate Draw fallback.
// GL object ownership lives in the backend registries (not here).
class DrawableBase {
public:
	virtual ~DrawableBase() = default;

	// Immediate-mode path (legacy fallback when AppendCommands returns false).
	virtual void Draw() = 0;

	// Token path (D-R2). Return true if commands were appended and immediate
	// Draw() should be skipped this frame.
	virtual bool AppendCommands(Renderer* renderer)
	{
		(void)renderer;
		return false;
	}

	bool isVisible() const { return visible; }
	void setVisible(bool v) { visible = v; }

protected:
	bool visible = true;
};

// CRTP helper: virtual Draw -> Derived::DrawImpl
template <typename Derived>
class Drawable : public DrawableBase {
public:
	__CSIM_FORCE_INLINE__ void Draw() override
	{
		if (isVisible())
		{
			static_cast<Derived*>(this)->DrawImpl();
		}
	}
};
