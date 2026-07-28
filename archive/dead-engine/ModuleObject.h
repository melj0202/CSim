#pragma once 
#include "Rendering/IMesh.h"
#include "Rendering/IShaderProgram.h"
#include "Rendering/ITexture.h"
#include "Foundation/MathTypes.h"

enum class DirtyFlags : uint8_t {
    None = 0,
    Transform = 1 << 0,
    Render = 1 << 1
};

using ObjectID = uint32_t;

struct Transform {
    Vector3 position;
    Vector3 scale;
    Quaternion rotation;

    Matrix4 toMatrix() {
        Matrix4 T = glm::translate(Matrix4(1.0f), position);
        Matrix4 R = glm::mat4_cast(rotation);
        Matrix4 S = glm::scale(Matrix4(1.0f), scale);
        return T * R * S;
    }
};

struct ModuleObject {

    ObjectID id;
    unsigned long meshID;
    unsigned long textureID;
    Transform transform;    
    
    ModuleObject(ObjectID id, unsigned long meshID, unsigned long textureID, Transform transform) : id(id), meshID(meshID), textureID(textureID), transform(transform) {}
    ModuleObject(ObjectID id, unsigned long meshID, unsigned long textureID) : id(id), meshID(meshID), textureID(textureID), transform(Transform()) {}
    ModuleObject(ObjectID id, unsigned long meshID) : id(id), meshID(meshID), textureID(0), transform(Transform()) {}
    ModuleObject(ObjectID id) : id(id), meshID(0), textureID(0), transform(Transform()) {}
    ModuleObject() : id(0), meshID(0), textureID(0), transform(Transform()) {}

};