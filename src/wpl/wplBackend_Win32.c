#ifndef WPL_WINDOWS
#error "Win32 Backend only functions under WPL_WINDOWS"
#endif

#include <Windows.h>
#include <Wingdi.h>
#include <Shlwapi.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <initguid.h>


#define WB_ALLOC_WINDOWS
#define WB_ALLOC_IMPLEMENTATION
#define WB_ALLOC_CUSTOM_INTEGER_TYPES
#include "thirdparty/wb_alloc.h"
 

#define STB_SPRINTF_IMPLEMENTATION
#include "thirdparty/stb_sprintf.h"

#define WB_GL_IMPLEMENTATION
#define WB_GL_NO_INCLUDES
#define WB_GL_USE_ALL_VERSIONS
#define WB_GL_WIN32
#include "thirdparty/wb_gl_loader.h"

#ifdef WPL_REPLACE_CRT
#define snprintf stbsp_snprintf
#define vsnprintf stbsp_vsnprintf
#include "wplCRT.c"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif


typedef struct
{
	IMMDevice* device;
	IAudioClient* audioClient;
	IAudioRenderClient* renderClient;
	WAVEFORMATEX format;
	CRITICAL_SECTION lock;
	HANDLE thread;
	wMixer* mixer;
} wWasapiState;


typedef struct
{
	HWND window;
	HDC windowDC;
	HGLRC glContext;
	wWasapiState audio;
	HCURSOR defaultCursor;
} wInternalWin32;

static inline
wInternalWin32* wGetInternal(wWindow* window)
{
	return (wInternalWin32*)window->internal;
}

static 
void handleKey(wInputState* input, i32 state, i32 code)
{
	input->keys[code] = state ? Button_JustDown : Button_JustUp;
}

static 
void handleMouse(wInputState* input, i32 state, i32 code)
{
	input->mouse[code] = state ? Button_JustDown : Button_JustUp;
}

static 
void handleMouseWheel(wInputState* input, i32 wheel)
{
	input->mouseWheel = (f32)wheel / (f32)WHEEL_DELTA;
}

static
LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	void* wlp = (void*)GetWindowLongPtr(window, GWLP_USERDATA);
	if(!wlp) {
		return DefWindowProcA(window, message, wParam, lParam);
	}
	wState* state = ((wWindow*)wlp)->state;
	HWND wnd = wGetInternal((wWindow*)wlp)->window;
	wInputState* input = state->input;
	switch(message) {
		case WM_QUIT:
		case WM_CLOSE:
			state->exitEvent = 1;
			break;

		case WM_KEYDOWN:
			if(!(lParam & (1 << 30)))  {
				handleKey(input, 1, (int)wParam);
			}
			break;

		case WM_KEYUP:
				handleKey(input, 0, (int)wParam);
			break;

		case WM_MOUSEWHEEL:
			handleMouseWheel(input, GET_WHEEL_DELTA_WPARAM(wParam));
			break;

		case WM_LBUTTONDOWN:
			SetCapture(wnd);
			handleMouse(input, 1, wMouseLeft); 
			break;
		case WM_MBUTTONDOWN:
			SetCapture(wnd);
			handleMouse(input, 1, wMouseMiddle); 
			break;
		case WM_RBUTTONDOWN:
			SetCapture(wnd);
			handleMouse(input, 1, wMouseRight); 
			break;
		case WM_XBUTTONDOWN:
			SetCapture(wnd);
			if(HIWORD(wParam) == XBUTTON1) {
				handleMouse(input, 1, wMouseX1); 
			} else if(HIWORD(wParam) == XBUTTON2) {
				handleMouse(input, 1, wMouseX2); 
			}
			break;

		case WM_LBUTTONUP:
			handleMouse(input, 0, wMouseLeft); 
			ReleaseCapture();
			break;
		case WM_MBUTTONUP:
			handleMouse(input, 0, wMouseMiddle); 
			ReleaseCapture();
			break;
		case WM_RBUTTONUP:
			handleMouse(input, 0, wMouseRight); 
			ReleaseCapture();
			break;
		case WM_XBUTTONUP:
			if(HIWORD(wParam) == XBUTTON1) {
				handleMouse(input, 0, wMouseX1); 
			} else if(HIWORD(wParam) == XBUTTON2) {
				handleMouse(input, 0, wMouseX2); 
			}
			break;


		default:
			return DefWindowProcA(window, message, wParam, lParam);
	}

	return 0;
}

void wQuit()
{
	ExitProcess(0);
}


