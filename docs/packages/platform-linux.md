# Illumo Linux scaffold

Linux is not supported. Engine-owned `_main.cpp` and `LinuxSaveLoad.cpp` use
the shared application and SaveLoad contracts but have not received a native
configure, build, or runtime validation. A future port must keep GTK limited to
native dialogs and pass native tests plus GLFW/OpenGL interaction and shutdown
checks before support is claimed.
