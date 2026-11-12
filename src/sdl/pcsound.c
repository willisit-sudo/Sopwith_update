// Modified 2026-11-12: switchable synthesized audio.
// Modified 2026-11-12: first playable title, music and round results.
// Modified 2026-11-12: switchable synthesized audio.
//
// Copyright(C) 2001-2005 Simon Howard
//
// You can redistribute and/or modify this program under the terms of the
// GNU General Public License version 2 as published by the Free Software
// Foundation, or any later version. This program is distributed WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.
//
//
// SDL PC speaker code. This emulates a PC speaker using SDL's audio API
//

#include <SDL.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

#include "sw.h"
#include "swmain.h"
#include "swsound.h"

#define VOLUME            4000 /* out of 1 << 15 */
#define FILTER_KERNEL_LEN 51
#define OVERSAMPLE_FACTOR 25

// The following values give the cutoff range for our band-pass
// filter. These frequencies come from a frequency analysis of
// the LGR video "Playing Silpheed on an IBM 5150 PC".
#define FILTER_LOW_CUTOFF_FREQ 1500 /* hz */
#define FILTER_HI_CUTOFF_FREQ  6000 /* hz */

#define TIMER_FREQ 1193280

struct filter {
	float *kernel;
	size_t kernel_len;
	// Circular buffer of recent samples:
	float *samples;
	int samples_next;
};

bool snd_tinnyfilter = 1;
bool snd_enhanced;
bool snd_modern_music = true;
bool snd_loss_cue = true;
static unsigned int loss_sample;
static bool loss_playing;
static bool modern_title_playing;
static SDL_atomic_t title_note;
static int rendered_title_note = -1;
static float title_phase, title_frequency, title_age, title_gain;
static bool sound_initted = false;
static bool speaker_on = false;
static SDL_AudioDeviceID audio_dev;
static int output_freq;
static struct filter tinny_filter;
static float current_freq = 0xff;

static float FilterNext(struct filter *f, float sample)
{
	float accum;
	int i, sidx;

	f->samples[f->samples_next] = sample;
	f->samples_next = (f->samples_next + 1) % f->kernel_len;

	accum = 0;
	sidx = f->samples_next;
	for (i = 0; i < f->kernel_len; ++i) {
		accum += f->kernel[i] * f->samples[sidx];
		sidx = (sidx + 1) % f->kernel_len;
	}

	return accum;
}

static float SincFunction(int idx, int kernel_len, float freq)
{
	// Relative to center of filter kernel.
	idx -= (kernel_len - 1) / 2;
	if (idx == 0) {
		return 1 / M_PI;
	}
	return sin(2 * M_PI * freq * idx) / (idx * M_PI);
}

// https://en.wikipedia.org/wiki/Window_function#Blackman_window
static float BlackmanFunction(int idx, int kernel_len)
{
	return 0.42 - 0.5 * cos(2 * M_PI * idx / (kernel_len - 1)) +
	       0.08 * cos(4 * M_PI * idx / (kernel_len - 1));
}

// Initialize f as a lowpass filter.
static void MakeLowPassFilter(struct filter *f, float sample_rate, float cutoff)
{
	int i;

	// Kernel length must be an odd number.
	assert((f->kernel_len % 2) == 1);

	f->kernel = checked_calloc(f->kernel_len, sizeof(float));
	f->samples = checked_calloc(f->kernel_len, sizeof(float));
	f->samples_next = 0;

	// For a good intro to how this works, read "The Scientist
	// and Engineer's Guide to Digital Signal Processing" chapter 16.
	for (i = 0; i < f->kernel_len; ++i) {
		// The sinc() function gives the inverse fourier
		// transform of an ideal lowpass filter that rejects
		// everything below the cutoff frequency. Frequency
		// is expressed as a ratio of the sample rate, so will
		// always be in the range 0.0-0.5.
		f->kernel[i] =
		    SincFunction(i, f->kernel_len, cutoff / sample_rate);
		// It is impossible to realise an ideal lowpass filter
		// in practice, and trying to do so produces a
		// frequency response with ringing around the cutoff
		// frequency. To mitigate this we use a windowing
		// function to smooth the frequency response.
		f->kernel[i] *= BlackmanFunction(i, f->kernel_len);
	}
}

