#pragma once

#include <Illumo/Rendering/Primitives/PrimitiveTypes.h>
#include <cstddef>
#include <vector>

enum class SpriteLoopMode
{
  Once,
  Loop,
  PingPong
};

struct SpriteAnimationFrame
{
  TextureRegion region;
  double durationSeconds = 0.1;
};

struct SpriteAnimationClip
{
  std::vector<SpriteAnimationFrame> frames;
  SpriteLoopMode loopMode = SpriteLoopMode::Loop;
};

class SpriteAnimator
{
public:
  void setClip(const SpriteAnimationClip* clip, bool restart = true);
  void update(double dt);
  void play() { playing = true; }
  void pause() { playing = false; }
  void reset();

  bool isPlaying() const { return playing; }
  size_t getFrameIndex() const { return frameIndex; }
  TextureRegion currentRegion() const;

private:
  const SpriteAnimationClip* clip = nullptr;
  size_t frameIndex = 0;
  double frameTime = 0.0;
  bool playing = true;
  bool pingPongForward = true;

  double currentDuration() const;
  void advanceFrame();
};
