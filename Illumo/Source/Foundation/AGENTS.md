# Foundation subsystem guidance

This file specializes the repository `AGENTS.md` for
`Illumo/Source/Foundation/`.

## Scope and boundaries

Foundation contains small dependency-light value types, platform/compiler
macros, and common helpers. It may be used by any first-party subsystem, so its
dependency direction must remain downward and narrow.

- Do not depend on App, Engine, Game, Rulesets, Services, or Rendering.
- Prefer the C++ standard library over new macros or global state.
- Keep platform/compiler branches isolated and compile-time visible.

## Required invariants

- Preserve the public names and layout of shared value types unless every
  consumer and compatibility expectation is deliberately migrated.
- Owning types must explicitly define or delete copy and move operations.
  Never leave a raw owning pointer in an implicitly copyable type.
- Macro arguments must be evaluated safely and all compiler/platform branches
  must be syntactically valid. Do not activate dormant legacy macros without
  focused compile and behavior tests.
- Time and sleep helpers must use consistent documented units on every branch.
- Do not add recursive helpers, hidden allocation, logging, rendering, or
  domain policy to Foundation.
- Keep headers self-contained; a consumer must not rely on incidental include
  order.

## Compatibility and verification

Foundation changes have repository-wide blast radius. Build both application
and tests with the supported MSVC configuration. When a compiler or platform
branch changes, add a focused compile check and verify on that toolchain before
claiming support; source inspection alone is not portability validation.

Relevant documentation is `docs/packages/foundation.md`,
`docs/contributing.md`, and the portability sections of the design book.
Update this file only for durable Foundation rules.
