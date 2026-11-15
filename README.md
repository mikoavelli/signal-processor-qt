# signal-processor-qt

Qt 6 / C++23 rewrite of **signal-processor** — a desktop application for the
spectrum analysis of communication lines. This repository contains **only the
Qt/C++ application**; the Python implementation lives in its own repository
(`~/Projects/signal-processor`) and is untouched.

The application reads two text spectra (the mixture *Signal + Noise* and the
*Own noise*), computes the per-point signal level and the q ratio against the
normals from `settings.json`, flags violations, and persists every measurement
(with all calculation points) into a SQLite database (`measurements.db`) next
to the executable. A startup dialog collects the device name and the object
category before measurement.

All interface strings are compiled into the binary from a Qt resource
(`resources/i18n/{ru,en}.json`). The default language is **Russian**
(`SIGNAL_PROCESSOR_LANGUAGE=ru`); a release in another language is produced by
changing that CMake option.

## Requirements

- C++23 compiler (GCC 11+ / Clang 14+ / MSVC 2022)
- Qt 6 (6.12 recommended) with the **Widgets** and **Sql** modules
- CMake 3.21+
- Ninja (used by the presets)
- (optional) `just` — the `justfile` wraps the common commands
- (optional) MinGW-w64 + static Qt for the Windows cross build
  (see *Windows (cross build)* below)

## Build

```sh
cmake --preset release
cmake --build --preset release
just settings   # copy settings.json next to every configured binary
```

The produced `build/signal-processor-qt` binary works after it is placed next
to `settings.json` (and, at runtime, `measurements.db` is created there).

### Shortcuts (`just`)

`CMakePresets.json` holds four configurations — `release` (`-O2` + LTO),
`debug`, `san` (Debug + ASan/UBSan) and `windows` (MinGW-w64 cross build).
All of them treat compiler warnings as errors. The `justfile` wraps them:

```sh
just              # optimized release build
just debug        # development build
just san          # debug build with sanitizers
just test         # build a preset and run the Qt Test suite
just all          # build all Linux presets in turn
just run          # build and run (release by default)
just smoke        # headless 5-second run of the release build
just settings     # copy settings.json next to every configured binary
just lang en      # release build in another compiled-in language
just windows-static  # cross build → build-win/SignalProcessor.exe
just dist         # assemble dist/: SignalProcessor.exe + settings.json
just docs         # Doxygen HTML into build/doxygen/html
just fmt          # format all sources with clang-format
just fmt-check    # verify sources are formatted
just clean        # remove all preset build directories
```

### Build options

| Option | Default | Meaning |
|--------|---------|---------|
| `CMAKE_BUILD_TYPE` | — | `Debug` for development, `Release` for shipping |
| `SIGNAL_PROCESSOR_LANGUAGE` | `ru` | UI language compiled into the binary |
| `SIGNAL_PROCESSOR_LTO` | `ON` | Interprocedural (link-time) optimization in release builds |
| `SIGNAL_PROCESSOR_NATIVE_OPT` | `OFF` | `-march=native` (GCC/Clang); the binary stops being portable |
| `SIGNAL_PROCESSOR_WARNINGS_AS_ERRORS` | `ON` | Fail the build on any compiler warning |
| `SIGNAL_PROCESSOR_SANITIZERS` | `OFF` | ASan + UBSan in Debug builds (GCC/Clang only) |
| `SIGNAL_PROCESSOR_BUILD_TESTS` | `ON` | Build the Qt Test suite (skipped on Windows) |

## Configuration (`settings.json`)

`settings.json` is the single external data file; it always lives next to the
executable. If it is missing, has a syntax error, or fails validation, the
program does not start and shows the precise reason.

Top-level schema (version `1`):

| Section | Contents |
|---------|----------|
| `schema_version` | Must be `1`. |
| `frequency_constants` | `number_of_constants` plus `f_i`, `delta_f_i`, `k_i`, `delta_a_i` — one value per control constant (20 in the shipped file). |
| `norm_params_by_category` | `delta_stn` (violation thresholds) and `w_n` (W limits), one value per object category. |
| `norm_noise_by_line` | Normalised noise per line type: `symmetrical`, `asymmetrical`, `power`. |
| `line` | Line-name templates per type; `{}` marks a field filled in by the user (e.g. `"ЛВС, пара {} - {}"`). Templates with unbalanced braces are rejected. |
| `operation_modes` | Operation modes shown in the interface; must not be empty. |

Validation is strict: unknown keys are rejected, values must be finite
numbers, central frequencies and normalised noise must not be negative, and
list length `number_of_constants` is enforced wherever applicable.

## Data (`measurements.db`)

On first start the app creates `measurements.db` next to itself with two
tables — `measurements` (device, category, line, mode, R, overall result) and
`measurement_points` (per control frequency; foreign key, cascade delete).
Rows are appended on every “Save measurement”.

If the directory the executable sits in is not writable (e.g. `C:\` root or
`Program Files`), the app falls back to a per-user location —
`%LOCALAPPDATA%\SignalProcessor\measurements.db` on Windows,
`~/.local/share/SignalProcessor/measurements.db` on Linux — copying an
existing adjacent `measurements.db` over best-effort. An adjacent
`measurements.db`, when present and writable, always takes precedence.

`measurement_type` is stored as the stable code (`voltage` / `current`), not as
a translated string, so database contents do not depend on the build language.

## Tests

```sh
just test          # configure, build, ctest --output-on-failure
```

The Qt Test suite mirrors the original Python `unittest` suite — calculation,
settings loading, spectrum reader, database and UI helpers (5 suites). Tests
run with `QT_QPA_PLATFORM=offscreen`. A historical note: the original Python
repository ships tests that fail against its own implementation (12 failing);
this port **implements** the intended behavior instead, so the ported suite is
fully green (including negative-frequency handling, conflicting duplicate
records, the intermediate-value zero guard, the resistance upper limit, the
empty operation-mode list and the unbalanced-brace line template).

## Windows

Two ways to produce a Windows build:

1. **Cross build on Linux (recommended here):**
   `just windows-static` — MinGW-w64 with a static Qt 6 for Windows;
   produces a fully self-contained `build-win/SignalProcessor.exe`
   (system DLLs only, no Qt DLLs needed); see `cmake/mingw-static.cmake`
   for the toolchain requirements.
   `just dist` — assembles `dist/SignalProcessor.exe` + `dist/settings.json`.
2. **Native build on Windows:** run the presets in a “x64 Native Tools”
   prompt (or after `vcvars64.bat`): `cmake --preset release &&
   cmake --build --preset release`. `windeployqt` deploys the Qt runtime;
   `just windows` wraps it.

## Project layout

```
settings.json          # external configuration (ships next to the binary)
src/                   # application code (main.cpp, models, settings_store, …)
src/ui/                # widgets: main window, startup dialog, table model, …
tests/                 # Qt Test suite (CTest)
resources/i18n/        # ru.json / en.json compiled into the binary
cmake/                 # MinGW-w64 static cross toolchain
```
