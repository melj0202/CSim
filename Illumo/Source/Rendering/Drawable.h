#pragma once
#include "Foundation/MacroDefs.h"

class Renderer;

// Scene list entry: token emitter (preferred) and/or legacy immediate Draw.
// GL object ownership lives in the backend registries (not here).
//
// Production pure-token drawables (always AppendCommands → true when visible):
//   Canvas, CommandLine, GLString, SplashText
// Hybrid immediate fallback exists for tests / future stubs only (D-R10).
// CRTP Draw→DrawImpl is leftover for the immediate path; not on the hot token path.
class DrawableBase {
public:
	virtual ~DrawableBase() = default;

	// Immediate-mode path (legacy; used only if AppendCommands returns false).
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

// CRTP helper: virtual Draw -> Derived::DrawImpl (immediate path only).
template <typename Derived>
class Drawable : public DrawableBase {
public:
	__ILLUMO_FORCE_INLINE__ void Draw() override
	{
		if (isVisible())
		{
			static_cast<Derived*>(this)->DrawImpl();
		}
	}
};
