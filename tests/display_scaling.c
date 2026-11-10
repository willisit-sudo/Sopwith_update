// Copyright (C) 2026 Sopwith Update contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#define SDL_MAIN_HANDLED
#include "../src/sdl/video.c"
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
	Vid_Shutdown();
	puts("Display integration checks passed: scaling, resize, F8, repeat, "
	     "state and persistence.");
	return 0;
}
