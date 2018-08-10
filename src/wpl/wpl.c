/* wpl -- William's Platform Layer */

// Global header
#include "wpl.h"

#if 0
// Backend API used by other code
// If you want to add a new backend, 
// if it implements these correctly
// it should work with the rest of the library.

// As it is, these are currently derived from the union 
// of what's possible with SDL and win32, mostly

i64 wCreateWindow(wWindowDef* def, wWindow* window);
void wShowWindow(wWindow* window);
i64 wUpdate(wWindow* window, wState* state);
i64 wRender(wWindow* window);
void wQuit();

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

typedef void* wFileHandle;
wFileHandle wGetFileHandle(string filename);
wFileHandle wGetLocalFileHandle(wWindow* window, string filename);;
isize wGetFileSize(wFileHandle file);
isize wGetFileModifiedTime(wFileHandle file);

void wLockAudioDevice(wWindow* window);
void wUnlockAudioDevice(wWindow* window);
#endif

static void wInputUpdate(wInputState* wInput);
#ifdef WPL_WIN32_BACKEND

#include "wplBackend_Win32.c"

#else

#ifdef WPL_SDL_BACKEND
#include "wplBackend_SDL.c"
#else
#error "WPL: No backend defined"
#endif

#endif

// Backend independent setup functions
wWindowDef wDefineWindow(string title)
{
	wWindowDef def;
	memset(&def, 0, sizeof(wWindowDef));
	def.title = title;
	def.posCentered = 1;
	def.resizeable = 1;
	def.glVersion = 33;
	def.width = 1280;
	def.height = 720;
	return def;
}

void wInitState(wWindow* window, wState* state, wInputState* input)
{
	//TODO(will): null assert error checking
	memset(state, 0, sizeof(wState));
	memset(input, 0, sizeof(wInputState));

	state->input = input;
	window->state = state;
}

// Functional components

#include "wplInput.c"
#include "wplRender.c"
#include "wplAudio.c"

// Utility
#define wHash_FNV64_Basis 14695981039346656037UL
#define wHash_FNV64_Prime 1099511628211UL
u64 wHashBuffer(const char* buf, isize length)
{
	u64 hash = wHash_FNV64_Basis;
	for(isize i = 0; i < length; ++i) {
		hash *= wHash_FNV64_Prime;
		hash ^= buf[i];
	}
	return hash;
}

u64 wHashString(string s)
{
	u64 hash = wHash_FNV64_Basis;
	while(*s != '\0') {
		hash *= wHash_FNV64_Prime;
		hash ^= *s++;
	}
	return hash;
}

// scw is source clip width/height
// sourceWidth is the total width of the source buffer
void wCopyMemoryBlock(void* dest, const void* source, 
		i32 sx, i32 sy, i32 scw, i32 sch,
		i32 dx, i32 dy, i32 dw, i32 sourceWidth,
		i32 size, i32 border)
{
	u8* dst = dest;
	const u8* src = source;
	for(isize i = 0; i < sch; ++i) {
		memcpy(
			dst + ((i+dy) * dw + dx) * size, 
			src + ((i+sy) * sourceWidth + sx) * size,
			scw * size);
	}

	if(border) {
		for(isize i = 0; i < sch; ++i) {
			memcpy(
					dst + ((i+dy) * dw + (dx-1)) * size, 
					dst + ((i+dy) * dw + dx) * size,
					1 * size);

			memcpy(
					dst + ((i+dy) * dw + (dx+scw)) * size, 
					dst + ((i+dy) * dw + (dx+scw-1)) * size,
					1 * size);
		}

		memcpy(
			dst + ((dy-1) * (dw) + (dx-1)) * size, 
			dst + ((dy) * (dw) + (dx-1)) * size,
			(scw+2) * size);

		memcpy(
			dst + ((dy+sch) * (dw) + (dx-1)) * size, 
			dst + ((dy+sch-1) * (dw) + (dx-1)) * size,
			(scw+2) * size);
	}
}

void wLogError(i32 errorClass, string fmt, VariadicArgs)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
}

/* wHotFile.h
 *
 * This is a debug-only lib for hot-reloading files.
 * It uses malloc which makes it not-really-safe to 
 * use in a shipping version (on windows at least)
 *
 * Usage:
 * 		wHotFile* file = wCreateHotFile(window, "basic.shader");
 * 		if(wUpdateHotFile(file)) {
 * 			//contents discarded, simply recreate shader now
 * 			...
 * 		}
 * 		wDestroyHotFile(file);
 */