// Spectrally invert given filter.
static void InvertFilter(struct filter *f)
{
	float sum;
	int i;

	sum = 0;
	for (i = 0; i < f->kernel_len; ++i) {
		sum += f->kernel[i];
	}
	for (i = 0; i < f->kernel_len; ++i) {
		f->kernel[i] /= sum;
	}

	// From "The Scientist and Engineer's Guide to Digital Signal
	// Processing", chapter 14:
	// > Example of spectral inversion. [...] A high-pass filter kernel is
	// > formed by changing the sign of each sample in (a), and adding one
	// > to the sample at the center of symmetry. This action in the time
	// > domain inverts frequency spectrum (ie. flips it top-for-bottom)
	for (i = 0; i < f->kernel_len; ++i) {
		f->kernel[i] = -f->kernel[i];
	}
	f->kernel[(f->kernel_len - 1) / 2] += 1.0;
}

// Initialize f as a highpass filter.
static void MakeHighPassFilter(struct filter *f, float sample_rate,
                               float cutoff)
{
	MakeLowPassFilter(f, sample_rate, cutoff);

	// See comment in InvertFilter() above.
	// Or in other words: lowpass(f) + highpass(f) = 1.0, so
	// highpass(f) = 1.0 - lowpass(f)
	InvertFilter(f);
}

static void FreeFilter(struct filter *f)
{
	free(f->kernel);
	free(f->samples);
}

static void AddFilters(struct filter *f1, struct filter *f2)
{
	int i;

	assert(f1->kernel_len == f2->kernel_len);
	for (i = 0; i < f1->kernel_len; ++i) {
		f1->kernel[i] += f2->kernel[i];
	}
}

static inline float SquareWave(float time)
{
	int l = (int) time;
	return time - l < 0.5 ? -0.5 : 0.5;
}

// In generating samples, we oversample the square wave output; this helps
// to avoid aliasing artifacts.
static float OversampledSquareWave(int i)
{
	float presample;
	int j;

	presample = 0;
	for (j = 0; j < OVERSAMPLE_FACTOR; j++) {
		float t = i + ((float) j) / OVERSAMPLE_FACTOR;
		t = (t * current_freq) / output_freq;
		presample += SquareWave(t);
	}
	return presample / ((float) OVERSAMPLE_FACTOR);
}

// The game thread publishes targets and effect triggers under the SDL audio
// device lock. Oscillators and envelopes advance only in the audio callback.
static struct {
	bool active;
	float engine_target, engine_gain, engine_pitch, pitch_target, phase;
	float shot, impact, rumble, impact_phase;
	uint32_t noise;
} enhanced = {.noise = 0x7149f6a1u};

void Speaker_EnhancedFrame(bool active, int throttle, bool engine)
{
	if (!sound_initted) {
		return;
	}
	SDL_LockAudioDevice(audio_dev);
	enhanced.active = snd_enhanced && active;
	enhanced.engine_target = engine ? 1.0f : 0.0f;
	enhanced.pitch_target =
	    48.0f + 13.0f * clamp_range(0, throttle, MAX_THROTTLE);
	if (!enhanced.active) {
		enhanced.engine_gain = enhanced.shot = enhanced.impact = 0;
	}
	SDL_UnlockAudioDevice(audio_dev);
}

void Speaker_EnhancedEffect(bool impact)
{
	if (!sound_initted || !snd_enhanced || !soundflg) {
		return;
	}
	SDL_LockAudioDevice(audio_dev);
	if (impact) {
		enhanced.impact = 1.0f;
		enhanced.impact_phase = 0;
	} else {
		enhanced.shot = 1.0f;
	}
	SDL_UnlockAudioDevice(audio_dev);
}

