// wplAudio
//
// Includes: 
//  - A mixer that runs nicely on callback based audio systems (ie, SDL)
//  - Wrappers that correctly lock/unlock audio devices for playing samples
//  - Some utility functions that wrap dr_wav and stb_vorbis for loading sounds
//
// Note:
// 	- Doesn't support streaming from disk, however, you can free the input for
// 	  wLoadSampleFrom***, and streams only store ~32k of data at time
//
// Using: sts_mixer.h - v0.01
// written 2016 by Sebastian Steinhauer
// sts_mixer was committed to the public domain.
// editied 2018 by William Bundy
//
// Notes: 
// 	- Samples are assumed to be mono
// 	- Streams are assumed to be stereo
// 	- The output assumes 2 channel, 44.1khz, 32-bit floating point audio
// 	  This cuts down on code size and complexity
//
// Changelog:
// 	- Reformatted code
// 	- foating point only; removed other input/output modes
// To do:
//  - SIMD-ize mixing
//  - Potentially keep a free list of unused voices
//    and a list of currently playing voices? This would be
//    a good use of wMemoryPool, for example


#define DR_WAV_NO_STDIO
#define DRWAV_ASSERT(...)
#define DR_WAV_IMPLEMENTATION
#include "thirdparty/dr_wav.h"

#ifdef WPL_REPLACE_CRT
#define WBTM_CRT_REPLACE
#define WBTM_API static
#include "thirdparty/wb_tm.c"
void __chkstk() {}
#else
#include <math.h>
#endif

#define stbv_ldexp ldexp
#define stbv_exp exp
#define stbv_log log
#define stbv_pow pow
#define stbv_cos cos
#define stbv_sin sin
#define stbv_floor floor

#ifndef assert
#define assert(c) do { if(!(c)) {wLogError(0, "%s\n", #c);}} while(0)
#define WPL_VORBIS_ASSERT
#endif

// I'm using a modified stb_vorbis that solves some problems with No-CRT builds
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API
#include "thirdparty/stb_vorbis.c"

#ifdef WPL_VORBIS_ASSERT
#undef assert
#undef WPL_VORBIS_ASSERT
#endif

enum {
	wMixer_VoiceStopped,
	wMixer_VoicePlaying,
	wMixer_VoiceStreaming
};


static 
f32 mixerClamp(f32 value, f32 min, f32 max)
{
	if (value < min) return min;
	else if (value > max) return max;
	else return value;
}

static 
f32 mixerClamp1(f32 sample) 
{
	if (sample < -1.0f) return -1.0f;
	else if (sample > 1.0f) return 1.0f;
	else return sample;
}


static 
f32 mixerSample(wMixerSample* sample, usize position)
{
	return ((f32*)sample->data)[position];
}


static
void mixerResetVoice(wMixer* mixer, i32 i) 
{
	wMixerVoice*  voice = &mixer->voices[i];
	voice->state = wMixer_VoiceStopped;
	voice->sample = NULL;
	voice->stream = NULL;
	voice->position = 0.0f;
	voice->gain = 0.0f;
	voice->pitch = 0.0f;
	voice->pan = 0.0f;
}

static 
i32 mixerFindFreeVoice(wMixer* mixer) 
{
	for(isize i = 0; i < mixer->voiceCount; ++i) {
		if(mixer->voices[i].state == wMixer_VoiceStopped) {
			return i;
		}
	}
	return -1;
}

void wMixerInit(wMixer* mixer, isize voiceCount, wMixerVoice* voices)
{
	mixer->frequency = 44100;
	mixer->gain = 0.5f;

	mixer->voiceCount = voiceCount;
	mixer->voices = voices;
	for (isize i = 0; i < voiceCount; ++i) {
		mixerResetVoice(mixer, i);
	}
}

i32 wMixerGetActiveVoices(wMixer* mixer) 
{
	isize i, active;
	for (i = 0, active = 0; i < mixer->voiceCount; ++i) {
		if (mixer->voices[i].state != wMixer_VoiceStopped) {
			++active;
		}
	}
	return active;
}

i32 wMixerInternalPlaySample(wMixer* mixer,
		wMixerSample* sample,
		f32 gain, f32 pitch, f32 pan)
{
	i32 i;
	wMixerVoice* voice;

	i = mixerFindFreeVoice(mixer);
	if (i >= 0) {
		voice = &mixer->voices[i];
		voice->gain = gain;
		voice->pitch = mixerClamp(pitch, 0.1f, 10.0f);
		voice->pan = mixerClamp(pan * 0.5f, -0.5f, 0.5f);
		voice->position = 0.0f;
		voice->sample = sample;
		voice->stream = NULL;
		voice->state = wMixer_VoicePlaying;
	}
	return i;
}

i32 wMixerInternalPlayStream(wMixer* mixer, wMixerStream* stream, f32 gain) 
{
	i32 i = mixerFindFreeVoice(mixer);
	if (i >= 0) {
		wMixerVoice* voice = mixer->voices + i;
		voice->gain = gain;
		voice->position = 0.0f;
		voice->sample = NULL;
		voice->stream = stream;
		voice->state = wMixer_VoiceStreaming;
	}
	return i;
}

void wMixerStopVoice(wMixer* mixer, i32 voice) 
{
	if (voice >= 0 && voice < mixer->voiceCount) {
		mixerResetVoice(mixer, voice);
	}
}

void wMixerStopSample(wMixer* mixer, wMixerSample* sample) 
{
	for(isize i = 0; i < mixer->voiceCount; ++i) {
		if(mixer->voices[i].sample == sample) {
			mixerResetVoice(mixer, i);
		}
	}
}

