// Modified 2026-11-12: first playable title, music and round results.
// Copyright (C) 2026 Sopwith Update contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SOPWITH_ENHANCED_H
#define SOPWITH_ENHANCED_H
#include "sw.h"
#define ENHANCED_SCALE  3
#define ENHANCED_WIDTH  (SCR_WDTH * ENHANCED_SCALE)
#define ENHANCED_HEIGHT (SCR_HGHT * ENHANCED_SCALE)
// Capture copies presentation state. The renderer never owns game objects or
// masks.
void Enhanced_ClearFrame(void);
bool Enhanced_HasFrame(void);
void Enhanced_Capture(const uint8_t *ui, unsigned int pitch, int camera,
                      const GRNDTYPE *terrain, const OBJECTS *objects,
                      int tick);
// Pixels are opaque 0xAARRGGBB, pitch measured in pixels.
void Enhanced_Render(uint32_t *pixels, unsigned int pitch);
extern bool vid_modern_title;
extern bool enhanced_title_frame;
extern bool enhanced_settings_frame;
void Enhanced_RenderTitle(uint32_t *pixels, const uint8_t *ui,
                          unsigned int pitch);
#endif
