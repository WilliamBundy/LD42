
#ifdef WPL_WINDOWS
#include <Windows.h>
#define WB_ALLOC_WINDOWS
#else
#ifdef WPL_LINUX
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/sysctl.h>
#define WB_ALLOC_LINUX
#else
#ifdef WPL_MACOS
#include <unistd.h>
#include <sys/sysctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#define WB_ALLOC_MACOS
#endif
#endif
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define WB_ALLOC_IMPLEMENTATION
#define WB_ALLOC_CUSTOM_INTEGER_TYPES
#define WB_ALLOC_BACKEND_API static
#include "thirdparty/wb_alloc.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#ifndef WPL_MACOS
#define WB_GL_IMPLEMENTATION
#define WB_GL_USE_LEGACY
#define WB_GL_USE_COMPAT
#define WB_GL_USE_CORE
#define WB_GL_SDL
#include "thirdparty/wb_gl_loader.h"
#else
//#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#endif
static int wWasInit = 0;

typedef struct 
{
	SDL_Window* windowHandle;
	SDL_AudioDeviceID audio;
} wInternalSDL;

wInternalSDL* wGetInternal(wWindow* window)
{
	return (wInternalSDL*)window->internal;
}

static
i64 wSDLInit()
{
	SDL_SetMainReady();
	int ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	wWasInit = 1;
	return ret;
}

void wQuit()
{
	SDL_Quit();
}

