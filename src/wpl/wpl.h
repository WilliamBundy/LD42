#pragma once

/* Initializing everything in wpl is easy, but not a single-line thing
 *
 * 
 * wMemoryArena* arena = wArenaBootstrap(wGetMemoryInfo(), wArena_Normal);
 *
 * // these can be allocated statically if you prefer
 * wWindow* window = wArenaPush(arena, sizeof(wWindow));
 * wState* state = wArenaPush(arena, sizeof(wState));
 * wInputState* state = wArenaPush(arena, sizeof(wInputState));
 * wMixer* mixer = wArenaPush(arena, sizeof(wMixer));
 *
 * wWindowDef def = wDefineWindow("My App");
 * // change various properties here
 * wCreateWindow(&def, window);
 * wInitState(window, state, input);
 * wInitAudio(window, mixer, 256, arena);
 * 
 * // do your own loading code here
 * // you'll also need to setup a graphics environment too
 *
 * while(!state->exitEvent) {
 *     wUpdate(window, state);
 *	   // do your own update code here
 *     wRender(window);
 * }
 *
 * wQuit();
 */


// Basic platform detection
//
// If you want to do this yourself, or specify something special, your choices are:
// WPL_WINDOWS or WPL_LINUX
// On WPL_LINUX, you have to use WPL_SDL_BACKEND
// On WPL_WINDOWS, you can use either WPL_WIN32_BACKEND or WPL_SDL_BACKEND

#if !defined(WPL_WINDOWS) && !defined(WPL_LINUX) && !defined(WPL_MACOS)
#if _MSC_VER
#define WPL_WINDOWS
#else
#ifdef __APPLE__
#define WPL_MACOS
#else
#define WPL_LINUX
#endif
#endif 
#endif

#if !defined(WPL_WIN32_BACKEND) && !defined(WPL_SDL_BACKEND)
#ifdef WPL_WINDOWS
#define WPL_WIN32_BACKEND
#else
#define WPL_SDL_BACKEND
#endif
#endif

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
//#ifndef WPL_WINDOWS
#include <emmintrin.h>
#include <immintrin.h>
//#else
//#include <intrin.h>
//#endif
#define VariadicArgs ...

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef size_t usize;
typedef ptrdiff_t isize;

typedef __m128 vf128;
typedef __m128i vi128;

union vf32x4
{
	vf128 v;
	f32 f[4];
};
typedef union vf32x4 vf32x4;

#define ShuffleToByte(x, y, z, w) (((x)<<6) | ((y)<<4) | ((z)<<2) | w)
#define vfShuffle(a, x, y, z, w) _mm_shuffle_ps(a, a,\
		ShuffleToByte(x, y, z, w))
#define vfShuffle2(a, b, ax, ay, bz, bw) _mm_shuffle_ps(a, b, \
		ShuffleToByte(x, y, z, w))

#define Math_Tau 6.283185307179586f
#define Math_DegToRad (Math_Tau / 360.0f)
#define Math_RadToDeg (360.0f / Math_Tau)

#define CalcKilobytes(x) (((usize)x) * 1024)
#define CalcMegabytes(x) (CalcKilobytes((usize)x) * 1024)
#define CalcGigabytes(x) (CalcMegabytes((usize)x) * 1024)

typedef struct wWindowDef wWindowDef;
typedef struct wWindow wWindow;
typedef struct wInputState wInputState;
typedef struct wState wState;

typedef struct wSarId wSarId;
typedef struct wSarHeader wSarHeader;
typedef struct wSarFile wSarFile;
typedef struct wSarArchive wSarArchive;
typedef struct wSarEditingArchive wSarEditingArchive;

typedef struct wMemoryInfo wMemoryInfo;
typedef struct wMemoryArena wMemoryArena;
typedef struct wMemoryPool wMemoryPool;
typedef struct wTaggedHeapArena wTaggedHeapArena;
typedef struct wTaggedHeap wTaggedHeap;

typedef struct wRenderBatch wRenderBatch;
typedef struct wShaderComponent wShaderComponent;
typedef struct wShader wShader;
typedef struct wTexture wTexture;

