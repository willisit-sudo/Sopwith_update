// Modified 2026-11-12: first playable title, music and round results.
// Modified 2026-11-12: improve sprite contrast during flight.
// Copyright (C) 2026 Sopwith Update contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// First presentation prototype: deterministic, read-only world rendering.
#include "enhanced.h"
#include "swsymbol.h"
#include <string.h>

#define S   ENHANCED_SCALE
#define W   ENHANCED_WIDTH
#define H   ENHANCED_HEIGHT
#define HUD 19
#define RGB(r, g, b)                                                           \
	(0xff000000u | ((uint32_t) (r) << 16) | ((uint32_t) (g) << 8) | (b))

static struct {
	bool valid;
	int camera, tick, count;
	GRNDTYPE terrain[SCR_WDTH];
	uint8_t ui[SCR_WDTH * SCR_HGHT];
	struct {
		int x, y, angle, inverted, color;
		obtype_t type;
		obstate_t state;
		const sopsym_t *symbol;
	} objects[MAX_OBJS];
} frame;

bool vid_modern_title = true;
bool enhanced_title_frame;

void Enhanced_ClearFrame(void)
{
	enhanced_title_frame = false;
	frame.valid = false;
}
bool Enhanced_HasFrame(void)
{
	return frame.valid;
}

void Enhanced_Capture(const uint8_t *ui, unsigned int pitch, int camera,
                      const GRNDTYPE *terrain, const OBJECTS *objects, int tick)
{
	int y, n = 0, visited = 0;
	const OBJECTS *ob;
	frame.camera = camera;
	frame.tick = tick;
	memcpy(frame.terrain, terrain, sizeof(frame.terrain));
	for (y = 0; y < SCR_HGHT; ++y) {
		memcpy(frame.ui + y * SCR_WDTH, ui + y * pitch, SCR_WDTH);
	}
	for (ob = objects; ob != NULL && visited < MAX_OBJS;
	     ob = ob->ob_next, ++visited) {
		if (!ob->ob_drwflg || ob->ob_symbol == NULL ||
		    ob->ob_x + ob->ob_symbol->w <= camera ||
		    ob->ob_x >= camera + SCR_WDTH) {
			continue;
		}
		frame.objects[n].x = ob->ob_x - camera;
		frame.objects[n].y = SCR_HGHT - 1 - ob->ob_y;
		frame.objects[n].angle = ob->ob_angle;
		frame.objects[n].inverted = ob->ob_orient;
		frame.objects[n].color = ob->ob_clr;
		frame.objects[n].type = ob->ob_type;
		frame.objects[n].state = ob->ob_state;
		frame.objects[n++].symbol = ob->ob_symbol;
	}
	frame.count = n;
	frame.valid = true;
}

// Coordinate noise is decorative and does not consume gameplay randomness.
static unsigned int Noise(int x, int y)
{
	uint32_t n = (uint32_t) x * 374761393u + (uint32_t) y * 668265263u;
	n = (n ^ (n >> 13)) * 1274126177u;
	return n ^ (n >> 16);
}

static void Pixel(uint32_t *p, unsigned int pitch, int x, int y, uint32_t c)
{
	if (x >= 0 && x < W && y >= 0 && y < (SCR_HGHT - HUD) * S) {
		p[y * pitch + x] = c;
	}
}

static void Background(uint32_t *p, unsigned int pitch)
{
	int x, y;
	for (y = 0; y < H; ++y) {
		int band = y / 24;
		uint32_t c =
		    y >= (SCR_HGHT - HUD) * S
		        ? RGB(18, 29, 38)
		        : RGB(77 + band * 4, 132 + band * 3, 162 + band * 2);
		for (x = 0; x < W; ++x) {
			p[y * pitch + x] = c;
		}
	}
	// Fixed sun, soft pixel rings and world-anchored clouds.
	for (y = 0; y < (SCR_HGHT - HUD) * S; ++y) {
		for (x = 0; x < W; ++x) {
			int dx = x - 770, dy = y - 130;
			int d = dx * dx + dy * dy;
			if (d < 25 * 25)
				p[y * pitch + x] = RGB(255, 236, 177);
			else if (d < 31 * 31)
				p[y * pitch + x] = RGB(217, 215, 174);
		}
	}
	for (x = 0; x < W; ++x) {
		int world = x + frame.camera * S / 6;
		int q = ((world % 310) + 310) % 310;
		int cloud_y = 112 + (world / 310 % 3) * 43;
		int top = cloud_y - (q > 40 && q < 105 ? 11 : 0);
		if (q > 10 && q < 155) {
			for (y = top; y < cloud_y + 10; ++y)
				Pixel(p, pitch, x, y, RGB(181, 207, 207));
		}
	}
	// Two restrained hill layers; parallax changes visuals only.
	for (x = 0; x < W; ++x) {
		int world = x + frame.camera * S / 4;
		int q = ((world % 460) + 460) % 460;
		int ridge = 405 - (q < 230 ? q : 460 - q) / 4;
		for (y = ridge; y < H; ++y)
			Pixel(p, pitch, x, y, RGB(111, 151, 153));
		world = x + frame.camera * S / 2;
		q = ((world % 310) + 310) % 310;
		ridge = 460 - (q < 155 ? q : 310 - q) / 5;
		for (y = ridge; y < H; ++y)
			Pixel(p, pitch, x, y, RGB(91, 132, 124));
	}
}

