#pragma once 
#include "Util/Math.h"


struct RenderableObject {
    Matrix4 transform;
    uint32_t meshID;
    uint32_t textureID;
};