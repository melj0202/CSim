#pragma once

#include <array>
#include <cstddef>

enum class TextureUploadSlotState
{
  Unused,
  Signaled,
  Busy
};

class TextureUploadPolicy
{
public:
  static constexpr int kPboCount = 3;
  static constexpr std::size_t kDirectUploadThresholdBytes = 64u * 1024u;

  static bool useDirectUpload(std::size_t packedBytes)
  {
    return packedBytes <= kDirectUploadThresholdBytes;
  }

  static int selectAvailableSlot(
    int currentSlot,
    const std::array<TextureUploadSlotState, kPboCount>& states)
  {
    for (int attempt = 0; attempt < kPboCount; ++attempt) {
      const int candidate = (currentSlot + 1 + attempt) % kPboCount;
      if (states[static_cast<std::size_t>(candidate)] !=
          TextureUploadSlotState::Busy) {
        return candidate;
      }
    }
    return -1;
  }
};
