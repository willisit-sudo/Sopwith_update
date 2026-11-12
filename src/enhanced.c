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
		int x, y, angle, inverted, color, life;
		obtype_t type;
		obstate_t state;
		const sopsym_t *symbol;
	} objects[MAX_OBJS];
} frame;

int enhanced_theme;

const char *Enhanced_ThemeName(void)
{
	switch (enhanced_theme) {
	case THEME_MODERN_DAY:
		return "Modern Day";
	case THEME_STAR_WARS:
		return "Star Wars";
	case THEME_MOON:
		return "Moon";
	default:
		return "Biplane";
	}
}

bool vid_modern_title = true;
bool enhanced_title_frame;
bool enhanced_settings_frame;

void Enhanced_ClearFrame(void)
{
	enhanced_title_frame = false;
	enhanced_settings_frame = false;
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
		frame.objects[n].life = ob->ob_life;
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

static bool CloudPuff(int x, int y, int cx, int cy, int radius)
{
	int dx = x - cx, dy = (y - cy) * 2;
	return dx * dx + dy * dy < radius * radius;
}

static void ThemeSky(uint32_t *p, unsigned int pitch)
{
	int x, y;
	for (y = 0; y < H; ++y) {
		for (x = 0; x < W; ++x) {
			uint32_t color = RGB(18, 29, 38);
			if (y < (SCR_HGHT - HUD) * S) {
				if (enhanced_theme == THEME_MOON ||
				    enhanced_theme == THEME_STAR_WARS) {
					int dx = x - 742, dy = y - 133;
					int distance = dx * dx + dy * dy;
					unsigned int star = Noise(x / 2, y / 2);
					color = RGB(13 + y / 50, 16 + y / 45,
					            34 + y / 23);
					if (star % 727 == 0)
						color = RGB(168, 186, 218);
					if (distance < 54 * 54) {
						unsigned int land = Noise(
						    (x + y / 2) / 15, y / 9);
						color = land % 5 < 2
						          ? RGB(112, 169, 145)
						          : RGB(68, 126, 185);
						if (dx + dy > 48)
							color =
							    RGB(38, 67, 106);
					}
				} else {
					int world = x + frame.camera * S / 6;
					int cloud_x = world % 390;
					int cloud_y =
					    y - 104 - (world / 390 % 3) * 51;
					color = RGB(65 + y / 5, 135 + y / 8,
					            188 + y / 13);
					if (CloudPuff(cloud_x, cloud_y, 74, 2,
					              42) ||
					    CloudPuff(cloud_x, cloud_y, 108,
					              -14, 48) ||
					    CloudPuff(cloud_x, cloud_y, 150, 1,
					              53) ||
					    CloudPuff(cloud_x, cloud_y, 191, 10,
					              31)) {
						color = cloud_y > 9
						          ? RGB(191, 211, 222)
						          : RGB(234, 241, 244);
					}
				}
			}
			p[y * pitch + x] = color;
		}
	}
}

static void Background(uint32_t *p, unsigned int pitch)
{
	int x, y;
	if (enhanced_theme != THEME_BIPLANE) {
		ThemeSky(p, pitch);
		return;
	}
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
			if (enhanced_theme == THEME_MOON ||
			    enhanced_theme == THEME_STAR_WARS) {
				int gray =
				    depth < 3 ? 177 : (depth < 13 ? 120 : 83);
				gray += (int) (noise % 13);
				c = RGB(gray, gray + 3, gray + 13);
			} else if (enhanced_theme == THEME_MODERN_DAY) {
				c = depth < 3
				      ? RGB(137, 162, 92)
				      : (noise % 17 == 0 ? RGB(83, 105, 70)
				                         : RGB(64, 86, 62));
			}
			Pixel(p, pitch, x, y, c);
		}
		if (enhanced_theme == THEME_BIPLANE &&
		    Noise(world_x / 3, 7) % 11 == 0) {
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

// Side-on silhouettes share the biplane's visual anchor and rotation.
static uint32_t JetMaterial(int x, int y, bool enemy)
{
	uint32_t trim = enemy ? RGB(188, 79, 63) : RGB(82, 152, 165);
	uint32_t color = 0;
	// Swept single tail, horizontal stabilizer and broad wing root.
	if (y >= 13 && y <= 24 && x >= 10 && x <= 13 + (y - 13) / 2)
		color = y < 17 ? trim : RGB(150, 164, 171);
	if (y >= 25 && y <= 28 && x >= 7 && x <= 16)
		color = RGB(129, 145, 151);
	if (y >= 25 && y <= 32 && x >= 16 && x <= 30 - (y - 25))
		color = y > 28 ? RGB(110, 126, 134) : RGB(183, 192, 194);
	// Single exhaust, level spine and long radome.
	if (x >= 7 && x <= 13 && y >= 22 && y <= 26)
		color = x < 10 ? RGB(62, 67, 70) : RGB(121, 132, 138);
	if (x >= 12 && x <= 35 && y >= 21 && y <= 27)
		color = y < 24 ? RGB(208, 215, 213) : RGB(151, 163, 168);
	if (x >= 35 && x <= 45 && abs(y - 24) <= (45 - x) / 3)
		color = RGB(177, 188, 192);
	// Raised bubble canopy and shaded chin intake.
	if (x >= 26 && x <= 34 && y >= 18 + abs(x - 30) / 3 && y <= 21)
		color = y == 19 ? RGB(154, 196, 208) : RGB(57, 96, 117);
	if (x >= 25 && x <= 33 && y >= 26 && y <= 28)
		color = y == 28 ? RGB(95, 113, 121) : RGB(57, 72, 80);
	if (x >= 18 && x <= 20 && y == 23)
		color = trim;
	return color;
}

static uint32_t StarWarsMaterial(int x, int y, bool enemy, int phase)
{
	int dy = abs(y - 24);
	if (enemy) {
		int distance = (x - 25) * (x - 25) + (y - 24) * (y - 24);
		if (distance <= 65)
			return distance < 22 ? RGB(35, 49, 58)
			                     : RGB(151, 162, 170);
		if ((x >= 7 && x <= 12) || (x >= 36 && x <= 41)) {
			if (dy <= 15 && dy <= 11 + (x % 7))
				return (y % 7 == 0 || x == 8 || x == 39)
				         ? RGB(164, 177, 190)
				         : RGB(61, 72, 89);
		}
		if (x >= 12 && x <= 36 && dy <= 2)
			return RGB(125, 143, 156);
		return 0;
	}

	// Offset far wings above the body and near wings below it.
	uint32_t color = 0;
	int far_upper = 10 + (x - 12) * 2 / 3;
	int far_lower = 28 - (x - 12) / 3;
	int near_upper = 16 + (x - 8) / 3;
	int near_lower = 39 - (x - 8) * 2 / 3;
	if (x >= 12 && x <= 31 &&
	    (abs(y - far_upper) <= 2 || abs(y - far_lower) <= 1))
		color = RGB(118, 134, 151);
	if (x >= 8 && x <= 30 &&
	    (abs(y - near_upper) <= 2 || abs(y - near_lower) <= 2))
		color =
		    x >= 16 && x <= 20 ? RGB(187, 62, 52) : RGB(206, 213, 215);
	if (x >= 7 && x <= 23 && (y == 10 || y == 16 || y == 28 || y == 39))
		color = RGB(167, 181, 192);
	if (x >= 9 && x <= 44 && dy <= 5 - (x - 9) / 9) {
		color = y < 24 ? RGB(230, 233, 225) : RGB(153, 169, 179);
		if (x >= 29 && x <= 35 && y <= 23)
			color = RGB(57, 99, 124);
		if (x >= 17 && x <= 29 && y == 25)
			color = RGB(188, 61, 48);
	}
	if (x >= 5 && x <= 14 && ((y >= 17 && y <= 20) || (y >= 29 && y <= 32)))
		color =
		    x <= 7 + phase ? RGB(255, 139, 198) : RGB(117, 132, 150);
	return color;
}

static uint32_t FlightMaterial(int x, int y, bool enemy, int phase)
{
	if (enhanced_theme == THEME_STAR_WARS)
		return StarWarsMaterial(x, y, enemy, phase);
	if (enhanced_theme == THEME_BIPLANE)
		return PlaneMaterial(x, y, enemy, phase);
	if (enhanced_theme == THEME_MODERN_DAY)
		return JetMaterial(x, y, enemy);

	int height = abs(y - 24);
	uint32_t trim = enemy ? RGB(224, 87, 69) : RGB(79, 206, 222);
	if (x >= 3 && x < 10 && height <= (phase ? 3 : 2))
		return x < 6 ? RGB(91, 135, 250) : RGB(192, 240, 255);
	if (x >= 9 && x <= 43 && height <= (43 - x) / 5 + 1) {
		if (x >= 26 && x <= 33 && y >= 20 && y <= 23)
			return RGB(42, 83, 111);
		if (x >= 13 && x < 19)
			return trim;
		return y < 24 ? RGB(218, 226, 224) : RGB(118, 141, 150);
	}
	if (x >= 8 && x <= 31 && height >= 5 && height <= (34 - x) / 3 + 3)
		return height >= 9 ? trim : RGB(166, 182, 198);
	return 0;
}

static void Plane(uint32_t *p, unsigned int pitch, int n)
{
	int x, y, angle = (frame.objects[n].angle % ANGLES + ANGLES) % ANGLES;
	int c = COS(angle), s = SIN(angle);
	bool enemy = frame.objects[n].color != FACTION_PLAYER1;
	bool wreck = enhanced_theme != THEME_BIPLANE &&
	             frame.objects[n].state == CRASHED;
	bool damaged = enhanced_theme != THEME_BIPLANE &&
	               (wreck || frame.objects[n].state == FALLING ||
	                frame.objects[n].state == WOUNDED ||
	                frame.objects[n].state == WOUNDSTALL);
	uint32_t sprite[48][48];
	int width = frame.objects[n].symbol->w * S;
	int height = frame.objects[n].symbol->h * S;
	for (y = 0; y < 48; ++y) {
		for (x = 0; x < 48; ++x) {
			int dx = x - 24, dy = y - 24;
			int sx = 24 + (dx * c - dy * s) / 256;
			int sy = 24 + (dx * s + dy * c) / 256;
			if (frame.objects[n].inverted)
				sy = 47 - sy;
			uint32_t material =
			    FlightMaterial(sx, sy, enemy, frame.tick & 1);
			if (wreck && (sx > 34 || (sx > 20 && sy < 24)))
				material = 0;
			if (material && damaged)
				material =
				    (sx + sy + frame.tick) % 13 == 0
				        ? ((enhanced_theme == THEME_MOON ||
				            enhanced_theme == THEME_STAR_WARS)
				               ? RGB(138, 225, 255)
				               : RGB(255, 150, 67))
				        : Shade(material, wreck ? -60 : -28);
			sprite[y][x] = material;
		}
	}
	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			int sx = x * 48 / width, sy = y * 48 / height;
			uint32_t color = sprite[sy][sx];
			if (!color && ((sx > 0 && sprite[sy][sx - 1]) ||
			               (sx < 47 && sprite[sy][sx + 1]) ||
			               (sy > 0 && sprite[sy - 1][sx]) ||
			               (sy < 47 && sprite[sy + 1][sx]))) {
				color = RGB(32, 42, 47);
			}
			if (color != 0)
				Pixel(p, pitch, frame.objects[n].x * S + x,
				      frame.objects[n].y * S + y, color);
		}
	}
}

