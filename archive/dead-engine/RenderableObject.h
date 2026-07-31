#pragma once 
#include "Foundation/MathTypes.h"


struct RenderableObject {
    Matrix4 transform;
    uint32_t meshID;
    uint32_t textureID;
};