i64 wCreateWindow(wWindowDef* def, wWindow* window)
{
	if(!stdout) {
		stdout = GetStdHandle(STD_OUTPUT_HANDLE);
		stderr = GetStdHandle(STD_ERROR_HANDLE);
	}
	// We have to use malloc here, as we don't create
	// our own allocator internally.
	wInternalWin32* internal = malloc(sizeof(wInternalWin32));
	window->internal = internal;

	HANDLE module = GetModuleHandle(NULL);
	WNDCLASSA windowClass = {0};
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.hInstance = module;
	windowClass.lpfnWndProc = windowCallback;
	windowClass.lpszClassName = "wplWindowClass";

	if(!RegisterClassA(&windowClass)) {
		wLogError(0, "Failed to register window class");
		return 1;
	}

	if(def->width == 0) {
		def->width = 1280;
	} 

	if(def->height == 0) {
		def->height = 720;
	}
	i64 wposx, wposy;
	if(def->posCentered) {
		wposx = (GetSystemMetrics(SM_CXSCREEN) - def->width) / 2;
		wposy = (GetSystemMetrics(SM_CYSCREEN) - def->height) / 2;
	} else if(def->posUndefined) {
		wposx = CW_USEDEFAULT;
		wposy = CW_USEDEFAULT;
	} else {
		wposx = def->x;
		wposy = def->y;
	}

	HWND wnd = CreateWindowExA(0, 
			windowClass.lpszClassName,
			def->title,
			WS_OVERLAPPEDWINDOW,
			wposx, wposy,
			def->width, def->height,
			0, 0, 
			windowClass.hInstance,
			0);
	HDC windowDC = GetDC(wnd);
	internal->window = wnd;
	internal->windowDC = windowDC;
	internal->glContext = wbgl_win32_create_context(windowDC, 3, 3, 1);
	i32 ret = wbgl_load_all(NULL);

	if(!def->hidden) {
		wShowWindow(window);
	}

	{
		char filename[1024] = {0};
		u32 size = GetModuleFileNameA(module, filename, 1024);
		for(u32 find = size-1; find > 0; find--) {
			if(filename[find] == '\\') {
				size = find + 1;
				break;
			}
		}
		filename[size] = '\0';
		char* path = malloc(size+1);
		memcpy(path, filename, size+1);
		window->basePath = path;
	}

	SetWindowLongPtr(internal->window, GWLP_USERDATA, (isize)window);
	// TODO(will): Add capability to enable/disable cursors
	// at runtime. Right now we lock to default arrow cursor, which
	// we probably don't want for gameplay
	internal->defaultCursor = LoadCursor(NULL, IDC_ARROW);

	return 0;
}


void wShowWindow(wWindow* window)
{
	HWND wnd = wGetInternal(window)->window;
	if(ShowWindow(wnd, SW_SHOWNORMAL) != 0) {
		//return 1;
	}
}