typedef struct wMixer wMixer;
typedef struct wMixerVoice wMixerVoice;
typedef struct wMixerStream wMixerStream;
typedef struct wMixerSample wMixerSample;
typedef void (*wMixerStreamProc)(wMixerSample* sample, void* userdata);

typedef struct wGlyph wGlyph;
typedef struct wGlyphImage wGlyphImage;
typedef struct wFontInfo wFontInfo;

typedef struct wTextureSegmentGrid wTextureSegmentGrid;
typedef struct wTextureSegment wTextureSegment;
typedef struct wTextureAtlas wTextureAtlas;

typedef const char* string;

/* optional CRT replacement header */
#ifdef WPL_REPLACE_CRT
void* memset(void* s, i32 ivalue, usize size);
i32 memcmp(const void* s1, const void* s2, usize size);
void* memcpy(void *dest, const void *source, usize size);
usize strlen(const char* c);
void* malloc(usize size);
void free(void* mem);
void* realloc(void* mem, usize size);
i32 abs(i32 x);
i32 printf(string fmt, VariadicArgs);
i32 fprintf(void* file, string fmt, VariadicArgs);
i32 vfprintf(void* file, string fmt, va_list args);
i32 stbsp_snprintf(char* buf, int count, string fmt, VariadicArgs);
extern void* stderr;
extern void* stdout;
#else
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#endif


/* Window/State interface */

struct wWindowDef
{
	string title;
	i64 width, height;
	
	// Position information
	// If centered == 1, 
	// center the window on monitorIndex
	// else, use x, y
	i64 posCentered;
	i64 posUndefined;
	i64 x, y;

	i64 resizeable;
	i64 borderless;
	i64 hidden;

	i64 glVersion;
};

struct wWindow
{
	wState* state;

	i64 refreshRate;
	i64 glVersion;
	i64 lastTicks;
	i64 elapsedTicks;

	string basePath;

	wSarArchive* baseArchive;
	wMixer* mixer;

	void* internal;
};


/* Win32 Only -- resource type is wplResource */
void* wLoadResource(string name, isize* size);

wWindowDef wDefineWindow(string title);

/* wCreateWindow only creates the OS window
 * It doesn't initialize everything!
 * You need to call wInitState and wInitAudio too!
 */

i64 wCreateWindow(wWindowDef* def, wWindow* window);
void wShowWindow();

struct wState
{
	wInputState* input;
	i64 width, height;
	i64 hasFocus;
	i64 mouseX, mouseY;
	i64 exitEvent;
};

i64 wUpdate(wWindow* window, wState* state);
i64 wRender(wWindow* window);
void wQuit();

/* Input */

enum ButtonState
{
	Button_JustUp = -1,
	Button_Up = 0,
	Button_Down = 1,
	Button_JustDown = 2
};

#define wMouseLeft 1
#define wMouseRight 3
#define wMouseMiddle 2
#define wMouseX1 3
#define wMouseX2 4

struct wInputState
{
	i8 keys[256];
	i8 mouse[16];
	f32 mouseWheel;
};

void wInitState(wWindow* window, wState* state, wInputState* input);
void wInitAudio(wWindow* window, wMixer* mixer, i32 voiceCount, wMemoryArena* arena);

i64 wKeyIsDown(wInputState* wInput, i64 keycode);
i64 wKeyIsUp(wInputState* wInput, i64 keycode);
i64 wKeyIsJustDown(wInputState* wInput, i64 keycode);
i64 wKeyIsJustUp(wInputState* wInput, i64 keycode);
i64 wMouseIsDown(wInputState* wInput, i64 btn);
i64 wMouseIsUp(wInputState* wInput, i64 btn);
i64 wMouseIsJustDown(wInputState* wInput, i64 btn);
i64 wMouseIsJustUp(wInputState* wInput, i64 btn);
f32 wGetMouseWheel(wInputState* wInput);

/* utility */

void wLogError(i32 errorClass, string fmt, VariadicArgs);

