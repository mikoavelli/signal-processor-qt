# Cross toolchain: build the Windows (static) signal-processor-qt with MinGW-w64 on Linux.
#
# Requires a static Qt 6 for Windows built with MinGW-w64, with CMake package
# configs (Qt6Core, Qt6Sql, the Windows platform plugin and the SQLite driver)
# under <STATIC_QT_PREFIX>/lib/cmake/Qt6*.
#
# Set MINGW_SYSROOT (MinGW-w64 sysroot, e.g. /opt/mingw/x86_64-w64-mingw32) and
# STATIC_QT_PREFIX (the static Qt install prefix) in the environment, or put it
# in CMakeUserPresets.json next to this file (gitignored):
#   {"version": 3, "configurePresets": [{"name": "windows", "inherits": "windows-base",
#     "environment": {"MINGW_SYSROOT": "/opt/mingw/x86_64-w64-mingw32", "STATIC_QT_PREFIX": "/opt/mingw/x86_64-w64-mingw32/static"}}]}
#
# Usage:
#   MINGW_SYSROOT=/opt/mingw/x86_64-w64-mingw32 STATIC_QT_PREFIX=/opt/mingw/x86_64-w64-mingw32/static cmake --preset windows

if(NOT DEFINED ENV{MINGW_SYSROOT})
    message(FATAL_ERROR
        "MINGW_SYSROOT is not set. Point it at the MinGW-w64 sysroot, e.g.\n"
        "  MINGW_SYSROOT=/opt/mingw/x86_64-w64-mingw32 STATIC_QT_PREFIX=/opt/mingw/x86_64-w64-mingw32/static cmake --preset windows")
endif()
if(NOT DEFINED ENV{STATIC_QT_PREFIX})
    message(FATAL_ERROR
        "STATIC_QT_PREFIX is not set. Point it at the static Qt 6 install prefix.")
endif()

set(MINGW_SYSROOT "$ENV{MINGW_SYSROOT}" CACHE PATH "MinGW-w64 sysroot prefix" FORCE)
set(STATIC_QT_PREFIX "$ENV{STATIC_QT_PREFIX}" CACHE PATH "Static Qt 6 install prefix" FORCE)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Qt's static Sql module auto-scans all of its drivers (QPSQL needs PostgreSQL,
# QMYSQL needs MySQL — neither is installed for MinGW). Skip the auto scan and
# import only the plugins the application needs: the Windows platform plugin and
# the SQLite driver (see the WIN32 block in CMakeLists.txt).
set(QT_SKIP_AUTO_PLUGIN_INCLUSION ON)

# Target (Windows) Qt: static libraries and CMake package configs.
set(CMAKE_PREFIX_PATH
    "${STATIC_QT_PREFIX}"
    "${MINGW_SYSROOT}"
)
set(CMAKE_FIND_ROOT_PATH
    "${STATIC_QT_PREFIX}"
    "${MINGW_SYSROOT}"
)

# Host Qt — generator tools.
set(QT_HOST_PATH /usr CACHE PATH "Host Qt 6 installation (moc/rcc/uic)")

# --- Static link against the MinGW dependencies ------------------------------
# The static Qt 6 build reports dynamic ("system") wrap dependencies, which
# makes SignalProcessor.exe depend on the matching MinGW runtime DLLs (libzstd,
# libfreetype, ...). Force the static archives instead: CMake's find_library
# honors pre-seeded cache variables, and the harfbuzz config only exports
# SHARED targets, so its discovery must be disabled for the static fallback.
set(CMAKE_DISABLE_FIND_PACKAGE_harfbuzz TRUE)
# …and also send pkg-config lookups to an empty directory, so its
# libharfbuzz/libzstd/libpcre2 hits cannot shadow the pre-seeded static
# archives below. With no .pc files found, the find_library fallbacks use
# the pre-seeded *_LIBRARIES paths.
set(PKG_CONFIG_PATH "" CACHE PATH "unused marker" FORCE)
set(PKG_CONFIG_LIBDIR "/nonexistent-pc-dir" CACHE PATH "unused marker" FORCE)
set(PKG_CONFIG_SYSROOT_DIR "" CACHE PATH "unused marker" FORCE)
#
# FindPkgConfig binds the program path in the cache on first lookup, so
# finding "nothing" reliably means pointing it at a wrapper that hides all
# .pc files. With no .pc results, the find_library fallbacks use the
# pre-seeded *_LIBRARIES archives.
# Make Qt's FindWrapZSTD prefer zstd::libzstd_static (its config ships both,
# shared is otherwise picked, leaking libzstd.dll into the import table).
set(CMAKE_FIND_LIBRARY_SUFFIXES_OVERRIDE TRUE CACHE BOOL
    "prefer static zstd in Qt's wrap" FORCE)

set(PKG_CONFIG_EXECUTABLE "${CMAKE_CURRENT_LIST_DIR}/pkg-config-static.sh" CACHE FILEPATH
    "pkg-config wrapper that hides .pc files for the static link" FORCE)
# Qt's FindWrapBrotli consults unofficial-brotli (vcpkg) first; the MinGW
# sysroot has no such config, so point it at the bundled compat package.
set(unofficial-brotli_DIR "${CMAKE_CURRENT_LIST_DIR}/compat/unofficial-brotli" CACHE PATH
    "compat config: static brotli for Qt" FORCE)
