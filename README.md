# nalay — nizar's amazing layout library

nalay is a C++20, single-header layout library built for performance.It uses a flexbox-inspired layout model with a static arena allocator and a React-like declarative syntax, producing a sorted list of renderer-agnostic primitives that you can composite in any 3D engine, pipe to raylib or SDL3, or even use in WASM.

---

## Quick start

```cpp
#include <raylib.h>

#include <nalay/nalay.hpp>
#include <nalay/raylib.hpp>

auto main() -> int
{
    nalay::render_queue   queue;
    nalay::renderer       renderer = raylib_renderer{};
    nalay::image_provider images   = raylib_image_provider{};
    nalay::font_provider  fonts    = raylib_image_provider{};
    nalay::context ctx { images, fonts };

    auto caskaydia_cove = fonts.load("caskaydia_cove.ttf");

    InitWindow(1900, 1200, "hello world");
    SetTargetFPS(60);

    using namespace nalay::ui;
    auto layout = vertical{
        button("this is a button").with_style({ font = caskaydia_cove }),
        button("this is a button")
    };

    while (not WindowShouldClose()) {
        queue.reset();
        layout.compute(queue, ctx);
        BeginDrawing();
            renderer.render(queue);
        EndDrawing();
    }
    return 0;
}
```

---

## Building

### Dependencies

Dependencies are managed via vcpkg. See [`vcpkg.json`](vcpkg.json) for the full list.

### Unix / single-configuration generators (Make, Ninja)

```sh
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```

### Windows / multi-configuration generators (Visual Studio)

```sh
cmake -S . -B build
cmake --build build --config Release
```

### MSVC

MSVC is not standards-compliant by default. Pass the conformance flags defined in the `flags-msvc` preset:

```sh
cmake -S . -B build --preset flags-msvc
cmake --build build --config Release
```

See [`CMakePresets.json`](CMakePresets.json) for the exact flags.

### Apple Silicon

CMake 3.20.1+ is required for native Apple Silicon support. Download the latest release from [cmake.org](https://cmake.org/download/) if your installed version is older.

---

## Installing

The project must be built before installing (see above). Requires CMake 3.15+.

### Single-configuration generators

```sh
cmake --install build
```

### Multi-configuration generators

```sh
cmake --install build --config Release
```

---

## CMake integration

nalay exports a CMake package for use with `find_package`:

| Field | Value |
|---|---|
| Package name | `nalay` |
| Target name | `nalay::nalay` |

```cmake
find_package(nalay REQUIRED)

target_link_libraries(my_target PRIVATE nalay::nalay)
```

### Note for packagers

`CMAKE_INSTALL_INCLUDEDIR` is set to a non-default path when nalay is built as a top-level project to avoid polluting a shared prefix. Review [`cmake/install-rules.cmake`](cmake/install-rules.cmake) for the full install rules.
