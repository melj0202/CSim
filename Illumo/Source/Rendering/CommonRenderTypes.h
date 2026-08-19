#pragma once

#include <cstdint>

enum ConstantBufferSlot : uint32_t
{
  Slot_PerFrame = 0,    // Always slot 0
  Slot_PerPass = 1,     // Always slot 1
  Slot_PerMaterial = 2, // Always slot 2
  Slot_PerObject = 3    // Always slot 3
};