set(HARFBUZZ_INCLUDE_DIRS "${MINGW_SYSROOT}/include/harfbuzz" CACHE PATH
    "harfbuzz include path for the static fallback" FORCE)
set(HARFBUZZ_LIBRARIES "${MINGW_SYSROOT}/lib/libharfbuzz.a" CACHE FILEPATH
    "static harfbuzz for the Qt wrap" FORCE)

set(FREETYPE_LIBRARY "${MINGW_SYSROOT}/lib/libfreetype.a" CACHE FILEPATH
    "static freetype for the Qt wrap" FORCE)
set(JPEG_LIBRARY "${MINGW_SYSROOT}/lib/libjpeg.a" CACHE FILEPATH
    "static libjpeg for the Qt wrap" FORCE)
set(JPEG_LIBRARY_RELEASE "${MINGW_SYSROOT}/lib/libjpeg.a" CACHE FILEPATH
    "static libjpeg for the Qt wrap" FORCE)
set(PNG_LIBRARY "${MINGW_SYSROOT}/lib/libpng16.a" CACHE FILEPATH
    "static libpng for the Qt wrap" FORCE)
set(PNG_LIBRARY_RELEASE "${MINGW_SYSROOT}/lib/libpng16.a" CACHE FILEPATH
    "static libpng for the Qt wrap" FORCE)
set(ZLIB_LIBRARY "${MINGW_SYSROOT}/lib/libz.a" CACHE FILEPATH
    "static zlib for the Qt wrap" FORCE)
set(ZLIB_LIBRARY_RELEASE "${MINGW_SYSROOT}/lib/libz.a" CACHE FILEPATH
    "static zlib for the Qt wrap" FORCE)
set(PCRE2_LIBRARY_RELEASE "${MINGW_SYSROOT}/lib/libpcre2-16.a" CACHE FILEPATH
    "static pcre2-16 for the Qt wrap" FORCE)
set(PCRE2_LIBRARY_DEBUG "${MINGW_SYSROOT}/lib/libpcre2-16.a" CACHE FILEPATH
    "static pcre2-16 for the Qt wrap (debug too)" FORCE)
set(PCRE2_LIBRARIES "${MINGW_SYSROOT}/lib/libpcre2-16.a" CACHE FILEPATH
    "static pcre2-16 for the Qt wrap" FORCE)
set(PCRE2_INCLUDE_DIRS "${MINGW_SYSROOT}/include" CACHE PATH
    "pcre2 include dir for the Qt wrap" FORCE)
set(ZSTD_LIBRARY_RELEASE "${MINGW_SYSROOT}/lib/libzstd.a" CACHE FILEPATH
    "static zstd for the Qt wrap" FORCE)
set(ZSTD_LIBRARIES "${MINGW_SYSROOT}/lib/libzstd.a" CACHE FILEPATH
    "static zstd for the Qt wrap" FORCE)
set(BrotliDec_LIBRARY_RELEASE "${MINGW_SYSROOT}/lib/libbrotlidec.a" CACHE FILEPATH
    "static brotli for the Qt wrap" FORCE)
set(BrotliEnc_LIBRARY_RELEASE "${MINGW_SYSROOT}/lib/libbrotlienc.a" CACHE FILEPATH
    "static brotli for the Qt wrap" FORCE)
set(BrotliCommon_LIBRARY_RELEASE "${MINGW_SYSROOT}/lib/libbrotlicommon.a" CACHE FILEPATH
    "static brotli for the Qt wrap" FORCE)
set(BrotliDec_LIBRARY_DEBUG "${MINGW_SYSROOT}/lib/libbrotlidec.a" CACHE FILEPATH
    "static brotli for the Qt wrap" FORCE)
set(BrotliEnc_LIBRARY_DEBUG "${MINGW_SYSROOT}/lib/libbrotlienc.a" CACHE FILEPATH
    "static brotli for the Qt wrap" FORCE)
set(BrotliCommon_LIBRARY_DEBUG "${MINGW_SYSROOT}/lib/libbrotlicommon.a" CACHE FILEPATH
    "static brotli for the Qt wrap" FORCE)
# System SQLite must be linked statically too: the wrapper selects the system
# library otherwise, and the produced exe would import libsqlite3-0.dll.
set(SQLite3_LIBRARY "${MINGW_SYSROOT}/lib/libsqlite3.a" CACHE FILEPATH
    "static sqlite3 for the Qt wrap" FORCE)
set(SQLite3_LIBRARY_RELEASE "${MINGW_SYSROOT}/lib/libsqlite3.a" CACHE FILEPATH
    "static sqlite3 for the Qt wrap (release)" FORCE)
set(SQLite3_LIBRARY_DEBUG "${MINGW_SYSROOT}/lib/libsqlite3.a" CACHE FILEPATH
    "static sqlite3 for the Qt wrap (debug too)" FORCE)
set(SQLite3_INCLUDE_DIR "${MINGW_SYSROOT}/include" CACHE PATH
    "sqlite3 headers for the Qt wrap" FORCE)
set(SQLite3_VERSION_STRING "3.54.1" CACHE STRING
    "sqlite3 version for the Qt wrap (pre-seeded, no run of the library needed)" FORCE)

set(OPENSSL_USE_STATIC_LIBS ON CACHE BOOL "link static OpenSSL for the Qt wrap" FORCE)
