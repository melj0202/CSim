#pragma once
#include "GLString.h"
#include <array>
#include <chrono>

class SplashText : public GLString {
    public:
    SplashText()
        : startTime(std::chrono::high_resolution_clock::now())
    {}
    SplashText(std::string content, int r, int g, int b, int a, int size_pt, int x, int y)
        : GLString(content, r, g, b, a, size_pt, x, y)
        , startTime(std::chrono::high_resolution_clock::now())
        , wakeDuration(2.0f)
        , fadeRate(1.0f)
    {}
    ~SplashText() = default;
    void DrawImpl() override { GLString::DrawImpl(); Fade(); };
    void Wake();
    void Fade();
    private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::duration<float> wakeDuration = std::chrono::duration<float>(2.0f);
    float fadeRate = 1.0f;

};