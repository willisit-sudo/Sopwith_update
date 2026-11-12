// Copyright (C) 2026 Sopwith Update contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#define SDL_MAIN_HANDLED
#include "../src/sdl/video.c"
#include "swgrpha.h"
#include "swmove.h"
#include <assert.h>
#include <math.h>

static void CheckScale(int w, int h, bool crisp)
{
	float sx, sy, expected;
	vid_integer_scaling = crisp;
	SDL_SetWindowSize(window, w, h);
	Vid_Update();
	SDL_RenderGetScale(renderer, &sx, &sy);
	expected = fminf((float) w / SCR_WDTH, (float) h / SCR_HGHT);
	if (crisp && expected >= 1.0f) {
		expected = floorf(expected);
	}
	assert(fabsf(sx - expected) < 0.001f);
	assert(fabsf(sy - expected) < 0.001f);
}

static void CheckEnhanced(void)
{
	char *args[] = {"display-tests", "-c", "-q"};
	OBJECTS saved[MAX_OBJS], *original[MAX_OBJS], *ob;
	GRNDTYPE *saved_ground;
	uint8_t mask[SYM_WDTH * SYM_HGHT];
	uint8_t classic[SCR_WDTH * SCR_HGHT];
	uint32_t *first, *second;
	SDL_Event event;
	int i, n = 0, moves, level, camera;
	unsigned int seed;
	const char *preview = SDL_getenv("SOPWITH_PREVIEW_DIR");
	swinit(3, args);
	swinitlevel();
	for (ob = objtop; ob != NULL; ob = ob->ob_next) {
		ob->ob_drwflg = ob->ob_symbol != NULL;
	}
	// A staged in-flight preview using the real initialized game world.
	successful_flight = true;
	consoleplayer->ob_y += 65;
	Notification("Presentation test");
	swdisp();
	assert(Enhanced_HasFrame());
	for (ob = objtop; ob != NULL && n < MAX_OBJS; ob = ob->ob_next) {
		original[n] = ob;
		saved[n++] = *ob;
	}
	assert(n > 0);
	saved_ground = malloc(currgame->gm_max_x * sizeof(*ground));
	assert(saved_ground != NULL);
	memcpy(saved_ground, ground, currgame->gm_max_x * sizeof(*ground));
	assert(consoleplayer->ob_symbol->w == SYM_WDTH);
	assert(consoleplayer->ob_symbol->h == SYM_HGHT);
	memcpy(mask, consoleplayer->ob_symbol->data, sizeof(mask));
	for (i = 0; i < SCR_HGHT; ++i)
		memcpy(classic + i * SCR_WDTH, vid_vram + i * vid_pitch,
		       SCR_WDTH);
	moves = countmove;
	level = gamenum;
	camera = displx;
	seed = explseed;
	first = malloc(sizeof(*first) * ENHANCED_WIDTH * ENHANCED_HEIGHT);
	second = malloc(sizeof(*second) * ENHANCED_WIDTH * ENHANCED_HEIGHT);
	assert(first != NULL && second != NULL);
	Enhanced_Render(first, ENHANCED_WIDTH);
	Enhanced_Render(second, ENHANCED_WIDTH);
	assert(memcmp(first, second,
	              sizeof(*first) * ENHANCED_WIDTH * ENHANCED_HEIGHT) == 0);
	// Notification glyphs are part of the independent UI overlay.
	for (i = 0; i < SCR_WDTH * 8; ++i) {
		if (classic[i] == 3) {
			assert(first[(i / SCR_WDTH * 3) * ENHANCED_WIDTH +
			             (i % SCR_WDTH) * 3] == 0xfff4e6c3u);
		}
	}
	// Render every pose and offscreen edge without writing game data.
	for (i = 0; i < 32; ++i) {
		OBJECTS edge = *consoleplayer;
		edge.ob_next = NULL;
		edge.ob_angle = i % 16;
		edge.ob_orient = i / 16;
		edge.ob_x = displx + (i & 1 ? SCR_WDTH - 2 : -14);
		edge.ob_y = 199 - i;
		edge.ob_symbol = &symbol_plane[i % 4].sym[i / 4];
		Enhanced_Capture(vid_vram, vid_pitch, displx, ground + displx,
		                 &edge, countmove);
		Enhanced_Render(second, ENHANCED_WIDTH);
	}
	// Restore the captured scene, not the simulation.
	Enhanced_Capture(classic, SCR_WDTH, displx, ground + displx, objtop,
	                 countmove);
	// UI capture is usually before objects. Rebuild that frame for preview.
	swdisp();
	vid_enhanced = false;
	memset(&event, 0, sizeof(event));
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_F9;
	event.key.keysym.scancode = SDL_SCANCODE_F9;
	for (i = 0; i < 8; ++i) {
		assert(SDL_PushEvent(&event) == 1);
		assert(Vid_GetKey() == 0);
		assert(vid_enhanced == (i % 2 == 0));
	}
	event.key.repeat = 1;
	assert(SDL_PushEvent(&event) == 1);
	Vid_GetKey();
	assert(!vid_enhanced);
	event.key.repeat = 0;
	SDL_PushEvent(&event);
	Vid_GetKey();
	assert(vid_enhanced);
	vid_enhanced = false;
	swloadconf();
	assert(vid_enhanced);
	assert(countmove == moves && gamenum == level && displx == camera &&
	       explseed == seed);
	for (i = 0; i < n; ++i)
		assert(memcmp(original[i], &saved[i], sizeof(OBJECTS)) == 0);
	assert(memcmp(saved_ground, ground,
	              currgame->gm_max_x * sizeof(*ground)) == 0);
	assert(memcmp(mask, consoleplayer->ob_symbol->data, sizeof(mask)) == 0);
	for (i = 0; i < SCR_HGHT; ++i)
		assert(memcmp(classic + i * SCR_WDTH, vid_vram + i * vid_pitch,
		              SCR_WDTH) == 0);
	if (preview != NULL) {
		char filename[1024];
		FILE *f;
		snprintf(filename, sizeof(filename), "%s/enhanced.argb",
		         preview);
		f = fopen(filename, "wb");
		assert(f != NULL);
		Enhanced_Render(first, ENHANCED_WIDTH);
		assert(fwrite(first, sizeof(*first),
		              ENHANCED_WIDTH * ENHANCED_HEIGHT,
		              f) == ENHANCED_WIDTH * ENHANCED_HEIGHT);
		fclose(f);
		snprintf(filename, sizeof(filename), "%s/classic.indexed",
		         preview);
		f = fopen(filename, "wb");
		assert(f != NULL);
		assert(fwrite(classic, 1, sizeof(classic), f) ==
		       sizeof(classic));
		fclose(f);
	}
	Vid_ClearBuf();
	assert(!Enhanced_HasFrame());
	Vid_Update(); // Menus must render without a stale world snapshot.
	free(first);
	free(second);
	free(saved_ground);
}

