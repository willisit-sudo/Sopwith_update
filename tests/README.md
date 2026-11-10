# Display and presentation integration checks

After configuring, run `./tests/run-display-tests.sh` on Linux with SDL2 development packages. The harness uses the real backend and dummy video/audio drivers. Temporary preferences are isolated. Assertions remain enabled even with optimized compiler settings.

Checks cover smooth/crisp scaling at 1080p, 1440p, 4K, ultrawide, portrait and sub-native sizes; F8/F9 routing and repeat suppression; saved settings; enhanced rendering at every original plane pose and screen edges; deterministic repeated rendering; and fallback from gameplay to menu frames.

An initialized game-world test captures all active object structs, terrain, player collision-mask pixels, camera, counters and explosion seed, then verifies that repeated F9 toggles preserve them and the classic framebuffer. It stages the player in flight for an optional visual preview. Set SOPWITH_PREVIEW_DIR to an existing directory to write enhanced.argb (960x600 little-endian 0xAARRGGBB pixels) and classic.indexed (320x200 CGA indices). These captures exercise the real renderer but are a staged scene, not a gameplay recording.

The checks do not establish complete simulation determinism, physical-monitor quality, touch/controller behavior, fullscreen transitions or Windows runtime compatibility.