static void MixEnhanced(int16_t *stream, int count)
{
	float dt = 1.0f / output_freq;
	float shot_decay = expf(-52.0f * dt);
	float impact_decay = expf(-7.0f * dt);
	int i;

	for (i = 0; i < count; ++i) {
		float noise, rotor, sample;
		enhanced.noise ^= enhanced.noise << 13;
		enhanced.noise ^= enhanced.noise >> 17;
		enhanced.noise ^= enhanced.noise << 5;
		noise = (enhanced.noise >> 16) / 32768.0f - 1.0f;
		enhanced.rumble += (noise - enhanced.rumble) * (700.0f * dt);
		enhanced.engine_gain +=
		    (enhanced.engine_target - enhanced.engine_gain) *
		    (24.0f * dt);
		enhanced.engine_pitch +=
		    (enhanced.pitch_target - enhanced.engine_pitch) *
		    (18.0f * dt);
		enhanced.phase += enhanced.engine_pitch * dt;
		if (enhanced.phase >= 1.0f) {
			enhanced.phase -= 1.0f;
		}
		rotor = sinf(2.0f * M_PI * enhanced.phase) +
		        0.43f * sinf(4.0f * M_PI * enhanced.phase) +
		        0.20f * sinf(10.0f * M_PI * enhanced.phase);
		enhanced.impact_phase += (38.0f + 53.0f * enhanced.impact) * dt;
		if (enhanced.impact_phase >= 1.0f) {
			enhanced.impact_phase -= 1.0f;
		}
		sample =
		    enhanced.engine_gain *
		        (0.09f * rotor + 0.08f * enhanced.rumble) +
		    enhanced.shot * (0.23f * noise + 0.11f * enhanced.rumble) +
		    enhanced.impact *
		        (0.21f * sinf(2.0f * M_PI * enhanced.impact_phase) +
		         0.53f * enhanced.rumble);
		enhanced.shot *= shot_decay;
		enhanced.impact *= impact_decay;
		stream[i] = (int16_t) (sample * 24000.0f);
	}
}

void Speaker_TitleMusic(bool playing)
{
	if (!sound_initted)
		return;
	SDL_LockAudioDevice(audio_dev);
	bool enabled = playing && snd_modern_music && soundflg;
	if (enabled && !modern_title_playing)
		rendered_title_note = -1;
	modern_title_playing = enabled;
	SDL_UnlockAudioDevice(audio_dev);
}

// Both title voices use the same sequencer, including custom mission tunes.
void Speaker_TitleNote(unsigned short count)
{
	// Retrigger repeated notes as well as changes in pitch.
	int retrigger = (SDL_AtomicGet(&title_note) ^ 0x10000) & 0x10000;
	SDL_AtomicSet(&title_note, retrigger | count);
}

static void MixTitleMusic(int16_t *stream, int count)
{
	int note = SDL_AtomicGet(&title_note);
	unsigned int divisor = note & 0xffff;
	float dt = 1.0f / output_freq;
	if (note != rendered_title_note) {
		rendered_title_note = note;
		title_age = 0;
		if (divisor)
			title_frequency = TIMER_FREQ / (float) divisor;
	}

	for (int i = 0; i < count; ++i) {
		float target =
		    divisor ? 0.42f + 0.58f * expf(-6.0f * title_age) : 0;
		title_gain += (target - title_gain) * (200.0f * dt);
		title_age += dt;
		title_phase += title_frequency * dt;
		if (title_phase >= 2.0f)
			title_phase -= 2.0f;
		float phase = 2.0f * M_PI * title_phase;
		float sample =
		    0.16f * sinf(phase) + 0.042f * sinf(2.0f * phase) +
		    0.018f * sinf(3.0f * phase) + 0.057f * sinf(0.5f * phase);
		stream[i] = (int16_t) (sample * title_gain * 24000.0f);
	}
}

void Speaker_PlaneLoss(bool playing)
{
	if (!sound_initted || (playing && (!snd_loss_cue || !soundflg)))
		return;
	SDL_LockAudioDevice(audio_dev);
	loss_playing = playing;
	loss_sample = 0;
	SDL_UnlockAudioDevice(audio_dev);
}

static void MixPlaneLoss(int16_t *stream, int count)
{
	static const float melody[] = {440.0f, 349.23f, 293.66f, 220.0f};
	unsigned int note_samples = output_freq / 5;
	for (int i = 0; i < count && loss_playing; ++i) {
		unsigned int note = loss_sample / note_samples;
		float time = (loss_sample % note_samples) / (float) output_freq;
		float phase = 2.0f * M_PI * melody[note] * time;
		float envelope = fminf(time * 125.0f, 1.0f) *
		                 fminf((0.2f - time) * 20.0f, 1.0f) *
		                 expf(-6.0f * time);
		int mixed = stream[i] +
		            (int) (4900.0f * envelope *
		                   (sinf(phase) + 0.24f * sinf(phase * 2.0f)));
		stream[i] = (int16_t) clamp_range(-32768, mixed, 32767);
		if (++loss_sample == note_samples * arrlen(melody))
			loss_playing = false;
	}
}