// use memcpy instead
//void* wCopyMemory(void *dest, const void *source, i64 size);
void wCopyMemoryBlock(void* dest, const void* source, 
		i32 sx, i32 sy, i32 sw, i32 sh,
		i32 dx, i32 dy, i32 dw, i32 sourceWidth,
		i32 size, i32 border);

usize wDecompressMemToMem(
		void *output,
		usize outSize,
		void *input,
		usize inSize,
		i32 flags);


/* File Handling */

u8* wLoadFile(string filename, isize* sizeOut, wMemoryArena* alloc);
isize wLoadSizedFile(string filename, u8* buffer, isize bufferSize);
u8* wLoadLocalFile(
		wWindow* window,
		string filename, 
		isize* sizeOut, 
		wMemoryArena* arena);
isize wLoadLocalSizedFile(
		wWindow* window,
		string filename,
		u8* buffer,
		isize bufferSize);

typedef struct wHotFile wHotFile;
typedef void* wFileHandle;
struct wHotFile
{
	void* handle;
	u64 lastTime;

	void* data;
	isize size;

	isize filenameLength;
	char filename[511], zero;

	i32 replaceBadSpaces;
};

wHotFile* wCreateHotFile(wWindow* window, string filename);
void wDestroyHotFile(wHotFile* file);
i32 wUpdateHotFile(wHotFile* file);
i32 wCheckHotFile(wHotFile* file);

// These currently only work on Win32
wFileHandle wGetFileHandle(string filename);
void wCloseFileHandle(wFileHandle file);
wFileHandle wGetLocalFileHandle(wWindow* window, string filename);;
isize wGetFileSize(wFileHandle file);
isize wGetFileModifiedTime(wFileHandle file);

/* s-archive interface */

#define wSar_Magic (0x77536172)
#define wSar_Version (101)
#define wSar_NameLen (55)
#pragma pack(push, 4)
struct wSarId
{
	u64 hash;
	char name[wSar_NameLen], zero;
};

struct wSarFile
{
	wSarId id;
	u32 kind;
	u32 version;
	u64 compressedSize;
	u64 fullSize;
	u64 location;
};

struct wSarHeader
{
	u32 magic;
	u32 version;
	u64 unused[3];

	wSarId id;
	u64 archiveSize;
	u64 fileCount;
	u64 fileTableLocation;
	u64 descriptionLength;
};

struct wSarArchive
{
	char* base;
	wSarHeader* header;
	char* description;
	wSarFile* files;
};

#pragma pack(pop)

u64 wHashBuffer(const char* buf, isize length);
u64 wHashString(string s);
wSarArchive* wSarLoad(void* file, wMemoryArena* alloc);
isize wSarGetFileIndexByHash(wSarArchive* archive, u64 key);
wSarFile* wSarGetFile(wSarArchive* archive, string name);
void* wSarGetFileData(wSarArchive* archive, string name, 
		isize* sizeOut, wMemoryArena* arena);


/* wb_alloc types */

#define wArena_Normal 0
#define wArena_FixedSIze 1
#define wArena_Stack 2
#define wArena_Extended 4
#define wArena_NoZeroMemory 8
#define wArena_NoRecommit 16 

#define wPool_Normal 0
#define wPool_FixedSize 1
#define wPool_Compacting 2
#define wPool_NoZeroMemory 4
#define wPool_NoDoubleFreeCheck 8

#define wTagged_Normal 0
#define wTagged_FixedSize 1
#define wTagged_NoZeroMemory 2
#define wTagged_NoSetCommitSize 4
#define wTagged_SearchForBestFit 8

struct wMemoryInfo
{
	usize totalMemory, commitSize, pageSize;
	isize commitFlags;
};

struct wMemoryArena
{
	const char* name;
	void *start, *head, *end;
	void *tempStart, *tempHead;
	wMemoryInfo info;
	isize align;
	isize flags;
};

struct wMemoryPool
{
	usize elementSize; 
	isize count, capacity;
	void* slots;
	const char* name;
	void** freeList;
	wMemoryArena* alloc;
	isize lastFilled;
	isize flags;
};