int main(void)
{
	SDL_Event event;
	int before_keys[NUM_KEYS], i;
	uint8_t before_pixels[SCR_WDTH * SCR_HGHT];
	const int sizes[][2] = {{1920, 1080}, {2560, 1440}, {3840, 2160},
	                        {3440, 1440}, {800, 1200},  {160, 100}};
	SDL_SetMainReady();
	Vid_Init();
	for (i = 0; i < (int) arrlen(sizes); ++i) {
		CheckScale(sizes[i][0], sizes[i][1], false);
		CheckScale(sizes[i][0], sizes[i][1], true);
	}
	CheckScale(1920, 1080, false);
	Vid_ClearBuf();
	for (i = 0; i < SCR_HGHT; ++i) {
		memcpy(before_pixels + i * SCR_WDTH, vid_vram + i * vid_pitch,
		       SCR_WDTH);
	}
	keysdown[KEY_FIRE] = KEYDOWN_KEYBOARD;
	memcpy(before_keys, keysdown, sizeof(before_keys));
	countmove = 123;
	gamenum = 7;
	GetEvents();
	memset(&event, 0, sizeof(event));
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_F8;
	event.key.keysym.scancode = SDL_SCANCODE_F8;
	assert(SDL_PushEvent(&event) == 1);
	assert(Vid_GetKey() == 0);
	assert(vid_integer_scaling);
	assert(countmove == 123 && gamenum == 7);
	assert(memcmp(before_keys, keysdown, sizeof(before_keys)) == 0);
	for (i = 0; i < SCR_HGHT; ++i) {
		assert(memcmp(before_pixels + i * SCR_WDTH,
		              vid_vram + i * vid_pitch, SCR_WDTH) == 0);
	}
	event.key.repeat = 1;
	assert(SDL_PushEvent(&event) == 1);
	Vid_GetKey();
	assert(vid_integer_scaling);
	vid_integer_scaling = false;
	swloadconf();
	assert(vid_integer_scaling);
	event.key.repeat = 0;
	assert(SDL_PushEvent(&event) == 1);
	Vid_GetKey();
	assert(!vid_integer_scaling);
	event.key.keysym.sym = SDLK_F10;
	event.key.keysym.scancode = SDL_SCANCODE_F10;
	assert(SDL_PushEvent(&event) == 1);
	assert(Vid_GetKey() == 0);
	assert(snd_enhanced && !vid_enhanced && !vid_integer_scaling);
	event.key.repeat = 1;
	assert(SDL_PushEvent(&event) == 1);
	Vid_GetKey();
	assert(snd_enhanced);
	snd_enhanced = false;
	swloadconf();
	assert(snd_enhanced);
	CheckEnhanced();
	Vid_Shutdown();
	puts("Display integration checks passed: scaling, resize, F8, repeat, "
	     "state, persistence and enhanced presentation.");
	return 0;
}
