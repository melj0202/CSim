#pragma once 
#include <GL/glew.h>
#include <vector>
#include "Rendering/Camera.h"
#include "Rendering/Drawable.h"
#include "Rendering/SceneObject.h"
#include <unordered_map>
#include "Engine/EntityTable.h"
#include <tracy/Tracy.hpp>

#include "RenderableObject.h"

class IRenderWindow;

class Scene {

    public:
        Scene(IRenderWindow* window, Camera* camera) : window(window), activeCamera(camera) {
            root = new SceneObject(static_cast<ObjectID>(0));
            root->transform = Matrix4(1.0f);
            nodeLookup[0] = root;
        };
        ~Scene() = default;
        
        EntityTable* entityTable;

        void AddDrawable(DrawableBase* drawable) {
            drawables.push_back(drawable);
        }

        void ClearDrawables() {
            drawables.clear();
        }

        void Update() {
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            for (auto* drawable : drawables) {
                if (drawable) {
                    drawable->Draw();
                }
            }
        }

        IRenderWindow* window;
        std::unordered_map<ObjectID, SceneObject*> nodeLookup;
        std::vector<DrawableBase*> drawables;
        Camera* activeCamera;
        SceneObject* root;
        std::vector<RenderableObject> renderableObjects;

};