static void Terrain(uint32_t *p, unsigned int pitch)
{
	int x, y;
	for (x = 0; x < W; ++x) {
		int gx = x / S;
		int top = (SCR_HGHT - 1 - (int) frame.terrain[gx]) * S;
		int world_x = frame.camera * S + x;
		for (y = top; y < H; ++y) {
			int depth = y - top;
			unsigned int noise = Noise(world_x / 2, y / 2);
			uint32_t c;
			if (depth < 3)
				c = RGB(153, 171, 78);
			else if (depth < 8)
				c = RGB(79, 112, 57);
			else if (depth < 15)
				c = RGB(75, 75, 46);
			else if (depth < 60)
				c = (noise % 13 == 0) ? RGB(124, 98, 67)
				                      : RGB(110, 86, 58);
			else
				c = (noise % 19 == 0) ? RGB(99, 80, 58)
				                      : RGB(85, 70, 53);
			Pixel(p, pitch, x, y, c);
		}
		if (Noise(world_x / 3, 7) % 11 == 0) {
			Pixel(p, pitch, x, top - 1, RGB(136, 157, 72));
			Pixel(p, pitch, x, top - 2, RGB(136, 157, 72));
		}
	}
}

static uint32_t Shade(uint32_t c, int delta)
{
	int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
	r = clamp_range(0, r + delta, 255);
	g = clamp_range(0, g + delta, 255);
	b = clamp_range(0, b + delta, 255);
	return RGB(r, g, b);
}

// A new 48x48 side-on biplane, rotated into the original 16 flight poses.
// This art is independent of the original 16x16 collision symbol.
static uint32_t PlaneMaterial(int x, int y, bool enemy, int phase)
{
	uint32_t c = 0, outline = RGB(44, 43, 36);
	uint32_t linen = enemy ? RGB(241, 164, 124) : RGB(208, 234, 221);
	uint32_t paint = enemy ? RGB(181, 65, 55) : RGB(40, 130, 128);
	// Tail and upper wing, with a one-pixel ink border.
	if (x >= 4 && x <= 10 && y >= 14 && y <= 27)
		c = outline;
	if (x >= 5 && x <= 9 && y >= 15 && y <= 26)
		c = paint;
	if (x >= 13 && x <= 39 && y >= 12 && y <= 16)
		c = outline;
	if (x >= 14 && x <= 38 && y >= 13 && y <= 15)
		c = y == 13 ? RGB(252, 237, 195) : linen;
	// Bracing wires, cockpit, then the fuselage.
	if (((x == 20 || x == 34) && y >= 17 && y <= 29) ||
	    (x >= 20 && x <= 34 && y >= 17 && y <= 28 && (x + y) % 14 == 0))
		c = outline;
	if (x >= 22 && x <= 29 && y >= 20 && y <= 24)
		c = outline;
	if (x >= 24 && x <= 28 && y >= 20 && y <= 22)
		c = RGB(129, 195, 202);
	if (x >= 5 && x <= 42 && y >= 24 && y <= 28)
		c = outline;
	if (x >= 10 && x <= 40 && y >= 23 && y <= 29)
		c = outline;
	if (x >= 6 && x <= 40 && y >= 25 && y <= 27)
		c = paint;
	if (x >= 11 && x <= 39 && y == 24)
		c = linen;
	// Lower wing and its highlight.
	if (x >= 16 && x <= 36 && y >= 29 && y <= 32)
		c = outline;
	if (x >= 17 && x <= 35 && y >= 30 && y <= 31)
		c = linen;
	// Engine cowling, propeller and wheels.
	if (x >= 38 && x <= 42 && y >= 23 && y <= 29)
		c = outline;
	if (x >= 39 && x <= 41 && y >= 24 && y <= 28)
		c = RGB(179, 184, 173);
	if (x == 43 && y >= (phase ? 19 : 22) && y <= (phase ? 33 : 30))
		c = RGB(55, 43, 32);
	if ((x == 25 || x == 33) && y >= 33 && y <= 35)
		c = outline;
	if ((x >= 24 && x <= 27 || x >= 32 && x <= 35) && y >= 35 && y <= 37)
		c = outline;
	if ((x == 25 || x == 33) && y == 36)
		c = RGB(156, 164, 154);
	// A tiny roundel, or a pale enemy identification stripe.
	if (enemy && x >= 13 && x <= 15 && y >= 25 && y <= 27)
		c = RGB(236, 213, 176);
	if (!enemy) {
		int d = (x - 15) * (x - 15) + (y - 26) * (y - 26);
		if (d <= 5)
			c = RGB(56, 93, 126);
		if (d <= 2)
			c = RGB(239, 227, 193);
		if (d == 0)
			c = RGB(174, 64, 51);
	}
	return c;
}

