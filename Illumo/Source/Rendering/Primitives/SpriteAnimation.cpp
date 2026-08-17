#include "SpriteAnimation.h"
#include <algorithm>

void
SpriteAnimator::setClip(const SpriteAnimationClip* clipValue, bool restart)
{
  if (clip != clipValue || restart) {
    clip = clipValue;
    reset();
  }
}

void
SpriteAnimator::reset()
{
  frameIndex = 0;
  frameTime = 0.0;
  pingPongForward = true;
}

double
SpriteAnimator::currentDuration() const
{
  if (clip == nullptr || frameIndex >= clip->frames.size()) {
    return 0.0;
  }
  return std::max(clip->frames[frameIndex].durationSeconds, 0.000001);
}

void
SpriteAnimator::advanceFrame()
{
  if (clip == nullptr || clip->frames.empty()) {
    return;
  }
  const size_t last = clip->frames.size() - 1;
  if (clip->loopMode == SpriteLoopMode::Once) {
    if (frameIndex < last) {
      frameIndex += 1;
    } else {
      playing = false;
    }
    return;
  }
  if (clip->loopMode == SpriteLoopMode::Loop) {
    frameIndex = frameIndex == last ? 0 : frameIndex + 1;
    return;
  }
  if (clip->frames.size() == 1) {
    frameIndex = 0;
    return;
  }
  if (pingPongForward) {
    if (frameIndex == last) {
      pingPongForward = false;
      frameIndex -= 1;
    } else {
      frameIndex += 1;
    }
  } else if (frameIndex == 0) {
    pingPongForward = true;
    frameIndex += 1;
  } else {
    frameIndex -= 1;
  }
}

void
SpriteAnimator::update(double dt)
{
  if (!playing || clip == nullptr || clip->frames.empty() || dt <= 0.0) {
    return;
  }
  frameTime += dt;
  size_t safety = 0;
  while (playing && frameTime >= currentDuration() && safety < 100000) {
    frameTime -= currentDuration();
    advanceFrame();
    safety += 1;
  }
}

TextureRegion
SpriteAnimator::currentRegion() const
{
  if (clip == nullptr || frameIndex >= clip->frames.size()) {
    return TextureRegion{};
  }
  return clip->frames[frameIndex].region;
}