static void ThemedTarget(uint32_t *p, unsigned int pitch, int n)
{
	const sopsym_t *sym = frame.objects[n].symbol;
	int width = sym->w * S, height = sym->h * S;
	int kind = frame.objects[n].inverted;
	bool moon =
	    (enhanced_theme == THEME_MOON || enhanced_theme == THEME_STAR_WARS);
	bool vehicle = frame.objects[n].type == OX || kind == TARGET_TANK ||
	               kind == TARGET_TRUCK || kind == TARGET_TANKER_TRUCK;
	bool fuel = kind == TARGET_OIL_TANK;
	uint32_t edge = RGB(34, 43, 49);
	uint32_t wall = moon ? RGB(166, 178, 195) : RGB(157, 166, 151);
	uint32_t roof = moon ? RGB(205, 216, 229) : RGB(101, 127, 137);
	int x, y;

	for (y = 0; y < height; ++y) {
		int v = y * 48 / height;
		for (x = 0; x < width; ++x) {
			int u = x * 48 / width;
			uint32_t color = 0;
			if (frame.objects[n].state != STANDING) {
				int rubble = 34 + (u / 6 * 7) % 10;
				if (u >= 5 && u <= 43 && v >= rubble && v <= 46)
					color = v == rubble ? roof : edge;
			} else if (vehicle) {
				if (v >= 35 && u >= 4 && u <= 44)
					color =
					    v >= 43 || u % 8 == 0 ? edge : wall;
				if (v >= 28 && v < 38 && u >= 6 && u <= 42)
					color = moon ? roof : RGB(103, 127, 71);
				if (v >= 21 && v < 30 && u >= 14 && u <= 33)
					color = roof;
				if (v >= 23 && v < 26 && u >= 29 && u <= 46)
					color =
					    moon ? RGB(116, 223, 239) : edge;
			} else if (u >= 4 && u <= 43 && v >= 12 && v <= 46) {
				int dx = u - 24, dy = v - 35;
				if (moon && v < 32 &&
				    dx * dx + dy * dy > 22 * 22)
					continue;
				color = u <= 5 || u >= 42 || v >= 44
				          ? edge
				          : (v < 22 ? roof : wall);
				if (v >= 26 && v <= 32 && u % 11 >= 4)
					color = moon ? RGB(84, 206, 221)
					             : RGB(54, 76, 89);
				if (fuel && v >= 34 && v <= 37)
					color = RGB(210, 127, 72);
				else if (u >= 21 && u <= 29 && v >= 36)
					color = edge;
			}
			if (color)
				Pixel(p, pitch, frame.objects[n].x * S + x,
				      frame.objects[n].y * S + y, color);
		}
	}
}