static void Plane(uint32_t *p, unsigned int pitch, int n)
{
	int x, y, angle = (frame.objects[n].angle % ANGLES + ANGLES) % ANGLES;
	int c = COS(angle), s = SIN(angle);
	bool enemy = frame.objects[n].color != FACTION_PLAYER1;
	uint32_t sprite[48][48];
	for (y = 0; y < 48; ++y) {
		for (x = 0; x < 48; ++x) {
			int dx = x - 24, dy = y - 24;
			int sx = 24 + (dx * c - dy * s) / 256;
			int sy = 24 + (dx * s + dy * c) / 256;
			if (frame.objects[n].inverted)
				sy = 47 - sy;
			sprite[y][x] =
			    PlaneMaterial(sx, sy, enemy, frame.tick & 1);
		}
	}
	for (y = 0; y < 48; ++y) {
		for (x = 0; x < 48; ++x) {
			uint32_t color = sprite[y][x];
			if (!color && ((x > 0 && sprite[y][x - 1]) ||
			               (x < 47 && sprite[y][x + 1]) ||
			               (y > 0 && sprite[y - 1][x]) ||
			               (y < 47 && sprite[y + 1][x]))) {
				color = RGB(32, 42, 47);
			}
			if (color != 0)
				Pixel(p, pitch, frame.objects[n].x * S + x,
				      frame.objects[n].y * S + y, color);
		}
	}
}

