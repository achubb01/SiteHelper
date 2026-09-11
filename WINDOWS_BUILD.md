# Windows desktop build

SiteHelper uses the SDL3 source tree in `external/SDL`; no separate SDL
installation is required. Use a current Visual Studio 2022 installation with
the **Desktop development with C++** workload and CMake.

From a Visual Studio Developer PowerShell:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug --target sitehelper_sdl
.\build\sitehelper\Debug\sitehelper_sdl.exe
```

The exact topology layer uses SiteHelper's portable two-limb 128-bit arithmetic
and is enabled on MSVC as well as GCC and Clang. The portable model, editor,
GUI, renderer abstraction, SDL renderer adapter, platform-event adapter, and
desktop application remain enabled. Portable tests
can be built with `cmake --build build --config Debug` and run with
`ctest --test-dir build -C Debug --output-on-failure`.

On Linux, the normal workflow remains:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The Windows executable has no runtime SDL search-path setup: the CMake target
copies the built SDL3 DLL beside `sitehelper_sdl.exe` after linking.
