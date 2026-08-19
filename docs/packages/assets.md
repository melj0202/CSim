# Illumo assets and runtime staging

Illumo owns `Shader/`, `Assets/RendererDemo`, fonts, notices, third-party
dependencies, and redistribution licenses. `AssetManager` retains its managed
file texture/shader cache, one CPU file/decode worker, render-thread GPU pump,
stable typed handles, Debug timestamp polling, explicit reload, last-good
fallback, reference counting, and shutdown-before-backend lifetime.

The library supplies the runtime-staging implementation that copies those
files beside a consuming executable. Tracked input changes retrigger staging
and removed shader or asset files disappear from the staged copy. The
IllumoGame target supplies its `envvars.json` seed as product data; Illumo's
copy-if-missing staging logic preserves existing user settings.