i64 wCreateWindow(wWindowDef* def, wWindow* window)
{

	window->internal = malloc(sizeof(wInternalSDL));
	wInternalSDL* internal = wGetInternal(window);

	if(!wWasInit) {
		wSDLInit();
	}

	window->basePath = SDL_GetBasePath();

	i64 wposx, wposy;
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
#define GLattr(attr, val) SDL_GL_SetAttribute(SDL_GL_##attr, val)
	GLattr(RED_SIZE, 8);
	GLattr(GREEN_SIZE, 8);
	GLattr(BLUE_SIZE, 8);
	GLattr(ALPHA_SIZE, 8);
	GLattr(DEPTH_SIZE, 24);
	GLattr(STENCIL_SIZE, 8);
	GLattr(DOUBLEBUFFER, 1);
	GLattr(FRAMEBUFFER_SRGB_CAPABLE, 1);
	GLattr(CONTEXT_MAJOR_VERSION, 3);
	GLattr(CONTEXT_MINOR_VERSION, 3);
	GLattr(CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	if(def->posCentered) {
		wposx = SDL_WINDOWPOS_CENTERED;
		wposy = SDL_WINDOWPOS_CENTERED;
	} else if(def->posUndefined) {
		wposx = SDL_WINDOWPOS_UNDEFINED;
		wposy = SDL_WINDOWPOS_UNDEFINED;
	} else {
		wposx = def->x;
		wposy = def->y;
	}

	if(def->width == 0) {
		def->width = 1280;
	} 

	if(def->height == 0) {
		def->height = 720;
	}

	SDL_Window* windowHandle = SDL_CreateWindow(
			def->title,
			wposx, wposy,
			def->width, def->height,
			(def->resizeable ? SDL_WINDOW_RESIZABLE : 0) |
			(def->borderless ? SDL_WINDOW_BORDERLESS : 0) |
			(def->hidden ? SDL_WINDOW_HIDDEN : 0) |
			SDL_WINDOW_OPENGL);

	if(!windowHandle) {
		fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
		return 1;
	}

	internal->windowHandle = windowHandle;

	SDL_DisplayMode dm = {0};
	SDL_GetWindowDisplayMode(windowHandle, &dm);
	if(dm.refresh_rate == 0) dm.refresh_rate = 60;
	window->refreshRate = dm.refresh_rate;

	SDL_GLContext glContext = SDL_GL_CreateContext(windowHandle);
	window->glVersion = 33;

	if(glContext == NULL) {
		fprintf(stderr, "Unable to create OpenGL context: %s\n", SDL_GetError());

		return 1;
	}

	SDL_GL_MakeCurrent(windowHandle, glContext);
	{
#ifndef WPL_MACOS
		struct wbgl_ErrorContext ctx;
		wbgl_load_all(&ctx);
#endif
		
		/*
		for(isize i = 0; i < ctx.failed_size; ++i) {
			printf("%s\n", ctx.failed[i]);
		}
		*/
		
	}

	glClearColor(0, 0, 0, 1);
	SDL_GL_SetSwapInterval(1);

	return windowHandle == NULL ? 0 : 1;
}

static void wSDLAudioCallback(void* userdata, u8* stream, i32 len)
{
	wWindow* window = userdata;
	if(window->mixer)
		wMixerMixAudio(window->mixer, (void*)stream, len / (sizeof(f32)*2));
}

void wLockAudioDevice(wWindow* window)
{
	SDL_LockAudioDevice(wGetInternal(window)->audio);
}

void wUnlockAudioDevice(wWindow* window)
{
	SDL_LockAudioDevice(wGetInternal(window)->audio);
}

void wInitAudio(wWindow* window, wMixer* mixer, i32 voices, wMemoryArena* arena)
{
	wInternalSDL* internal = wGetInternal(window);
	SDL_AudioSpec want, have;
	want.format = AUDIO_F32SYS;
	want.freq = 44100;
	want.channels = 2;
	want.userdata = window;
	want.samples = 4096;
	want.callback = wSDLAudioCallback;
	internal->audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	// TODO(will): error checking for SDL audio

	window->mixer = mixer;
	wMixerInit(window->mixer, voices,
			wArenaPush(arena, sizeof(wMixerVoice) * voices));
}


void wShowWindow(wWindow* w)
{
	SDL_ShowWindow(wGetInternal(w->internal)->windowHandle);
}

i64 wUpdate(wWindow* window, wState* state)
{
	wInternalSDL* internal = wGetInternal(window);
	window->lastTicks = SDL_GetTicks();
	wState lstate;
	SDL_Event event = {0};

	{
		int width, height;
		SDL_GetWindowSize(internal->windowHandle, &width, &height);
		state->width = width;
		state->height = height;
		glViewport(0, 0, width, height);
	}

	lstate = *state;
	lstate.exitEvent = 0;
	wInputUpdate(state->input);
	while(SDL_PollEvent(&event))  {
		switch(event.type) {
			case SDL_QUIT:
				lstate.exitEvent = 1;
				*state = lstate;
				return 0;

			case SDL_MOUSEBUTTONDOWN:
				state->input->mouse[event.button.button] = Button_JustDown;
				break;
			case SDL_MOUSEBUTTONUP:
				state->input->mouse[event.button.button] = Button_JustUp;
				break;

			case SDL_MOUSEWHEEL:
				state->input->mouseWheel = event.wheel.y;
				break;

			case SDL_KEYDOWN:
				if(event.key.keysym.sym < 256) {
					state->input->keys[event.key.keysym.sym] = Button_JustDown;
				}
				break;

			case SDL_KEYUP:
				if(event.key.keysym.sym < 256) {
					state->input->keys[event.key.keysym.sym] = Button_JustUp;
				}
				break;

			case SDL_WINDOWEVENT:
				switch(event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
						{
							int width, height;
							SDL_GetWindowSize(internal->windowHandle, &width, &height);
							state->width = width;
							state->height = height;
						}
						break;
					case SDL_WINDOWEVENT_FOCUS_GAINED:
						lstate.hasFocus = 1;
						break;
					case SDL_WINDOWEVENT_FOCUS_LOST:
						lstate.hasFocus = 0;
						break;
				}
				break;
		}
	}

	{
		int mx, my;
		SDL_GetMouseState(&mx, &my);
		lstate.mouseX = mx;
		lstate.mouseY = my;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	*state = lstate;
	return 1;
}

i64 wRender(wWindow* window)
{
	wInternalSDL* internal = wGetInternal(window);
	SDL_GL_SwapWindow(internal->windowHandle);
	window->elapsedTicks = SDL_GetTicks() - window->lastTicks;
	if(window->elapsedTicks < 16) {
		SDL_Delay(16 - window->elapsedTicks);
	}
	return 0;
}

u8* wLoadFile(string filename, isize* sizeOut, wMemoryArena* alloc)
{
	u8* buffer = NULL;
	FILE* fp = fopen(filename, "rb");
	if(fp) {
		fseek(fp, 0L, SEEK_END);
		isize size = ftell(fp);
		rewind(fp);
		buffer = wArenaPush(alloc, size + 1);
		fread(buffer, sizeof(char), size, fp);
		buffer[size] = '\0';

		if(sizeOut) {
			*sizeOut = size;
		}
		fclose(fp);
	} else {
		wLogError(0, "wLoadFile: could not open %s\n", filename);
	}

	return buffer;
}

//returns actual number of bytes loaded;
isize wLoadSizedFile(string filename, u8* buffer, isize bufferSize)
{
	isize size = 0;;
	FILE* fp = fopen(filename, "rb");
	if(fp) {
		fseek(fp, 0L, SEEK_END);
		size = ftell(fp);
		rewind(fp);
		if(size > bufferSize) size = bufferSize;
		fread(buffer, sizeof(char), size, fp);
		fclose(fp);
	} else {
		wLogError(0, "wLoadSizedFile: could not open %s\n", filename);
	}

	return size;
}

u8* wLoadLocalFile(wWindow* window, string filename, isize* sizeOut, wMemoryArena* arena)
{
	char buf[1024];
	snprintf(buf, 1024, "%s%s", window->basePath, filename);
	return wLoadFile(buf, sizeOut, arena);
}

isize wLoadLocalSizedFile(
		wWindow* window, string filename,
		u8* buffer, isize bufferSize)
{
	char buf[1024];
	snprintf(buf, 1024, "%s%s", window->basePath, filename);
	return wLoadSizedFile(buf, buffer, bufferSize);
}

// These require using stat
// and I'm not particularly able to test unix code right now
// internally, these are only used by dev features
// so, uh, yeah 
// TODO(will): fancy up the unix backend
#ifdef WPL_WINDOWS
wFileHandle wGetFileHandle(string filename)
{
	wLogError(0, "wGetFileHandle not implemented for this backend (SDL)");
	return NULL;
}

void wCloseFileHandle(wFileHandle handle)
{
	wLogError(0, "wCloseFileHandle not implemented for this backend (SDL)");
}

isize wGetFileSize(wFileHandle file)
{
	wLogError(0, "wGetFileSize not implemented for this backend (SDL)");
	return -1;
}

isize wGetFileModifiedTime(wFileHandle file)
{
	wLogError(0, "wGetFileModifiedTime not implemented for this backend (SDL)");
	return -1;
}
#elif defined(WPL_LINUX) || defined(WPL_MACOS)

wFileHandle wGetFileHandle(string filename)
{
	return filename;
}

void wCloseFileHandle(wFileHandle handle)
{
}

isize wGetFileSize(wFileHandle file)
{
	string filename = (void*)file;
	struct stat s;//= {0};
	int ret = stat(filename, &s);
	return (isize)s.st_size;
}

isize wGetFileModifiedTime(wFileHandle file)
{
	string filename = (void*)file;
	struct stat s;// = {0};
	int ret = stat(filename, &s);
#ifdef WPL_MACOS
	return (isize)s.st_mtimespec.tv_nsec;
#elif defined(WPL_LINUX)
	return (isize)s.st_mtim;
#endif
}
#endif