void wMixerStopStream(wMixer* mixer, wMixerStream* stream) 
{
	for(isize i = 0; i < mixer->voiceCount; ++i) {
		if (mixer->voices[i].stream == stream) {
			mixerResetVoice(mixer, i);
		}
	}
}

void wMixerMixAudio(wMixer* mixer, f32* output, u32 samples) 
{
	wMixerVoice*  voice;
	u32 i, position;
	f32 left, right, advance, sample;

	// mix all voices
	advance = 1.0f / (f32)mixer->frequency;
	for (; samples > 0; --samples) {
		left = 0.0f;
		right = 0.0f;

		for (i = 0; i < mixer->voiceCount; ++i) {
			voice = mixer->voices + i;

			if (voice->state == wMixer_VoicePlaying) {
				wMixerSample* vsample = voice->sample;
				position = (i32)voice->position;

				if (position < vsample->length) {
					sample = mixerClamp1(mixerSample(vsample, position) * voice->gain);
					left += mixerClamp1(sample * (0.5f - voice->pan));
					right += mixerClamp1(sample * (0.5f + voice->pan));
					voice->position += (f32)vsample->frequency * advance * voice->pitch;
				} else {
					mixerResetVoice(mixer, i);
				}

			} else if (voice->state == wMixer_VoiceStreaming) {
				wMixerSample* vsample = &voice->stream->sample;
				position = ((i32)voice->position) * 2;

				if (position >= vsample->length) {
					// buffer empty...refill
					voice->stream->callback(vsample, voice->stream->userdata);
					voice->position = 0.0f;
					position = 0;
				}

				left += mixerClamp1(mixerSample(vsample, position) * voice->gain);
				right += mixerClamp1(mixerSample(vsample, position + 1) * voice->gain);
				voice->position += (f32)vsample->frequency * advance;
			}
		}

		// write to buffer
		left = mixerClamp1(left);
		right = mixerClamp1(right);
		*output++ = left;
		*output++ = right;
	}
}

// You always want to use these from user code
void wPlaySample(wWindow* window, wMixerSample* sample, f32 gain, f32 pitch, f32 pan)
{
	wLockAudioDevice(window);
	wMixerInternalPlaySample(window->mixer, sample, gain, pitch, pan);
	wUnlockAudioDevice(window);
}

// You always want to use this from user code instead of wMixerPlay***
void wPlayStream(wWindow* window, wMixerStream* stream, f32 gain)
{
	wLockAudioDevice(window);
	wMixerInternalPlayStream(window->mixer, stream, gain);
	wUnlockAudioDevice(window);
}

wMixerSample* wWavToSample(u8* data, isize size, wMemoryArena* arena)
{
	drwav wav;
	drwav_init_memory(&wav, data, size);
	f32* buf = wArenaPush(arena, sizeof(f32) * wav.totalSampleCount);
	usize samplesDecoded = drwav_read_f32(&wav, wav.totalSampleCount, buf);
	drwav_uninit(&wav);
	wMixerSample s = {
		(u32) samplesDecoded,
		44100,
		buf
	};
	wMixerSample* ss = wArenaPush(arena, sizeof(wMixerSample));
	*ss = s;
	return ss;
}

#define wVorbisStreamSamples 4096
void wVorbisCallback(wMixerSample* sample, void* user)
{
	stb_vorbis* f = user;
	i32 size = stb_vorbis_get_samples_float_interleaved(
			f, 
			2,
			sample->data,
			sample->length);
	if(size < wVorbisStreamSamples) {
		stb_vorbis_seek_start(f);
	}
}

wMixerStream* wOggToStream(u8* data, isize size, wMemoryArena* arena)
{
	i32 error = 0;
	stb_vorbis_alloc vba;
	vba.alloc_buffer = wArenaPush(arena, CalcMegabytes(256));
	vba.alloc_buffer_length_in_bytes = CalcMegabytes(256);

	stb_vorbis* vorb = stb_vorbis_open_memory(data, size, &error, &vba);
	if(vorb == NULL) {
		wLogError(0, "Error opening ogg stream: %d\n", error);
		return NULL;
	}
	wMixerStream* ss = wArenaPush(arena, sizeof(wMixerStream));
	ss->userdata = vorb;
	ss->callback = wVorbisCallback;
	ss->sample.length = wVorbisStreamSamples * 2;
	ss->sample.frequency = vorb->sample_rate;
	ss->sample.data = wArenaPush(arena, sizeof(f32) * wVorbisStreamSamples * 2);
	wVorbisCallback(&ss->sample, vorb);
	return ss;
}

wMixerSample* wOggToSample(u8* data, isize size, wMemoryArena* arena)
{
	i32 error = 0;
	stb_vorbis_alloc vba;
	vba.alloc_buffer = wArenaPush(arena, CalcKilobytes(256));
	vba.alloc_buffer_length_in_bytes = CalcKilobytes(256);

	stb_vorbis* vorbis = stb_vorbis_open_memory(data, size, &error, &vba);
	if(vorbis == NULL) {
		wLogError(0, "Error opening ogg stream: %d\n", error);
		return NULL;
	}
	wMixerSample* sample = wArenaPush(arena, sizeof(wMixerSample));
	sample->length = stb_vorbis_stream_length_in_samples(vorbis);
	sample->frequency = vorbis->sample_rate;
	sample->data = wArenaPush(arena, sizeof(f32) * sample->length);
	stb_vorbis_get_samples_float_interleaved(
			vorbis, 
			1,
			sample->data,
			sample->length);
	return sample;
}