// Retain each current symbol's silhouette and pose. Subpixel materials are
// presentation-only; no collision data or symbol dimensions are replaced.
static void Object(uint32_t *p, unsigned int pitch, int n)
{
	const sopsym_t *sym = frame.objects[n].symbol;
	int x, y, sx, sy;
	obtype_t type = frame.objects[n].type;
	bool player_color = frame.objects[n].color == FACTION_PLAYER1;
	uint32_t body = player_color ? RGB(183, 153, 86) : RGB(161, 72, 59);
	uint32_t wing = player_color ? RGB(222, 206, 151) : RGB(214, 153, 111);
	if (type == PLANE &&
	    (frame.objects[n].state == FLYING ||
	     frame.objects[n].state == WOUNDED ||
	     frame.objects[n].state == STALLED ||
	     frame.objects[n].state == WOUNDSTALL) &&
	    sym->w == 16 && sym->h == 16) {
		Plane(p, pitch, n);
		return;
	}
	for (y = 0; y < sym->h; ++y) {
		for (x = 0; x < sym->w; ++x) {
			unsigned int ink = sym->data[y * sym->w + x];
			uint32_t color;
			if (!ink)
				continue;
			if (type == PLANE)
				color = ink == 1
				          ? body
				          : (ink == 2 ? wing
				                      : RGB(242, 229, 190));
			else if (type == TARGET || type == BALLOON)
				color = ink == 1
				          ? RGB(196, 153, 99)
				          : (ink == 2 ? RGB(102, 131, 148)
				                      : RGB(235, 219, 173));
			else if (type == EXPLOSION || type == SHOT)
				color = ink == 1 ? RGB(245, 127, 47)
				                 : RGB(255, 222, 121);
			else if (type == SMOKE)
				color = RGB(123, 132, 127);
			else if (type == OX)
				color = ink == 3 ? RGB(237, 225, 193)
				                 : RGB(94, 72, 55);
			else
				color = ink == 3 ? RGB(238, 225, 181) : body;
			for (sy = 0; sy < S; ++sy) {
				for (sx = 0; sx < S; ++sx) {
					int px =
					    (frame.objects[n].x + x) * S + sx;
					int py =
					    (frame.objects[n].y + y) * S + sy;
					int shade =
					    sy == 0 ? 13 : (sy == 2 ? -17 : 0);
					uint32_t c = Shade(color, shade);
					if (type == TARGET || type == BALLOON) {
						const uint8_t *cell =
						    sym->data + y * sym->w + x;
						if ((sx == 0 &&
						     (x == 0 || !cell[-1])) ||
						    (sx == S - 1 &&
						     (x == sym->w - 1 ||
						      !cell[1])) ||
						    (sy == 0 &&
						     (y == 0 ||
						      !cell[-sym->w])) ||
						    (sy == S - 1 &&
						     (y == sym->h - 1 ||
						      !cell[sym->w]))) {
							c = RGB(43, 49, 51);
						}
					}
					// Small panel rivets and linen seams on
					// the planes.
					if (type == PLANE && sx == 1 &&
					    sy == 1 && (x + y) % 4 == 0)
						c = Shade(color, -30);
					Pixel(p, pitch, px, py, c);
				}
			}
		}
	}
}

void Enhanced_Render(uint32_t *pixels, unsigned int pitch)
{
	static const uint32_t hud_colors[] = {
	    RGB(18, 29, 38), RGB(102, 198, 213), RGB(235, 164, 111),
	    RGB(244, 230, 195)};
	int x, y, sx, sy, i;
	if (!frame.valid || pixels == NULL || pitch < W)
		return;
	Background(pixels, pitch);
	Terrain(pixels, pitch);
	for (i = 0; i < frame.count; ++i)
		Object(pixels, pitch, i);
	// Original HUD, messages, help and splats are a separate overlay.
	for (y = 0; y < SCR_HGHT; ++y) {
		for (x = 0; x < SCR_WDTH; ++x) {
			unsigned int c = frame.ui[y * SCR_WDTH + x] & 3;
			if (!c && y < SCR_HGHT - HUD)
				continue;
			for (sy = 0; sy < S; ++sy)
				for (sx = 0; sx < S; ++sx)
					pixels[(y * S + sy) * pitch + x * S +
					       sx] = hud_colors[c];
		}
	}
	for (x = 0; x < W; ++x)
		pixels[((SCR_HGHT - HUD) * S) * pitch + x] = RGB(62, 86, 92);
}

void Enhanced_RenderTitle(uint32_t *pixels, const uint8_t *ui,
                          unsigned int pitch)
{
	static const uint32_t inks[] = {0, RGB(129, 218, 207),
	                                RGB(255, 192, 124), RGB(255, 241, 207)};
	int x, y;
	for (y = 0; y < H; ++y) {
		for (x = 0; x < W; ++x) {
			int band = y / 16;
			int dx = x - 746, dy = y - 180;
			int ridge = 386 - abs(x % 470 - 235) / 5;
			uint32_t color =
			    RGB(24 + band * 4, 53 + band * 3, 79 + band * 2);
			if (dx * dx + dy * dy < 78 * 78)
				color = RGB(244, 187, 123);
			if (y >= ridge)
				color = RGB(39, 83, 86);
			if (y >= 428 - abs((x + 140) % 360 - 180) / 5)
				color = RGB(24, 54, 62);
			if (x >= 642 && x < 834 && y >= 214 && y < 406) {
				uint32_t plane = PlaneMaterial(
				    (x - 642) / 4, (y - 214) / 4, false, 1);
				if (plane)
					color = plane;
			}
			if (y >= 160 * S)
				color = y == 160 * S ? RGB(112, 171, 164)
				                     : RGB(18, 31, 42);
			unsigned int ink = ui[(y / S) * pitch + x / S] & 3;
			pixels[y * W + x] = ink ? inks[ink] : color;
		}
	}
}