// SDL callback function to generate sound output
static void SoundCallback(void *userdata, Uint8 *stream8, int len)
{
	static int lasttime;
	static float lastfreq;
	int16_t *stream = (int16_t *) stream8;
	float sample;
	int i;

	len /= 2; // 2 bytes per sample

	swsndupdate();

	// lasttime stores the time offset from the last call.
	// We save the time so that the multiple time slices all fit together
	// smoothly. If we have changed frequency since last time, we need to
	// adjust lasttime to the new frequency
	if (modern_title_playing) {
		MixTitleMusic(stream, len);
		lasttime = 0;
		return;
	}
	if (enhanced.active) {
		MixEnhanced(stream, len);
		MixPlaneLoss(stream, len);
		lasttime = 0;
		return;
	}
	if (current_freq > 0.1f) {
		lasttime *= lastfreq / current_freq;
	} else {
		lasttime = 0;
	}

	for (i = 0; i < len; ++i) {
		if (!speaker_on) {
			sample = 0;
		} else {
			sample = OversampledSquareWave(i + lasttime);
		}

		sample = FilterNext(&tinny_filter, sample);
		stream[i] = (signed int) (sample * VOLUME);
	}

	MixPlaneLoss(stream, len);
	lasttime += len;
	if (current_freq > 0.1) {
		lasttime = lasttime % (int) (1000 * output_freq / current_freq);
	}
	lastfreq = current_freq;
}

void Speaker_Output(unsigned short count)
{
	if (!count) {
		current_freq = 0;
		speaker_on = false;
		return;
	}
	speaker_on = true;
	current_freq = TIMER_FREQ / (float) count;
}

void Speaker_Off(void)
{
	speaker_on = false;
}

void Speaker_Shutdown(void)
{
	if (!sound_initted) {
		return;
	}

	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	sound_initted = false;
}

static void InitializeTinnyFilter(unsigned int sample_rate)
{
	struct filter hp_filter;

	// We use a band-pass filter as a model for the PC speaker. Rationale
	// is as follows: the PC speaker is a small, low-quality speaker with
	// no good low-end response; the speaker is usually inside a metal PC
	// case that muffles any high-end response.

	// To get a band-pass filter, we initialize a band-stop filter for
	// our desired frequency range, then spectrally invert it.
	tinny_filter.kernel_len = FILTER_KERNEL_LEN;
	MakeLowPassFilter(&tinny_filter, (float) sample_rate,
	                  FILTER_LOW_CUTOFF_FREQ);

	hp_filter.kernel_len = FILTER_KERNEL_LEN;
	MakeHighPassFilter(&hp_filter, (float) sample_rate,
	                   FILTER_HI_CUTOFF_FREQ);

	AddFilters(&tinny_filter, &hp_filter);
	InvertFilter(&tinny_filter);

	FreeFilter(&hp_filter);
}

static void InitializeNullFilter(void)
{
	static float null_kernel, one_sample;
	tinny_filter.kernel_len = 1;
	tinny_filter.kernel = &null_kernel;
	tinny_filter.samples = &one_sample;
	tinny_filter.samples_next = 0;

	// We want relatively similar volume whether the filter is on or not.
	// The filter makes things quite a bit quieter.
	null_kernel = 0.4;
	one_sample = 0.0;
}

// initialize sound
void Speaker_Init(void)
{
	static SDL_AudioSpec audiospec, audiospec_actual;

	if (sound_initted) {
		return;
	}

	SDL_Init(SDL_INIT_AUDIO);

	audiospec.samples = 1024;
	audiospec.freq = 48000;
	audiospec.format = AUDIO_S16SYS;
	audiospec.channels = 1;
	audiospec.callback = &SoundCallback;

	audio_dev = SDL_OpenAudioDevice(NULL, 0, &audiospec, &audiospec_actual,
	                                SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	if (audio_dev == 0) {
		fprintf(stderr, "Failed to initialize sound: %s\n",
		        SDL_GetError());
		return;
	}

	output_freq = audiospec_actual.freq;

	atexit(Speaker_Shutdown);
	sound_initted = true;
	if (snd_tinnyfilter) {
		InitializeTinnyFilter(audiospec.freq);
	} else {
		InitializeNullFilter();
	}

	SDL_PauseAudioDevice(audio_dev, 0);
}
