# Assets

Runtime asset files live under `Illumo/Assets/`, outside the first-party
`Illumo/Source/` package tree. The current tree contains the Handjet font and
its license. Font rendering and runtime handles live in the rendering path
(`GLString` / `GameVisual`), not in a `Source/Assets` subsystem.
