# Sopwith Update

A modern presentation experiment built on [SDL Sopwith](https://github.com/fragglet/sdl-sopwith), preserving classic flight and gameplay while preparing optional enhanced graphics and audio. Early development: a first enhanced graphics prototype is available; enhanced audio is available as an independent prototype.

## Available now

- Classic CGA graphics and emulated PC speaker audio from SDL Sopwith.
- Resizable, aspect-preserving display, high-DPI support and desktop fullscreen.
- **F9** switches between classic and enhanced graphics during play, without restarting. The choice is saved.
- Enhanced graphics add a 960x600 pixel-art canvas, an independent rotating biplane sprite, layered sky and hills, and live textured terrain. Original masks still define collisions.
- **F8** switches between the existing smooth fit and crisp integer pixel scaling. Small windows fit the whole image instead of cropping it.
- The Options menu exposes **Enhanced graphics** and **Crisp pixel scaling**.
- **Alt+Enter** switches fullscreen.

The display toggle does not restart the game or change its rules. Original symbol data still determines collisions. Future replacement artwork will be separate from those masks.

## Build on Linux

On Debian/Ubuntu install the normal build dependencies, then build using the upstream build system:

```sh
sudo apt install build-essential autoconf automake libsdl2-dev
./autogen.sh --with-hiscores-path=
make -j
./src/sopwith
```

The empty high-scores path uses per-user storage. See [upstream documentation](README.upstream.md) and [the controls](FAQ.md). This project uses SDL2; SDL3 migration is not required to add enhanced graphics.

## Build on Windows

Use MSYS2 MINGW64 with Autotools, make, a C compiler and SDL2 development packages. The included GitHub Actions workflow builds Windows packages as well as Linux builds. Windows runtime testing of this fork is still pending.

## Development and verification

See [BUILDING.md](BUILDING.md), [the roadmap](ROADMAP.md), and [upstream provenance](UPSTREAM.md). Headless display checks are described in tests/README.md.

## Credits and license

Based on Sopwith by David L. Clark and SDL Sopwith by Simon Howard and contributors. Retains the original copyright notices, [AUTHORS](AUTHORS), and [COPYING.md](COPYING.md). Inspected source headers permit GPL version 2 or later. This is an independent fork, not an official upstream release.

## Graphics prototype

Classic remains the default. Press F9 during a game to compare. The splash defaults to a new illustrated title. Options → Title settings independently switches Modern splash and Modern title music back to the original versions. Enhanced gameplay uses the original HUD as an overlay. Press F10 during play to switch between classic speaker audio and synthesized engine, gunfire, and impact sounds. The setting is saved independently of graphics. The usual sound key still mutes either mode.

The replacement biplane is drawn procedurally in its original flight poses; damaged and finale sprites and nonstandard-size plane symbols retain shaded original artwork. Scenery and colours are an initial experiment, not a finished art direction. Enhanced world graphics currently fall back to classic when touch controls are active. This keeps the touch panel usable.

## First playable

Press **S** for ground targets only or **C** for ground targets and enemy planes. At the end of a round the game shows Level cleared or Out of planes. The original sound key still controls gameplay sound. Start with `-q` to start silent.

The Title settings menu previews Classic and Modern artwork and music as you switch them. Plane loss music in the same menu controls a short cue when your plane crashes.

Modern title music uses the same score as Classic with a layered synth voice. The stock tune is “The U.S. Air Force” (Wild Blue Yonder). Custom mission title tunes also play through either voice.

Menu artwork applies to the splash, Options and Key bindings. Title music uses the same original melody in either voice.
