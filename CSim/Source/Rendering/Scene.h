#pragma once
#include <vector>
#include <unordered_map>
#include "Rendering/Camera.h"
#include "Rendering/Drawable.h"
#include "Rendering/SceneObject.h"
#include <tracy/Tracy.hpp>

class IRenderWindow;

// Scene holds the per-frame drawable contribution list and an optional node graph root.
// Frame clear + submission live in Renderer::RenderScene (token path + hybrid draw).
// EntityTable / ModuleObject scaffolding was unused and archived (D-E3).
class Scene {

public:
	Scene(IRenderWindow* window, Camera* camera)
		: window(window)
		, activeCamera(camera)
	{
		root = new SceneObject(static_cast<ObjectID>(0));
		root->transform = Matrix4(1.0f);
		nodeLookup[0] = root;
	}

	~Scene() = default;

	void AddDrawable(DrawableBase* drawable)
	{
		drawables.push_back(drawable);
	}

	void ClearDrawables()
	{
		drawables.clear();
	}

	// Deprecated for frame submission — kept empty so old call sites are harmless.
	// Prefer Renderer::RenderScene.
	void Update()
	{
	}

	IRenderWindow* window;
	std::unordered_map<ObjectID, SceneObject*> nodeLookup;
	std::vector<DrawableBase*> drawables;
	Camera* activeCamera;
	SceneObject* root;
};
