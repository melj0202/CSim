#pragma once
#include <string>
#include <vector>
#include "Drawable.h"

class RenderQueue {

public:
    RenderQueue() = default;
    ~RenderQueue() = default;

    int getDrawableCount() const;
    
    DrawableBase* getDrawable(int index) const;
    
    void add(DrawableBase* drawable);
    
    void remove(DrawableBase* drawable);
    
    void draw();

private:
    std::vector<DrawableBase*> drawables;
};