void wConvertBytesInBuffer(u8* buffer, isize size, u8 target, u8 replacement)
{
	for(isize i = 0; i < size; ++i) {
		if(buffer[i] == target) {
			buffer[i] = replacement;
		}
	}
}

wHotFile* wCreateHotFile(wWindow* window, string filename)
{
	wHotFile* file = malloc(sizeof(wHotFile));
	memset(file, 0, sizeof(wHotFile));
	file->zero = '\0';
	file->filenameLength = snprintf(file->filename, 511, "%s%s", 
			window->basePath,
			filename);
	file->handle = wGetFileHandle(file->filename);
	wUpdateHotFile(file);
	return file;
}

void wDestroyHotFile(wHotFile* file)
{
	if(file->data) free(file->data);
	wCloseFileHandle(file->handle);
	free(file);
}

i32 wCheckHotFile(wHotFile* file)
{
	return wGetFileModifiedTime(file->handle) != file->lastTime;
}

i32 wUpdateHotFile(wHotFile* file)
{
	if(wCheckHotFile(file)) {
		if(file->data) free(file->data);
		file->lastTime = wGetFileModifiedTime(file->handle);
		file->size = wGetFileSize(file->handle);
		file->data = malloc(file->size + 1);
		memset(file->data, 0, file->size + 1);
		wLoadSizedFile(file->filename, file->data, file->size + 1);
		char* buf = file->data;
		buf[file->size] = '\0';
		buf[file->size-1] = '\0';
		if(file->replaceBadSpaces) {
			wConvertBytesInBuffer(file->data, file->size, 160, ' ');
		}
		return 1;
	}
	return 0;
}

// wplArchive / s-archive reader

#define TINFL_IMPLEMENTATION
#include "thirdparty/tinfl.h"

wSarArchive* wSarLoad(void* file, wMemoryArena* alloc)
{
	wSarArchive* archive = wArenaPush(alloc, sizeof(wSarArchive));
	archive->base = file;
	archive->header = file;

	if(archive->header->magic != wSar_Magic) {
		printf("S-archive: Wrong magic?\n");
		printf("Expected: %u, Got: %u\n", wSar_Magic, archive->header->magic);
	}

	if(archive->header->version > wSar_Version) {
		printf("S-archive: Wrong version?\n");
		printf("Expected: %u, Got: %u\n", wSar_Version, archive->header->version);
	}
	archive->description = (void*)((usize)file + sizeof(wSarHeader));
	archive->files = (void*)(archive->base + archive->header->fileTableLocation);
	return archive;
}


isize wSarGetFileIndexByHash(wSarArchive* archive, u64 key)
{
	u64 localKey = 0;
	isize min = 0, max = archive->header->fileCount - 1, mid = 0;
	while(min <= max) {
		mid = (min + max) / 2;
		localKey = archive->files[mid].id.hash;
		if(localKey == key) {
			return mid;
		} else if(localKey < key) {
			min = mid + 1;
		} else {
			max = mid - 1;
		}
	}

	return -1;
}

wSarFile* wSarGetFile(wSarArchive* archive, string name)
{
	u64 hash = wHashString(name);
	isize index = wSarGetFileIndexByHash(archive, hash);
	if(hash == -1) return NULL;
	return archive->files + index;
}

void* wSarGetFileData(wSarArchive* archive, string name, 
		isize* sizeOut, wMemoryArena* arena)
{
	wSarFile* file = wSarGetFile(archive, name);
	void* input = archive->base + file->location;
	void* output = wArenaPush(arena, file->fullSize + 8);
	wDecompressMemToMem(
			output, file->fullSize + 8, 
			input, file->compressedSize, 
			0);
	if(sizeOut) {
		*sizeOut = file->fullSize;
	}
	return output;
}

/*
 * Texture Packer
 *  
 * TODO(will)
 *	--	More programmatic options to export tiles from sheet
 *		right now we're just abusing grid
 */ 

#define STBRP_ASSERT(...)
#define STBRP_SORT(...)
#define STB_RECT_PACK_IMPLEMENTATION
#include "thirdparty/stb_rect_pack.h"


void wInitAtlas(wTextureAtlas* atlas, isize capacity, wMemoryArena* arena)
{
	atlas->capacity = capacity;
	atlas->count = 0;
	atlas->segments = wArenaPush(arena, capacity * sizeof(wTextureSegment));
	//atlas->indexUpdate = wArenaPush(arena, capacity * sizeof(i32));
}

