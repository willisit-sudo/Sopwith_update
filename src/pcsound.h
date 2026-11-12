// Modified 2026-11-12: switchable synthesized audio.
// Modified 2026-11-12: first playable title, music and round results.
// Modified 2026-11-12: switchable synthesized audio.
//
// Copyright(C) 1984-2000 David L. Clark
// Copyright(C) 2001-2005 Simon Howard
//
// You can redistribute and/or modify this program under the terms of the
// GNU General Public License version 2 as published by the Free Software
// Foundation, or any later version. This program is distributed WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.
//
//
// PC Speaker Interface
//

#ifndef __SDLSOUND_H__
#define __SDLSOUND_H__

#include "std.h"

void Speaker_Init(void);
void Speaker_Off(void);

//
// Play a particular tone to the speaker
//
// count: This determines the frequency. count is
//        the number of clock ticks between speaker
//        pulses. Hence clock_freq/count == tone
//        frequency
//

void Speaker_Output(unsigned short count);

extern bool snd_tinnyfilter;
extern bool snd_enhanced;
extern bool snd_modern_music;
extern bool snd_loss_cue;
void Speaker_PlaneLoss(bool playing);
void Speaker_TitleMusic(bool playing);
void Speaker_TitleNote(unsigned short count);

// Called from the game thread.
void Speaker_EnhancedFrame(bool active, int throttle, bool engine);
void Speaker_EnhancedEffect(bool impact);

#endif
