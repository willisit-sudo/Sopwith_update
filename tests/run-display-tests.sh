#!/bin/sh
# Copyright (C) 2026 Sopwith Update contributors
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
cd "$(dirname "$0")/.."
test_build=$(mktemp -d)
trap 'rm -rf "$test_build"' EXIT HUP INT TERM
source_files=$(awk '/COMMON_SOURCES =/{copy=1; next} /^fixincludes:/{copy=0} copy {for (i=1;i<=NF;i++) if ($i ~ /^[a-z_]+[.]c$/) print "src/" $i}' src/Makefile.am)
${CC:-cc} -std=gnu99 -I. -Isrc $(sdl2-config --cflags) tests/display_scaling.c $source_files src/sdl/pcsound.c src/sdl/timer.c src/sdl/controller.c $(sdl2-config --libs) -lm -o "$test_build/display-tests"
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy XDG_DATA_HOME="$test_build/data" "$test_build/display-tests"
