#pragma once 
#include "Rendering/IMesh.h"
#include "Rendering/IShaderProgram.h"
#include "Rendering/ITexture.h"
#include "Util/Math.h"

enum DirtyFlags : uint8_t {
    None = 0,
    Transform = 1 << 0,
    Render = 1 << 1
};

using ObjectID = uint32_t;

struct Transform {
    Vec3 position;
    Vec3 scale;
    Quaternion rotation;

    Matrix4 toMatrix() {
        Matrix4 T = glm::translate(position);
        Matrix4 R = glm::mat4_cast(rotation);
        Matrix4 S = glm::scale(scale);
        return T * R * S;
    }
};

struct ModuleObject {

    
    unsigned long meshID;
    unsigned long textureID;
    Transform transform;    
    
    ModuleObject(unsigned long meshID, unsigned long textureID, Transform transform) : meshID(meshID), textureID(textureID), transform(transform) {}
    ModuleObject(unsigned long meshID, unsigned long textureID) : meshID(meshID), textureID(textureID), transform(Transform()) {}
    ModuleObject(unsigned long meshID) : meshID(meshID), transform(Transform()) {}
    ModuleObject() : meshID(0), textureID(0), transform(Transform()) {}

};