#pragma once

#include <cstdlib>
#include "IAllocator.h"

class MallocAlloc {
public:
    MallocAlloc() = default;
    ~MallocAlloc() override = default;
    void* Allocate(size_t size) override;
    void Free(void* ptr) override;
};