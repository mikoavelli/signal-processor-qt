# signal-processor-qt

Qt 6 / C++23 rewrite of **signal-processor** — a desktop application for the
spectrum analysis of communication lines. This repository contains **only the
Qt/C++ application**; the Python implementation lives in its own repository
and is untouched.

The application reads two text spectra (the mixture *Signal + Noise* and the
*Own noise*), computes the per-point signal level and the q ratio against the
normals from `settings.json`, flags violations, and persists every measurement
(with all calculation points) into a SQLite database (`measurements.db`) next
to the executable. A startup dialog collects the device name and the object
category before measurement.

All interface strings are compiled into the binary from a Qt resource
(`resources/i18n/{ru,en}.json`). The default language is **Russian**
(`SIGNAL_PROCESSOR_LANGUAGE=ru`); a build in another language is produced by
changing that CMake option.

## Requirements

- C++23 compiler (GCC 11+ / Clang 14+)
- Qt 6 (6.12 recommended) with the **Widgets** and **Sql** modules
- CMake 3.21+ and Ninja
- (optional) MinGW-w64 + a static Qt 6 for the Windows cross build
  (see *Windows* below)

## Build

`CMakePresets.json` provides `release` (`-O2` + LTO), `debug` and `windows`
(MinGW-w64 cross) configurations; all of them build with compiler warnings as
errors.

```sh
cmake --preset release
cmake --build --preset release
cp settings.json build/      # settings.json must sit next to the binary
```

`build/signal-processor-qt` runs from `build/` with `settings.json` beside it;
`measurements.db` is created there at runtime.

### Useful commands

```sh
cmake --preset debug             # development build
cmake --preset windows           # MinGW-w64 static cross build
ctest --test-dir build           # run the Qt Test suite
xargs clang-format -i < <(find src tests -name '*.cpp' -o -name '*.h')
xargs clang-format --Werror -n < <(find src tests -name '*.cpp' -o -name '*.h')
```

### Build options

| Option | Default | Meaning |
|--------|---------|---------|
| `CMAKE_BUILD_TYPE` | — | `Debug` for development, `Release` for shipping |
| `SIGNAL_PROCESSOR_LANGUAGE` | `ru` | UI language compiled into the binary |
| `SIGNAL_PROCESSOR_WARNINGS_AS_ERRORS` | `ON` | Fail the build on any compiler warning |
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
ctest --test-dir build   # tests build on Linux; skipped on Windows
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
   `cmake --preset windows && cmake --build --preset windows` —
   MinGW-w64 with a static Qt 6 for Windows; produces a fully self-contained
   `build-win/SignalProcessor.exe` (system DLLs only, no Qt DLLs needed);
   see `cmake/mingw-static.cmake` for the toolchain requirements. To assemble
   the release bundle: `cp build-win/SignalProcessor.exe dist/ &&
   cp settings.json dist/`.
2. **Native build on Windows:** run the presets in a “x64 Native Tools”
   prompt (or after `vcvars64.bat`): `cmake --preset release &&
   cmake --build --preset release`. `windeployqt` deploys the Qt runtime.

## Project layout

```
settings.json          # external configuration (ships next to the binary)
src/                   # application code (main.cpp, models, settings_store, …)
src/ui/                # widgets: main window, startup dialog, table model, …
tests/                 # Qt Test suite (CTest)
resources/i18n/        # ru.json / en.json compiled into the binary
cmake/                 # MinGW-w64 static cross toolchain
```
