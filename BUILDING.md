# Build and verification

The standard build remains upstream Autotools + SDL2. See README.md for Linux commands and .github/workflows/build.yml for Windows/MSYS2 packaging.

On Linux, after configuring, run `./tests/run-display-tests.sh` for headless display integration checks. The GitHub workflow runs these checks on Linux.

## Initial local build, 10 November 2026

This restricted desktop environment has no system compiler or Autotools. An isolated Zig 0.14.1 C compiler and SDL2 2.32.10 headers were downloaded outside the source tree. The published Zig SHA-256 was verified. All common sources listed in src/Makefile.am and the five src/sdl backend files were compiled directly against the installed SDL2 shared library. A generated config.h supplied package strings, stdbool, Linux sockets and per-user high-score storage.

The unchanged upstream source compiled and its command-line help ran. This direct build does not validate Autotools packaging. A pre-existing signed-char comparison warning remains in SDL text-input handling. Native Windows builds and physical-display play testing are pending.

Do not commit compiler downloads, headers, absolute local paths, generated config.h or build caches. Standard developer machines should use normal dependency packages.

## Hosted build results

The first hosted Linux GCC and Clang builds passed, including the display integration checks. GCC also passed Autotools distcheck. The inherited 32-bit Windows job failed during dependency installation because MSYS2 no longer supplies its requested mingw-w64-i686-SDL2 package. This fork targets 64-bit Windows; the obsolete 32-bit matrix entry was removed. This does not establish compatibility with 32-bit Windows.

## Enhanced prototype

The 10 November 2026 graphics increment adds src/enhanced.c and enhanced.h to the common source list. Tests now verify F9 routing, preference persistence, deterministic rendering, original mask and object preservation, and menu-frame clearing. The harness explicitly enables assertions even in optimized builds. The local SDL2 build remains compatible with the original build process.

Presentation integration checks also passed locally with address and undefined-behavior sanitizers (upstream allocation-leak reporting disabled). They deliberately stage object visibility without running the flight simulation: a debug-mode trial of the existing AI encountered a pre-existing negative left shift in swobject.c:setdxdy. Flight arithmetic is unchanged in this graphics increment and remains a separate portability follow-up.
