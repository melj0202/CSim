#pragma once 
#include <vector>
#include "Rendering/Camera2D.h"
#include "Rendering/Drawable.h"
#include "Rendering/SceneObject.h"
#include <unordered_map>
#include "System/EntityTable.h"
#include <tracy/Tracy.hpp>

#include "RenderableObject.h"

class Scene {

    public:
        Scene(IRenderWindow* window, Camera* camera) : window(window), activeCamera(camera) {
            root = new SceneObject(0);
            root->transform = Matrix4();
            nodeLookup[0] = root;
        };
        ~Scene() = default;
        
        EntityTable* entityTable;

        

        Camera* activeCamera;
        SceneObject* root;
        std::vector<RenderableObject> renderableObjects;

};