struct wTaggedHeapArena
{
	isize tag;
	wTaggedHeapArena *next;
	void *head, *end;
	char buffer;
};

struct wTaggedHeap
{
	const char* name;
	wMemoryPool pool;
	wTaggedHeapArena* arenas[64];
	wMemoryInfo info;
	usize arenaSize, align;
	isize flags;
};

wMemoryInfo wGetMemoryInfo();

void wArenaInit(wMemoryArena* arena, wMemoryInfo info, isize flags);
wMemoryArena* wArenaBootstrap(wMemoryInfo info, isize flags);
void wArenaFixedSizeInit(wMemoryArena* arena, void* buffer, isize size, isize flags);
wMemoryArena* wArenaFixedSizeBootstrap(void* buffer, usize size, isize flags);
void* wArenaPushEx(wMemoryArena* arena, isize size, isize extended);
void* wArenaPush(wMemoryArena* arena, isize size);
void wArenaPop(wMemoryArena* arena);
void wArenaStartTemp(wMemoryArena* arena);
void wArenaEndTemp(wMemoryArena* arena);
void wArenaClear(wMemoryArena* arena);
void wArenaDestroy(wMemoryArena* arena);
 
void* wPoolRetrieve(wMemoryPool* pool);
void wPoolRelease(wMemoryPool* pool, void* ptr);
void wPoolInit(wMemoryPool* pool,wMemoryArena* alloc, usize elementSize, isize flags);
wMemoryPool* wPoolBootstrap(wMemoryInfo info,isize elementSize, isize flags);
wMemoryPool* wPoolFixedSizeBootstrap(
		isize elementSize, 
		void* buffer, usize size,
		isize flags);
void* wPoolFromIndex(wMemoryPool* pool, isize index);
isize wPoolIndex(wMemoryPool* pool, void* ptr);
 
void wTaggedInit(wTaggedHeap* heap, wMemoryArena* arena, isize lsize, isize flags);
wTaggedHeap* wTaggedBootstrap(wMemoryInfo info, isize arenaSize, isize flags);
wTaggedHeap* wTaggedFixedSizeBootstrap(isize arenaSize, 
		void* buffer, isize bufferSize, 
		isize flags);
void* wTaggedAlloc(wTaggedHeap* heap, isize tag, usize size);
void wTaggedFree(wTaggedHeap* heap, isize tag);

/* Mixer Interface */
struct wMixerSample
{
	//in samples (4 bytes)
	u32 length;
	u32 frequency;        
	void* data;
};

struct wMixerStream 
{
	void* userdata;         
	wMixerStreamProc callback;         
	wMixerSample sample;           
};

struct wMixerVoice 
{
	wMixerSample* sample;
	wMixerStream* stream;
	f32 position;
	f32 gain;
	f32 pitch;
	f32 pan;
	i32 state;
};

struct wMixer
{
	f32 gain; 
	u32 frequency;
	
	isize voiceCount;
	wMixerVoice* voices;
};

// These mixer functions are provided for extra control, 
// however, they shouldn't be used without locking/unlocking the audio
// device first, generally.

// Use wInitAudio to initialize your mixer
void wMixerInit(wMixer* mixer, isize voiceCount, wMixerVoice* voices);
i32 wMixerGetActiveVoices(wMixer* mixer);
i32 wMixerInternalPlaySample(
		wMixer* mixer, 
		wMixerSample* sample,
		f32 gain,
		f32 pitch, 
		f32 pan);
i32 wMixerInternalPlayStream(wMixer* mixer, wMixerStream* stream, f32 gain);
void wMixerStopVoice(wMixer* mixer, i32 voice);
void wMixerStopSample(wMixer* mixer, wMixerSample* sample);
void wMixerStopStream(wMixer* mixer, wMixerStream* stream);
void wMixerMixAudio(wMixer* mixer, f32* output, u32 samples);
void wLockAudioDevice(wWindow* window);
void wUnlockAudioDevice(wWindow* window);