void wInitSegment(wTextureSegment* segment)
{
	wTextureSegment null = {0};
	*segment = null;
	segment->name = "";
	segment->index = -1;
}

i32 wAddSegment(wTextureAtlas* atlas,
		wTexture* texture,
		string name,
		i16 x, i16 y, i16 w, i16 h, 
		i32 bordered) 
{
	if(atlas->count >= atlas->capacity) return -1;
	wTextureSegment* s = atlas->segments + atlas->count;
	wInitSegment(s);
	s->index = atlas->count++;
	//s->region = region;
	s->x = x;
	s->y = y;
	s->w = w;
	s->h = h;
	s->texture = texture;
	s->bordered = bordered;
	s->name = name;
	s->hash = wHashString(name);

	return s->index;
}

void wAddSegmentGrid(wTextureAtlas* atlas, wTexture* texture, wTextureSegmentGrid* grid)
{
	if(grid->count == 0) {
		wAddSegment(atlas, texture, (string)grid->names, 
				0, 0, texture->w, texture->h, 0);
		return;
	}
	for(isize i = 0; i < grid->h; ++i) {
		for(isize j = 0; j < grid->w; ++j) {
			isize index = i * grid->w + j;
			if(index >= grid->count) return;
			//printf("%s\n", grid->names[index]);
			wAddSegment(atlas,
					texture,
					grid->names[index], 
					j * grid->size + grid->x,
					i * grid->size + grid->y,
					grid->size, grid->size,
					grid->bordered);
		}
	}
}

void wSortSegments(wTextureAtlas* atlas)
{
	for(isize i = 1; i < atlas->count; ++i) {
		isize j = i - 1;

		u64 hash = atlas->segments[i].hash;
		if(atlas->segments[j].hash > hash) {
			wTextureSegment temp = atlas->segments[i];
			while((j >= 0) && (atlas->segments[j].hash > hash)) {
				atlas->segments[j + 1] = atlas->segments[j];
				j--;
			}
			atlas->segments[j + 1] = temp;
		}
	}

}

i32 wFindSegmentByHash(wTextureAtlas* atlas, u64 key)
{
	u64 localKey = 0;
	isize min = 0, max = atlas->count - 1, mid = 0;
	while(min <= max) {
		mid = (min + max) / 2;
		localKey = atlas->segments[mid].hash;
		if(localKey == key) {
			return mid;
		} else if(localKey < key) {
			min = mid + 1;
		} else {
			max = mid - 1;
		}
	}

	return -1;
}

wTextureSegment* wFindSegment(wTextureAtlas* atlas, string name)
{
	u64 hash = wHashString(name);
	i32 index = wFindSegmentByHash(atlas, hash);
	if(index == -1) return NULL;
	return atlas->segments + index;
}
void wAtlasCreateTexture(wTextureAtlas* atlas, i32 size, wMemoryArena* arena)
{
	atlas->texture.w = size;
	atlas->texture.h = size;
	atlas->texture.pixels = wArenaPush(arena, 4 * size * size + 1);
}

// Surround with start/end temp arena to clean up intermediates
i32 wFinalizeAtlas(wTextureAtlas* atlas, wMemoryArena* arena) 
{
	// create a bunch of rects
	stbrp_rect* rects = wArenaPush(arena, sizeof(stbrp_rect) * atlas->count) ;
	for(isize i = 0; i < atlas->count; ++i) {
		wTextureSegment* s = atlas->segments + i;
		stbrp_rect* r = rects + i;
		r->id = i;
		r->w = s->w + 2;
		r->h = s->h + 2;
	}

	stbrp_context ctx = {0};
	i32 size = atlas->texture.w;
	stbrp_node* nodes = wArenaPush(arena, sizeof(stbrp_node) * size + 1);
	stbrp_init_target(&ctx, size, size, nodes, size + 1);
	i32 ret = stbrp_pack_rects(&ctx, rects, atlas->count);
	if(!ret) return 0;

	for(isize i = 0; i < atlas->count; ++i) {
		stbrp_rect* r = rects + i;
		wTextureSegment* s = atlas->segments + r->id;
		
		wCopyMemoryBlock(atlas->texture.pixels, s->texture->pixels,
				s->x, s->y, s->w, s->h,
				r->x + 1, r->y + 1,
				size, s->texture->w,
				4, // 4bpp pixels
				s->bordered
				);
		s->x = r->x + 1;
		s->y = r->y + 1;
	}

	wSortSegments(atlas);

	return 1;
}