i64 wUpdate(wWindow* window, wState* state)
{
	wInputUpdate(state->input);
	
	wInternalWin32* internal = wGetInternal(window);
	HWND wnd = internal->window;
	SetCursor(internal->defaultCursor);
	MSG message;
	while(PeekMessage(&message, wnd, 0, 0, PM_REMOVE) != 0) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	RECT r;
	GetClientRect(wnd, &r);
	state->width = r.right - r.left;
	state->height = r.bottom - r.top;

	POINT mp;
	GetCursorPos(&mp);
	ScreenToClient(wnd, &mp);

	i32 lastMouseOut = state->mouseX < 0 || 
		state->mouseY < 0 || 
		state->mouseX >= state->width || 
		state->mouseY >= state->height;
	i32 currentMouseOut = mp.x < 0 || 
		mp.y < 0 || 
		mp.x >= state->width || 
		mp.y >= state->height;

	if((!lastMouseOut && currentMouseOut) || (!currentMouseOut && lastMouseOut)) {
		for(isize i = 0; i < 256; ++i) {
			state->input->keys[i] = Button_JustUp;
		}
	}
	state->mouseX = mp.x;
	state->mouseY = mp.y;

	glViewport(0, 0, state->width, state->height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	return 1;
}

i64 wRender(wWindow* window)
{
	HDC windowDC = wGetInternal(window)->windowDC;
	SwapBuffers(windowDC);
	//TODO(will): perform manual timing here
	return 0;
}


/* WASAPI audio layer
 *
 * Thanks to StrangeZak/LudusEngine for the base for this code 
 * It's heavily modified, but I wouldn't have gotten it done without it.
 * */

void wLockAudioDevice(wWindow* window)
{
	wInternalWin32* internal = wGetInternal(window);
	EnterCriticalSection(&internal->audio.lock);
}

void wUnlockAudioDevice(wWindow* window)
{
	wInternalWin32* internal = wGetInternal(window);
	LeaveCriticalSection(&internal->audio.lock);
}

static
void wInternalAudioCallback(void* userdata, u8* byteStream, i32 len)
{
	//mix audio and write to stream here
	//this is called from another thread
	//so we need to make sure our mixer is locked
	
	wWasapiState* state = userdata;
	if(state->mixer)
	wMixerMixAudio(state->mixer, byteStream, len);
}

static 
u32 wWasapiAudioThreadProc(void *parameter)
{
	wWasapiState* state = (wWasapiState*)parameter;
	IAudioClient* client = state->audioClient;
	IAudioRenderClient* render = state->renderClient;

	SetThreadPriority(state->thread, THREAD_PRIORITY_HIGHEST); 

	HANDLE bufferReadyEvent = CreateEvent(0, 0, 0, 0);
	HRESULT ret = client->lpVtbl->SetEventHandle(client, bufferReadyEvent);
	if(ret != S_OK) {
		wLogError(0, "Error: SetEventHandle failed for audio client! %x\n", ret);
		return 0;
	}

	u32 bufferSize = 0;
	ret = client->lpVtbl->GetBufferSize(client, &bufferSize);
	if(ret != S_OK) {
		wLogError(0, "Error: Failed to get buffer size for audio! %x\n", ret);
		return 0;

	}

	ret = client->lpVtbl->Start(client);
	if(ret != S_OK) {
		wLogError(0, "Error: Failed to start audio client! %x\n", ret);
		return 0;
	}

	while(1) {
		if(WaitForSingleObject(bufferReadyEvent, INFINITE) == WAIT_OBJECT_0) {
			u32 paddingFrameCount = 0;
			ret = client->lpVtbl->GetCurrentPadding(client, &paddingFrameCount); 
			if(!SUCCEEDED(ret)) continue;
			i32 writeAmount = bufferSize - paddingFrameCount;

			if(writeAmount <= 0) {
				continue;
			}

			u8 *buf = 0;
			ret = render->lpVtbl->GetBuffer(render, writeAmount, &buf); 
			if(!SUCCEEDED(ret)) continue;

			memset(buf, 0, writeAmount * sizeof(f32));

			EnterCriticalSection(&state->lock);
			wInternalAudioCallback(state, buf, writeAmount);
			LeaveCriticalSection(&state->lock);

			render->lpVtbl->ReleaseBuffer(render, writeAmount, 0);
		}
	}
	return 0;
}


DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395,
		0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2,
		0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(IID_IAudioClient, 0x1CB9AD4C,
		0xDBFA, 0x4C32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_GUID(IID_IAudioRenderClient, 0xF294ACFC, 
		0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);
DEFINE_GUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 0x3,
		0, 0x10, 0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71);

static 
void wInitWasapi(wWasapiState* state)
{
	InitializeCriticalSection(&state->lock);
	IMMDeviceEnumerator *devEnum = 0;

	HRESULT ret = CoCreateInstance(
			&CLSID_MMDeviceEnumerator, 
			0, 
			CLSCTX_ALL, 
			&IID_IMMDeviceEnumerator, 
			(void **)&devEnum);
	if(ret != S_OK) {
		wLogError(0, "Error: MMDeviceEnumerator creation failed! %x\n", ret);
		return;
	}

	ret = devEnum->lpVtbl->GetDefaultAudioEndpoint(
			devEnum,
			eRender,
			eConsole, 
			&state->device);
	IMMDevice *device = state->device;
	if(ret != S_OK || !device) {
		wLogError(0, "GetDefaultAudioEndpoint failed! %x\n", ret);
		return;
	}

	ret = device->lpVtbl->Activate(device,
			&IID_IAudioClient, 
			CLSCTX_ALL, 
			0, 
			(void **)&state->audioClient);
	IAudioClient* client = state->audioClient;

	if(ret != S_OK) {
		wLogError(0, "Audio device activation failed! %x\n", ret);
		return;
	}


	WAVEFORMATEX* internalFmt = NULL;
	ret = client->lpVtbl->GetMixFormat(client, &internalFmt);
	if(ret != S_OK) {
		wLogError(0, "Unable to retrieve default format %x\n", ret);
		//return;
	}
	
	//internalFmt->nSamplesPerSec = 44100;

	WAVEFORMATEX fmt = {0};
	fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	fmt.wBitsPerSample = 32;
	fmt.nChannels = 2;
	fmt.nSamplesPerSec = 44100;
	fmt.nBlockAlign = (fmt.nChannels * fmt.wBitsPerSample) / 8;
	fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
	fmt.cbSize = 0;
	//probably breaks everything
	//__debugbreak();
	//fmt = *internalFmt;
	state->format = fmt;

	//REFERENCE_TIME TimeRequested = 10000000;
	ret = client->lpVtbl->Initialize(client,
			AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
			AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM, 

			// samples per ns = samples per second * 10^9
			// therefore: (samples per ns)/100 * sample size in bytes
			// allocates us enough room for a second's worth of samples
			// we probably only want enough for a very small amount of time
			// though; this seems to work?
			fmt.nSamplesPerSec * fmt.nBlockAlign, 
			0,
			//internalFmt,
			&fmt,
			0);

	if(ret != S_OK) {
		wLogError(0, "Error: WriteAudioClient init failed! %x\n", ret);
		return;
	}

	ret = client->lpVtbl->GetService(client,
			&IID_IAudioRenderClient,
			(void **)&state->renderClient);
	if(ret != S_OK) {
		wLogError(0, "Error: GetService for Audio client failed! %x\n", ret);
		return;
	}

	devEnum->lpVtbl->Release(devEnum);
	state->thread = CreateThread(0, 0, wWasapiAudioThreadProc, state, 0, 0);
}

void wInitAudio(wWindow* window, wMixer* mixer, i32 voices, wMemoryArena* arena)
{
	wInternalWin32* internal = wGetInternal(window);
	wInitWasapi(&internal->audio);
	window->mixer = mixer;
	wMixerInit(window->mixer, voices,
			wArenaPush(arena, sizeof(wMixerVoice) * voices));
	internal->audio.mixer = mixer;
}

u8* wLoadFile(string filename, isize* sizeOut, wMemoryArena* alloc)
{
	HANDLE file = CreateFile(filename, 
			GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	LARGE_INTEGER largeSize;
	if(!GetFileSizeEx(file, &largeSize)) {
		CloseHandle(file);
		return NULL;
	}
	isize size = (isize)largeSize.QuadPart;  
	u8* buffer = wArenaPush(alloc, size+1);
	
	if(!ReadFile(file, buffer, (u32)size, NULL, NULL)) {
		CloseHandle(file);
		return NULL;
	}

	buffer[size] = '\0';

	if(sizeOut) {
		*sizeOut = size;	
	}
	CloseHandle(file);
	return buffer;
}

//returns actual number of bytes loaded;
isize wLoadSizedFile(string filename, u8* buffer, isize bufferSize)
{
	HANDLE file = CreateFileA(filename, 
			GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE) {
		wLogError(0, "Failed to open %s: %d\n", filename, GetLastError());
		return 0;
	}
	LARGE_INTEGER largeSize;
	if(!GetFileSizeEx(file, &largeSize)) {
		CloseHandle(file);
		return 0;
	}
	// this dance to avoid a signed/unsigned mismatch
	isize size = (isize)largeSize.QuadPart;  
	if(size > bufferSize) size = bufferSize;
	
	if(!ReadFile(file, buffer, size, NULL, NULL)) {
		CloseHandle(file);
		wLogError(0, "Failed to read %zd bytes from %s\n", size, filename);
		return 0;
	}
	CloseHandle(file);
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

wFileHandle wGetFileHandle(string filename)
{
	u32 access = GENERIC_READ;
	u32 share = FILE_SHARE_READ | FILE_SHARE_WRITE;
	u32 create = OPEN_EXISTING;
	u32 flags = FILE_ATTRIBUTE_NORMAL;
	wFileHandle file = CreateFileA(
			filename,
			access,
			share, 
			NULL,
			create,
			flags,
			NULL
			);
	if(file == INVALID_HANDLE_VALUE) {
		wLogError(0, "Loading %s failed: %d", filename, GetLastError());
		return NULL;
	}
	return file;
}

void wCloseFileHandle(wFileHandle file)
{
	CloseHandle(file);
}

isize wGetFileSize(wFileHandle file)
{
	LARGE_INTEGER i;
	GetFileSizeEx(file, &i);
	return (isize)i.QuadPart;
}

isize wGetFileModifiedTime(wFileHandle file)
{
	LARGE_INTEGER i;
	FILETIME t;
	GetFileTime(file, NULL, NULL, &t);
	i.u.LowPart = t.dwLowDateTime;
	i.u.HighPart = t.dwHighDateTime;
	return i.QuadPart;
}

void* wLoadResource(string name, isize* size)
{
	HRSRC hrsrc = FindResource(NULL, name, "wplData");
	HGLOBAL res = LoadResource(NULL, hrsrc);
	u32 rsize = SizeofResource(NULL, hrsrc);
	void* rdata = LockResource(res);
	if(size)
		*size = (isize)rsize;
	return rdata;
}
