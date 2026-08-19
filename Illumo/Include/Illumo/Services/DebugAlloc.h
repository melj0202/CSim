#pragma once
#include <cstddef>

void*
operator new(std::size_t size);
void
operator delete(void* ptr) noexcept;
void*
operator new[](std::size_t size);
void
operator delete[](void* ptr) noexcept;