void wPlaySample(wWindow* window, 
		wMixerSample* sample,
		f32 gain,
		f32 pitch,
		f32 pan);
void wPlayStream(wWindow* window, wMixerStream* stream, f32 gain);

wMixerSample* wWavToSample(u8* data, isize size, wMemoryArena* arena);
wMixerSample* wOggToSample(u8* data, isize size, wMemoryArena* arena);
wMixerStream* wOggToStream(u8* data, isize size, wMemoryArena* arena);

/* Graphics */

#define Shader_MaxAttribs 16
#define Shader_MaxUniforms 16

struct wShaderComponent
{
	string name;
	i32 loc, divisor;

	i32 type, count;
	usize ptr;
};

enum {
	//Shader kinds
	wShader_Vertex,
	wShader_Frag,
	//Component kinds
	wShader_Attrib,
	wShader_Uniform,
	//Component types
	wShader_Float,
	wShader_Double,
	//Passed with glVertexAttributeIPointer
	wShader_Int,
	wShader_Short,
	wShader_Byte,
	//Passed with normalize set to true
	wShader_NormalizedInt,
	wShader_NormalizedShort,
	wShader_NormalizedByte,
	//Converted to floats on the GPU
	wShader_FloatInt,
	wShader_FloatShort,
	wShader_FloatByte,
	wShader_Mat22,	
	wShader_Mat33,
	wShader_Mat44
};

enum {
	wRenderBatch_Arrays,
	wRenderBatch_Elements,
	wRenderBatch_ArraysInstanced,
	wRenderBatch_ElementsInstanced,
};

enum {
	wRenderBatch_BlendNormal,
	wRenderBatch_BlendPremultiplied,
	wRenderBatch_BlendNone,
};

enum {
	wRenderBatch_Triangles,
	wRenderBatch_TriangleStrip,
	wRenderBatch_TriangleFan,
	wRenderBatch_Lines,
	wRenderBatch_LineStrip,
	wRenderBatch_LineLoop
};

struct wShader
{
	u32 vert, frag, program;
	i32 targetVersion;

	i32 defaultDivisor;
	i32 stride;
	
	i32 attribCount, uniformCount;
	wShaderComponent attribs[Shader_MaxAttribs];
	wShaderComponent uniforms[Shader_MaxUniforms];
};

struct wTexture
{
	i64 w, h;
	u8* pixels;
	u32 glIndex;
};

struct wRenderBatch
{
	wTexture* texture;
	wShader* shader;

	u32 vao, vbo;

	isize elementSize, elementCount, instanceSize;
	isize indicesCount;
	isize startOffset;
	void* data;
	u32* indices;

	i32 clearOnDraw;
	i32 renderCall;
	i32 blend;

	i32 scissor;
	f32 scissorRect[4];

	i32 primitiveMode;
};

void wInitShader(wShader* shader, i32 stride);
i32 wAddAttribToShader(wShader* shader, wShaderComponent* attrib);
wShaderComponent* wCreateAttrib(wShader* shader, 
		string name, i32 type, i32 count, usize ptr);
i32 wAddUniformToShader(wShader* shader, wShaderComponent* uniform);
wShaderComponent* wCreateUniform(wShader* shader,
		string name, i32 type, i32 count, usize ptr);
i32 wFinalizeShader(wShader* shader);
i32 wAddSourceToShader(wShader* shader, string src, i32 kind);
void wRemoveSourceFromShader(wShader* shader, i32 kind);
void wDeleteShaderProgram(wShader* shader);

void wInitBatch(wRenderBatch* batch,
		wTexture* texture, wShader* shader,
		i32 renderCall, i32 primitiveMode, 
		isize elementSize, isize instanceSize,
		void* data, u32* indices);
void wConstructBatchGraphicsState(wRenderBatch* batch);
void wDrawBatch(wState* state, wRenderBatch* batch, void* uniformData);
void wSetBatchScissorRect(wRenderBatch* batch, i32 enabled, f32 x, f32 y, f32 w, f32 h);


