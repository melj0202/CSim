#include "Rendering/OpenGL/TextureUploadPolicy.h"
#include <Illumo/Testing/TestHelpers.h>
#include <Illumo/Testing/TestRegistry.h>
#include <array>

static int
testTextureUploadPolicy()
{
  TestCounters counters;
  testSection("Texture upload: direct cutoff and non-waiting fallback");
  testTrue(counters,
           TextureUploadPolicy::useDirectUpload(64u * 1024u),
           "64 KiB uploads use the direct path");
  testTrue(counters,
           !TextureUploadPolicy::useDirectUpload(64u * 1024u + 1u),
           "larger uploads are eligible for PBO staging");

  std::array<TextureUploadSlotState, TextureUploadPolicy::kPboCount> states = {
    TextureUploadSlotState::Busy,
    TextureUploadSlotState::Busy,
    TextureUploadSlotState::Busy
  };
  testEqInt(counters,
            TextureUploadPolicy::selectAvailableSlot(0, states),
            -1,
            "all busy PBO slots select direct fallback without waiting");
  states[2] = TextureUploadSlotState::Signaled;
  testEqInt(counters,
            TextureUploadPolicy::selectAvailableSlot(0, states),
            2,
            "a signaled PBO slot is selected in ring order");
  states[1] = TextureUploadSlotState::Unused;
  testEqInt(counters,
            TextureUploadPolicy::selectAvailableSlot(0, states),
            1,
            "an unused PBO slot takes priority in ring order");
  return counters.failures;
}

void
registerTextureUploadPolicyTests(IllumoTestRegistry& registry)
{
  registry.add("Illumo.Rendering.TextureUploadPolicy",
               []() { return testTextureUploadPolicy(); });
}
