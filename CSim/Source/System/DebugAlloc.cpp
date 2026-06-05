#include "DebugAlloc.h"
#include <tracy/TracyC.h>
#include <malloc.h>
#include <exception>

void* operator new(std::size_t size)
{
	void* ptr = std::malloc(size);
	if (!ptr)
	{
		throw std::bad_alloc();
	}
	TracyCAlloc(ptr, size);
	return ptr;
}

void operator delete(void* ptr) noexcept
{
	TracyCFree(ptr);
	std::free(ptr);
}

void* operator new[](std::size_t size)
{
	void* ptr = std::malloc(size);
	if (!ptr)
	{
		throw std::bad_alloc();
	}
	TracyCAlloc(ptr, size);
	return ptr;
}

void operator delete[](void* ptr) noexcept
{
	TracyCFree(ptr);
	std::free(ptr);
}