//wTexture* wLoadTexture(wWindow* window, string filename, wMemoryArena* arena);
i32 wInitTexture(wTexture* texture, void* data, isize size);
void wUploadTexture(wTexture* texture);

#pragma pack(push, 4)
struct wGlyph
{
	int character;
	f32 width, height;
	f32 x, y;
	f32 advance;
	f32 l, b, r, t;
};

struct wGlyphImage
{
	i32 x, y, w, h;
	f32 bbx, bby;
};

struct wFontInfo
{
	i32 sizeX, sizeY;
	i32 scale;
	i32 offsetX, offsetY;
	i32 pxRange;
	i32 lineSpacing;
	i32 atlasX, atlasY;

	wGlyph glyphs[96];
	wGlyphImage images[96];
	f32 kerning[96][96];
};
#pragma pack(pop)

/* Texture Packer */

struct wTextureSegmentGrid
{
	i32 x, y;
	i32 w, h;
	i32 count;
	string* names;
	i32 size; 
	i32 bordered;
};

struct wTextureSegment
{
	string name;
	u64 hash;
	i32 index;
	i32 bordered;
	i32 trimAlpha;
	u8* pixels;
	i16 x, y, w, h;
	wTexture* texture;
};

struct wTextureAtlas
{
	wTextureSegment* segments;
	isize count, capacity;
	wTexture texture;
};


void wInitAtlas(wTextureAtlas* atlas, isize capacity, wMemoryArena* arena);
void wInitSegment(wTextureSegment* segment);
i32 wAddSegment(wTextureAtlas* atlas,
		wTexture* texture,
		string name,
		i16 x, i16 y, i16 w, i16 h, 
		i32 bordered);
void wAddSegmentGrid(wTextureAtlas* atlas, wTexture* texture, wTextureSegmentGrid* grid);
void wSortSegments(wTextureAtlas* atlas);
i32 wFindSegmentByHash(wTextureAtlas* atlas, u64 key);
wTextureSegment* wFindSegment(wTextureAtlas* atlas, string name);
void wAtlasCreateTexture(wTextureAtlas* atlas, i32 size, wMemoryArena* arena);
i32 wFinalizeAtlas(wTextureAtlas* atlas, wMemoryArena* arena);

//wFontInfo* wLoadFontInfo(wWindow* window, char* filename, wMemoryArena* arena);
//this is basically exposed by 
// TODO(will) convenience functions that make it easy to implement nice font
// 				layout/rendering on the client side.
// Stuff like:
//  - wFontGetKerningPair
//  - wFontLayoutGlyph
//  - wFontGetGlyphRect
// Essentially things that encapsulate the weirdness that goes on in the 
// render text function of the old batch (though we'll provide that too)

#define  wTabKey 0x9
#define  wEnterKey 0xd
#define  wShiftKey 0x10
#define  wCtrlKey 0x11
#define  wAltKey 0x12
#define  wEscKey 0x1b
#define  wSpaceKey 0x20

#define  wEndKey 0x23
#define  wHomeKey 0x24
#define  wDeleteKey 0x2e
#define  wInsertKey 0x2d

#define  wLeftKey 0x25
#define  wUpKey 0x26
#define  wRightKey 0x27
#define  wDownKey 0x28

#define  wF1Key 0x70
#define  wF2Key 0x71
#define  wF3Key 0x72
#define  wF4Key 0x73
#define  wF5Key 0x74
#define  wF6Key 0x75
#define  wF7Key 0x76
#define  wF8Key 0x77
#define  wF9Key 0x78
#define  wF10Key 0x79
#define  wF11Key 0x7a
#define  wF12Key 0x7b

#define  wSemicolonKey 0xba
#define  wPlusKey 0xbb
#define  wCommaKey 0xbc
#define  wMinusKey 0xbd
#define  wPeriodKey 0xbe
#define  wSlashKey 0xbf
#define  wTildeKey 0xc0
#define  wOpenBracketKey 0xdb
#define  wBackslashKey 0xdc
#define  wCloseBracketKey 0xdd
#define  wQuoteKey 0xde
