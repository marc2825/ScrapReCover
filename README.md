[WIP] The source code is complete, but documentation and data are still being prepared. [WIP]

# [SCF' 25] ScrapReCover: An Interactive Optimization System for Freeform Patchwork Layouts [WISS '25]
ScrapReCover is an interactive tool for designing freeform patchwork layouts from fabric scraps.

(Homepage: https://marc2825.github.io/ScrapReCover)

## Quick Start (WSL2)

1) Install dependencies (see `DEPENDENCIES.md`) or run:

```
./setup.sh
```

2) Build:

```
cmake -S . -B build -G Ninja
cmake --build build -j1
```

Siv3D is found via `find_package(Siv3D)`, so it must be installed (see `BUILD.md` or `setup.sh`).

3) Run from the build directory so `assets/config.json` is found:

```
cd build
../external/Siv3D/Linux/App/Siv3DApp
```

The executable is placed under `external/Siv3D/Linux/App` by `CMAKE_RUNTIME_OUTPUT_DIRECTORY`.
If you run from the repository root, the config path in `src/main.cpp` will not resolve.

## Configuration

- Main config file: `assets/config.json`
- Pattern shape config: `assets/pattern_shape.json`
- Key sections: `generate_polygon`, `unplaced_list`, `polygon_config`, `paths` (preset/cropper).
- See `CONFIG.md` for details.

## Project Layout

- `src/` and `include/`: C++ application code.
- `src/ui/measure_scrap/`: Python tool for scrap polygon extraction.
- `assets/`: configuration and assets.
- `external/`: third-party dependencies (Siv3D, nlohmann/json).
- `docs/`: optional documentation.

## License

This project includes third-party libraries with their own licenses under `external/`.
