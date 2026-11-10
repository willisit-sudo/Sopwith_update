# Display integration checks

After configuring the project, run `./tests/run-display-tests.sh` on Linux with SDL2 development packages. The harness uses the real video backend and SDL dummy drivers. Temporary preferences are isolated and removed.

Checks cover smooth and crisp scaling at 1080p, 1440p, 4K, ultrawide and portrait sizes; sub-native fitting; F8 routing and repeat suppression; held gameplay input, tick/level and framebuffer preservation; and saved preference reload. These checks do not prove full simulation determinism, visual quality on a physical monitor, controller behavior, fullscreen transitions or Windows runtime compatibility.