static void ThemedDebris(uint32_t *p, unsigned int pitch, int n)
{
	const sopsym_t *sym = frame.objects[n].symbol;
	bool moon =
	    (enhanced_theme == THEME_MOON || enhanced_theme == THEME_STAR_WARS);
	int cx = frame.objects[n].x * S + sym->w * S / 2;
	int cy = frame.objects[n].y * S + sym->h * S / 2;
	int radius = 2 + clamp_range(0, frame.objects[n].life, 5);
	int angle = (frame.objects[n].inverted * 2 + frame.tick) & 15;
	int dx = COS(angle), dy = SIN(angle);
	for (int step = -radius; step <= radius; ++step) {
		int x = cx + dx * step / 256;
		int y = cy + dy * step / 256;
		uint32_t color = moon ? RGB(116, 171, 245) : RGB(211, 103, 52);
		Pixel(p, pitch, x, y, color);
		Pixel(p, pitch, x, y + 1, color);
	}
	Pixel(p, pitch, cx, cy, moon ? RGB(217, 247, 255) : RGB(255, 226, 160));
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
	if (enhanced_theme == THEME_STAR_WARS && type == SHOT) {
		uint32_t bolt =
		    player_color ? RGB(255, 83, 67) : RGB(110, 255, 119);
		int px = frame.objects[n].x * S, py = frame.objects[n].y * S;
		for (int i = -2; i <= 4; ++i)
			Pixel(p, pitch, px + i, py + 1, bolt);
		Pixel(p, pitch, px, py + 1, RGB(255, 245, 222));
		return;
	}
	if (enhanced_theme != THEME_BIPLANE && type == EXPLOSION) {
		ThemedDebris(p, pitch, n);
		return;
	}
	if (type == PLANE && enhanced_theme != THEME_BIPLANE) {
		Plane(p, pitch, n);
		return;
	}
	if (type == PLANE &&
	    (frame.objects[n].state == FLYING ||
	     frame.objects[n].state == WOUNDED ||
	     frame.objects[n].state == STALLED ||
	     frame.objects[n].state == WOUNDSTALL) &&
	    sym->w == 16 && sym->h == 16) {
		Plane(p, pitch, n);
		return;
	}
	if ((enhanced_theme == THEME_MOON ||
	     enhanced_theme == THEME_STAR_WARS) &&
	    type == OX && frame.objects[n].state == STANDING) {
		ThemedTarget(p, pitch, n);
		return;
	}
	if (enhanced_theme != THEME_BIPLANE && type == TARGET && sym->w == 16 &&
	    sym->h == 16 && frame.objects[n].inverted <= TARGET_TANKER_TRUCK) {
		ThemedTarget(p, pitch, n);
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
				color = (enhanced_theme == THEME_MOON ||
				         enhanced_theme == THEME_STAR_WARS)
				          ? RGB(127, 157, 195)
				          : RGB(123, 132, 127);
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
			if (enhanced_settings_frame) {
				color =
				    RGB(18 + y / 90, 31 + y / 55, 42 + y / 50);
				if (y < 32 * S)
					color = RGB(30, 60, 72);
				else if (y == 32 * S || y == 176 * S)
					color = RGB(68, 113, 119);
			}
			unsigned int ink = ui[(y / S) * pitch + x / S] & 3;
			pixels[y * W + x] = ink ? inks[ink] : color;
		}
	}
}
