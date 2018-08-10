/* This is free and unencumbered software released into the public domain. */
/* Check the bottom of this file for the remainder of the unlicense */


#ifndef WB_ALLOC_NO_DISABLE_STUPID_MSVC_WARNINGS
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:201 204 28 244 706)
#endif
#endif

#define WB_ALLOC_ERROR_HANDLER(...)
#ifndef WB_ALLOC_ERROR_HANDLER
#define WB_ALLOC_ERROR_HANDLER(message, object, name) fprintf(stderr, \
		"wbAlloc error: [%s] %s\n", name, message)
#endif

#define WB_ALLOC_API
#define WB_ALLOC_BACKEND_API static
#define WB_ALLOC_STACK_PTR usize
#define WB_ALLOC_EXTENDED_INFO isize

#define WB_ALLOC_MEMSET memset
#define WB_ALLOC_MEMCPY memcpy

#ifndef WB_ALLOC_TAGGEDHEAP_MAX_TAG_COUNT
/* NOTE(will): if you listen to the Naughty Dog talk the tagged heap is based 
 * on, it seems like they only have ~4 tags? Something like "game", "render",
 * "physics", "anim", kinda thing. 64 should be way more than enough if that
 * is the average use case
 */
#define WB_ALLOC_TAGGEDHEAP_MAX_TAG_COUNT 64
#endif

/* These are equivalent to the PROT_READ values and friends */
#define wb_None 0
#define wb_Read 1
#define wb_Write 2
#define wb_Execute 4


#define FlagArenaNormal 0
#define FlagArenaFixedSize 1
#define FlagArenaStack 2
#define FlagArenaExtended 4
#define FlagArenaNoZeroMemory 8
#define FlagArenaNoRecommit 16 

#define FlagPoolNormal 0
#define FlagPoolFixedSize 1
#define FlagPoolCompacting 2
#define FlagPoolNoZeroMemory 4
#define FlagPoolNoDoubleFreeCheck 8

#define FlagwTaggedHeapNormal 0
#define FlagwTaggedHeapFixedSize 1
#define FlagwTaggedHeapNoZeroMemory 2
#define FlagwTaggedHeapNoSetCommitSize 4
#define FlagwTaggedHeapSearchForBestFit 8
#define wbi__wTaggedHeapSearchSize 8

/* Struct Definitions */

/* Function Prototypes */
WB_ALLOC_BACKEND_API void* wbi__allocateVirtualSpace(usize size);
WB_ALLOC_BACKEND_API void* wbi__commitMemory(void* addr, usize size, 
		isize flags);
WB_ALLOC_BACKEND_API void wbi__decommitMemory(void* addr, usize size);
WB_ALLOC_BACKEND_API void wbi__freeAddressSpace(void* addr, usize size);
WB_ALLOC_API wMemoryInfo wGetMemoryInfo();

WB_ALLOC_API 
isize alignTo(usize x, usize align);
WB_ALLOC_API 
void wbi__wTaggedArenaInit(wTaggedHeap* heap, 
		wTaggedHeapArena* arena, 
		isize tag);

WB_ALLOC_API 
void wbi__wTaggedArenaSortBySize(wTaggedHeapArena** array, isize count);


/* Platform-Specific Code */

#ifdef WB_ALLOC_WINDOWS
#ifdef WB_ALLOC_IMPLEMENTATION
//#include <Windows.h>

WB_ALLOC_BACKEND_API
void* wbi__allocateVirtualSpace(usize size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
}
 
WB_ALLOC_BACKEND_API
void* wbi__commitMemory(void* addr, usize size, isize flags)
{
	DWORD newFlags = 0;
	if(flags & wb_Read) {
		if(flags & wb_Write) {
			if(flags & wb_Execute) {
				newFlags = PAGE_EXECUTE_READWRITE;
			}
			newFlags = PAGE_READWRITE;
		} else if(flags & wb_Execute) {
			newFlags = PAGE_EXECUTE_READ;
		} else {
			newFlags = PAGE_READONLY;
		}
	} else if(flags & wb_Write) {
		if(flags & wb_Execute) {
			newFlags = PAGE_EXECUTE_READWRITE;
		} else {
			newFlags = PAGE_READWRITE;
		}
	} else if(flags & wb_Execute) {
		newFlags = PAGE_EXECUTE;
	} else {
		newFlags = PAGE_NOACCESS;
	}

    return VirtualAlloc(addr, size, MEM_COMMIT, newFlags);
}
 
WB_ALLOC_BACKEND_API
void wbi__decommitMemory(void* addr, usize size)
{
    VirtualFree((void*)addr, size, MEM_DECOMMIT);
}
 
WB_ALLOC_BACKEND_API
void wbi__freeAddressSpace(void* addr, usize size)
{
	/* This is a stupid hack, but blame clang/gcc with -Wall; not me */
	/* In any kind of optimized code, this should just get removed. */
	usize clangWouldWarnYouAboutThis = size;
	clangWouldWarnYouAboutThis++;
    VirtualFree((void*)addr, 0, MEM_RELEASE);
}

WB_ALLOC_API
wMemoryInfo wGetMemoryInfo()
{
	SYSTEM_INFO systemInfo;
	wMemoryInfo info;
	GetSystemInfo(&systemInfo);
	usize pageSize, localMem, totalMem;
	int ret;

	pageSize = systemInfo.dwPageSize;

	localMem = 0;
	totalMem = 0;
	ret = GetPhysicallyInstalledSystemMemory(&localMem);
	if(ret) {
		totalMem = localMem * 1024;
	}
	
	info.totalMemory = totalMem;
	info.commitSize = CalcMegabytes(1);
	info.pageSize = pageSize;
	info.commitFlags = wb_Read | wb_Write;
	return info;

}
#endif
#endif

#if defined(WB_ALLOC_LINUX) || defined(WB_ALLOC_MACOS)
#ifdef WB_ALLOC_IMPLEMENTATION

WB_ALLOC_BACKEND_API
void* wbi__allocateVirtualSpace(usize size)
{
    void * ptr = mmap((void*)0, size, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0);
    msync(ptr, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
}
 
WB_ALLOC_BACKEND_API
void* wbi__commitMemory(void* addr, usize size, isize flags)
{
    void * ptr = mmap(addr, size, flags, MAP_FIXED|MAP_SHARED|MAP_ANON, -1, 0);
    msync(addr, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
}
 
WB_ALLOC_BACKEND_API
void wbi__decommitMemory(void* addr, usize size)
{
    mmap(addr, size, PROT_NONE, MAP_FIXED|MAP_PRIVATE|MAP_ANON, -1, 0);
    msync(addr, size, MS_SYNC|MS_INVALIDATE);
}
 
WB_ALLOC_BACKEND_API
void wbi__freeAddressSpace(void* addr, usize size)
{
    msync(addr, size, MS_SYNC);
    munmap(addr, size);
}

WB_ALLOC_API
wMemoryInfo wGetMemoryInfo()
{
	usize totalMem, pageSize;
	wMemoryInfo info;

#if defined(WB_ALLOC_LINUX)
	struct sysinfo si;
	sysinfo(&si);
	totalMem = si.totalram;
#elif defined(WB_ALLOC_MACOS)
	int mib[2];
	usize size, eightByte;

	size = sizeof(usize);
	mib[0] = CTL_HW;
	mib[1] = HW_MEMSIZE;
	sysctl(mib, 2, &eightByte, &size, NULL, 0);
	totalMem = eightByte;
#endif

	pageSize = sysconf(_SC_PAGESIZE);

	info.totalMemory = totalMem;
	info.commitSize = CalcMegabytes(1);
	info.pageSize = pageSize;
	info.commitFlags = wb_Read | wb_Write;
	return info;
}
#endif
#endif

/* ===========================================================================
 * 		Main library -- Platform non-specific code
 * ===========================================================================
 */

#ifdef WB_ALLOC_IMPLEMENTATION
WB_ALLOC_API
isize alignTo(usize x, usize align)
{
	usize mod = x & (align - 1);
	return mod ? x + (align - mod) : x;
}

/* Memory Arena */

WB_ALLOC_API 
void wArenaFixedSizeInit(wMemoryArena* arena, 
		void* buffer, isize size, 
		isize flags)
{
#ifndef WB_ALLOC_NO_ZERO_ON_INIT
	WB_ALLOC_MEMSET(arena, 0, sizeof(wMemoryArena));
#endif

	arena->name = "arena";

	arena->flags = flags | FlagArenaFixedSize;
	arena->align = 8;

	arena->start = buffer;
	arena->head = buffer;
	arena->end = (void*)((isize)arena->start + size);
	arena->tempStart = NULL;
	arena->tempHead = NULL;
}


WB_ALLOC_API 
void wArenaInit(wMemoryArena* arena, wMemoryInfo info, isize flags)
{
	void* ret;
	ret = 0;
#ifndef WB_ALLOC_NO_ZERO_ON_INIT
	WB_ALLOC_MEMSET(arena, 0, sizeof(wMemoryArena));
#endif

#ifndef WB_ALLOC_NO_FLAG_CORRECTNESS_CHECKS
	if(flags & FlagArenaFixedSize) {
		WB_ALLOC_ERROR_HANDLER(
				"can't create a fixed-size arena with wArenaInit\n"
				"use wArenaFixedSizeInit instead.",
				arena, "arena");
		return;
	}
#endif

	arena->flags = flags;
	arena->name = "arena";
	arena->info = info;

	arena->start = wbi__allocateVirtualSpace(info.totalMemory);
	ret = wbi__commitMemory(arena->start,
			info.commitSize,
			info.commitFlags);
	if(!ret) {
		WB_ALLOC_ERROR_HANDLER("failed to commit inital memory", 
				arena, arena->name);
		return;
	}
	arena->head = arena->start;
	arena->end = (char*)arena->start + info.commitSize;
	arena->tempStart = NULL;
	arena->tempHead = NULL;
	arena->align = 8;
}

WB_ALLOC_API 
void* wArenaPushEx(wMemoryArena* arena, isize size, 
		WB_ALLOC_EXTENDED_INFO extended)
{
	void *oldHead, *ret;
	usize newHead, toExpand;

	if(arena->flags & FlagArenaStack) {
		size += sizeof(WB_ALLOC_STACK_PTR);
	}

	if(arena->flags & FlagArenaExtended) {
		size += sizeof(WB_ALLOC_EXTENDED_INFO);
	}

	oldHead = arena->head;
	newHead = alignTo((isize)arena->head + size, arena->align);

	if(newHead > (usize)arena->end) {
		if(arena->flags & FlagArenaFixedSize) {
			WB_ALLOC_ERROR_HANDLER(
					"ran out of memory",
					arena, arena->name);
			return NULL;
		}

		toExpand = alignTo(size, arena->info.commitSize);
		ret = wbi__commitMemory(arena->end, toExpand, arena->info.commitFlags);
		if(!ret) {
			WB_ALLOC_ERROR_HANDLER("failed to commit memory in wArenaPush",
					arena, arena->name);
			return NULL;
		}
		arena->end = (char*)arena->end + toExpand;
	}

	if(arena->flags & FlagArenaStack) {
		WB_ALLOC_STACK_PTR* head;
		head = (WB_ALLOC_STACK_PTR*)newHead;
		
		head--;
		*head = (WB_ALLOC_STACK_PTR)oldHead;
	}
	
	if(arena->flags & FlagArenaExtended) {
		WB_ALLOC_EXTENDED_INFO* head;
		head = (WB_ALLOC_EXTENDED_INFO*)oldHead;
		*head = extended;
		head++;
		oldHead = (void*)head;
	}

	arena->head = (void*)newHead;

	return oldHead;
}

WB_ALLOC_API
void* wArenaPush(wMemoryArena* arena, isize size)
{
	return wArenaPushEx(arena, size, 0);
}

WB_ALLOC_API 
void wArenaPop(wMemoryArena* arena)
{
	usize prevHeadPtr;
	void* newHead;
#ifndef WB_ALLOC_NO_FLAG_CORRECTNESS_CHECKS
	if(!(arena->flags & FlagArenaStack)) {
		WB_ALLOC_ERROR_HANDLER(
				"can't use wArenaPop with non-stack arenas",
				arena, arena->name);
		return;
	}
#endif

	
	prevHeadPtr = (isize)arena->head - sizeof(WB_ALLOC_STACK_PTR);
	newHead = (void*)(*(WB_ALLOC_STACK_PTR*)prevHeadPtr);
	if((isize)newHead <= (isize)arena->start) {
		arena->head = arena->start;
		return;
	}

	if(!(arena->flags & FlagArenaNoZeroMemory)) {
		usize size;
		size = (isize)arena->head - (isize)newHead;
		if(size > 0) {
			WB_ALLOC_MEMSET(newHead, 0, size);
		}
	}

	arena->head = newHead;
}

WB_ALLOC_API 
wMemoryArena* wArenaBootstrap(wMemoryInfo info, isize flags)
{
	wMemoryArena arena, *strapped;
#ifndef WB_ALLOC_NO_FLAG_CORRECTNESS_CHECKS
	if(flags & FlagArenaFixedSize) {
		WB_ALLOC_ERROR_HANDLER(
				"can't create a fixed-size arena with wArenaBootstrap\n"
				"use wArenaFixedSizeBootstrap instead.",
				NULL, "arena");
		return NULL;
	}
#endif

	wArenaInit(&arena, info, flags);
	strapped = (wMemoryArena*)
		wArenaPush(&arena, sizeof(wMemoryArena) + 16);

	*strapped = arena;

	if(flags & FlagArenaStack) {
		wArenaPushEx(strapped, 0, 0);
		*((WB_ALLOC_STACK_PTR*)(strapped->head) - 1) = 
			(WB_ALLOC_STACK_PTR)strapped->head;
	}
	
	return strapped;
}

WB_ALLOC_API 
wMemoryArena* wArenaFixedSizeBootstrap(void* buffer, usize size,
		isize flags)
{
	wMemoryArena arena, *strapped;
	wArenaFixedSizeInit(&arena, buffer, size, flags | FlagArenaFixedSize);
	strapped = (wMemoryArena*)
		wArenaPush(&arena, sizeof(wMemoryArena) + 16);
	*strapped = arena;
	if(flags & FlagArenaStack) {
		wArenaPushEx(strapped, 0, 0);
		*((WB_ALLOC_STACK_PTR*)(strapped->head) - 1) = 
			(WB_ALLOC_STACK_PTR)strapped->head;
	}
	return strapped;
}

WB_ALLOC_API 
void wArenaStartTemp(wMemoryArena* arena)
{
	if(arena->tempStart) return;
	arena->tempStart = (void*)alignTo((isize)arena->head, 
			arena->info.pageSize);
	arena->tempHead = arena->head;
	arena->head = arena->tempStart;
}

WB_ALLOC_API 
void wArenaEndTemp(wMemoryArena* arena)
{
	isize size;
	if(!arena->tempStart) return;
	arena->head = (void*)alignTo((isize)arena->head, arena->info.pageSize);
	size = (isize)arena->head - (isize)arena->tempStart;

	/* NOTE(will): if you have an arena with flags 
	 * 	ArenaNoRecommit | ArenaNoZeroMemory
	 * This just moves the pointer, which might be something you want to do.
	 */
	if(!(arena->flags & FlagArenaNoRecommit)) {
		wbi__decommitMemory(arena->tempStart, size);
		wbi__commitMemory(arena->tempStart, size, arena->info.commitFlags);
	} else if(!(arena->flags & FlagArenaNoZeroMemory)) {
		WB_ALLOC_MEMSET(arena->tempStart, 0,
				(isize)arena->head - (isize)arena->tempStart);
	}

	arena->head = arena->tempHead;
	arena->tempHead = NULL;
	arena->tempStart = NULL;
}

WB_ALLOC_API 
void wArenaClear(wMemoryArena* arena)
{
	wMemoryArena local = *arena;
	isize size = (isize)arena->end - (isize)arena->start;
	wbi__decommitMemory(local.start, size);
	wbi__commitMemory(local.start, size, local.info.commitFlags);
	*arena = local;
}

WB_ALLOC_API
void wArenaDestroy(wMemoryArena* arena)
{
	wbi__freeAddressSpace(arena->start, 
			(isize)arena->end - (isize)arena->start);
}

/* Memory Pool */
WB_ALLOC_API
void wPoolInit(wMemoryPool* pool, wMemoryArena* alloc, 
		usize elementSize,
		isize flags)
{
#ifndef WB_ALLOC_NO_ZERO_ON_INIT
	WB_ALLOC_MEMSET(pool, 0, sizeof(wMemoryPool));
#endif

	pool->alloc = alloc;
	pool->flags = flags;
	pool->name = "pool";
	pool->elementSize = elementSize < sizeof(void*) ?
		sizeof(void*) :
		elementSize;
	pool->count = 0;
	pool->lastFilled = -1;
	pool->capacity = (isize)
		((char*)alloc->end - (char*)alloc->head) / elementSize;

	pool->slots = alloc->head;
	pool->freeList = NULL;
}

WB_ALLOC_API
wMemoryPool* wPoolBootstrap(wMemoryInfo info, 
		isize elementSize,
		isize flags)
{
	wMemoryArena* alloc;
	wMemoryPool* pool;

	isize arenaFlags = 0;
	if(flags & FlagPoolFixedSize) {
		arenaFlags = FlagArenaFixedSize;
	}
	
	alloc = wArenaBootstrap(info, arenaFlags);
	pool = (wMemoryPool*)wArenaPush(alloc, sizeof(wMemoryPool));

	wPoolInit(pool, alloc, elementSize, flags);
	return pool;
}

WB_ALLOC_API
wMemoryPool* wPoolFixedSizeBootstrap(
		isize elementSize, 
		void* buffer, usize size, 
		isize flags)
{
	wMemoryArena* alloc;
	wMemoryPool* pool;
	flags |= FlagPoolFixedSize;
	
	alloc = wArenaFixedSizeBootstrap(buffer, size, FlagArenaFixedSize);
	pool = (wMemoryPool*)wArenaPush(alloc, sizeof(wMemoryPool));

	wPoolInit(pool, alloc, elementSize, flags);
	return pool;
}

isize wPoolIndex(wMemoryPool* pool, void* ptr)
{
	isize diff = (isize)ptr - (isize)pool->slots;
	return diff / pool->elementSize;
}

void* wPoolFromIndex(wMemoryPool* pool, isize index)
{
	return (char*)pool->slots + index * pool->elementSize;
}

WB_ALLOC_API
void* wPoolRetrieve(wMemoryPool* pool)
{
	void *ptr, *ret;
	ptr = NULL;
	if((!(pool->flags & FlagPoolCompacting)) && pool->freeList) {
		ptr = pool->freeList;
		pool->freeList = (void**)*pool->freeList;
		pool->count++;

		if(!(pool->flags & FlagPoolNoZeroMemory)) {
			WB_ALLOC_MEMSET(ptr, 0, pool->elementSize);
		}

		return ptr;
	} 

	if(pool->lastFilled >= pool->capacity - 1) {
		if(pool->flags & FlagPoolFixedSize) {
			WB_ALLOC_ERROR_HANDLER("pool ran out of memory",
					pool, pool->name);
			return NULL;
		}

		ret = wArenaPush(pool->alloc, pool->alloc->info.commitSize);
		if(!ret) {
			WB_ALLOC_ERROR_HANDLER("wArenaPush failed in wPoolRetrieve", 
					pool, pool->name);
			return NULL;
		}
		pool->capacity = (isize)
			((char*)pool->alloc->end - (char*)pool->slots) / pool->elementSize;
	}

	ptr = (char*)pool->slots + ++pool->lastFilled * pool->elementSize;
	pool->count++;
	if(!(pool->flags & FlagPoolNoZeroMemory)) {
		WB_ALLOC_MEMSET(ptr, 0, pool->elementSize);
	}
	return ptr;
}

WB_ALLOC_API
void wPoolRelease(wMemoryPool* pool, void* ptr)
{
	pool->count--;

	if(pool->freeList && !(pool->flags & FlagPoolNoDoubleFreeCheck)) {
		void** localList = pool->freeList;
		do {
			if(ptr == localList) {
				WB_ALLOC_ERROR_HANDLER("caught attempting to free previously "
						"freed memory in wPoolRelease", 
						pool, pool->name);
				return;
			}
		} while((localList = (void**)*localList));
	}

	if(pool->flags & FlagPoolCompacting) {
		WB_ALLOC_MEMCPY(ptr, 
				(char*)pool->slots + pool->count * pool->elementSize,
				pool->elementSize);
		return;
	}

	*(void**)ptr = pool->freeList;
	pool->freeList = (void**)ptr;
}

/*
 * TODO(will): Maybe, someday, have a tagged heap that uses real memoryArenas
 * 	behind the scenes, so that you get to benefit from stack and extended 
 * 	mode for ~free; maybe as a preprocessor flag?
 */ 
WB_ALLOC_API
isize calcwTaggedHeapSize(isize arenaSize, isize arenaCount,
		isize bootstrapped)
{
	return arenaCount * (arenaSize + sizeof(wTaggedHeapArena))
		+ sizeof(wTaggedHeap) * bootstrapped;
}

WB_ALLOC_API
void wTaggedInit(wTaggedHeap* heap, wMemoryArena* arena, 
		isize internalArenaSize, isize flags)
{
#ifndef WB_ALLOC_NO_ZERO_ON_INIT
	WB_ALLOC_MEMSET(heap, 0, sizeof(wTaggedHeap));
#endif

	heap->name = "wTaggedHeap";
	heap->flags = flags;
	heap->align = 8;
	heap->arenaSize = internalArenaSize;
	wPoolInit(&heap->pool, arena, 
			internalArenaSize + sizeof(wTaggedHeapArena), 
			FlagPoolNormal | FlagPoolNoDoubleFreeCheck | 
			((flags & FlagwTaggedHeapNoZeroMemory) ? 
			FlagPoolNoZeroMemory : 
			0));
}

WB_ALLOC_API
wTaggedHeap* wTaggedBootstrap(wMemoryInfo info, 
		isize arenaSize,
		isize flags)
{
	wTaggedHeap* strapped;
	wTaggedHeap heap;
	wMemoryArena* arena;
	info.commitSize = calcwTaggedHeapSize(arenaSize, 8, 1);
	arena = wArenaBootstrap(info, 
			((flags & FlagwTaggedHeapNoZeroMemory) ? 
			FlagArenaNoZeroMemory :
			FlagArenaNormal));
	strapped = (wTaggedHeap*)wArenaPush(arena, sizeof(wTaggedHeap) + 16);
	wTaggedInit(&heap, arena, arenaSize, flags);
	*strapped = heap;
	return strapped;
}

WB_ALLOC_API
wTaggedHeap* wTaggedFixedSizeBootstrap(
		isize arenaSize, 
		void* buffer, isize bufferSize, 
		isize flags)
{
	wMemoryArena* alloc;
	wTaggedHeap* heap;
	flags |= FlagwTaggedHeapFixedSize;
	
	alloc = wArenaFixedSizeBootstrap(buffer, bufferSize, 
			FlagArenaFixedSize |
			((flags & FlagwTaggedHeapNoZeroMemory) ? 
			FlagArenaNoZeroMemory : 0));
	heap = (wTaggedHeap*)wArenaPush(alloc, sizeof(wTaggedHeap));

	wTaggedInit(heap, alloc, arenaSize, flags);
	return heap;
	
}

WB_ALLOC_API
void wbi__wTaggedArenaInit(wTaggedHeap* heap, 
		wTaggedHeapArena* arena,
		isize tag)
{
#ifndef WB_ALLOC_NO_ZERO_ON_INIT
	WB_ALLOC_MEMSET(arena, 0, sizeof(wTaggedHeapArena));
#endif

	arena->tag = tag;
	arena->head = &arena->buffer;
	arena->end = (void*)((char*)arena->head + heap->arenaSize);
}

WB_ALLOC_API
void wbi__wTaggedArenaSortBySize(wTaggedHeapArena** array, isize count)
{
#define wbi__arenaSize(arena) ((isize)arena->head - (isize)arena->head)
	isize i, j, minSize;
	for(i = 1; i < count; ++i) {
		j = i - 1;

		minSize = wbi__arenaSize(array[i]);
		if(wbi__arenaSize(array[j]) > minSize) {
			wTaggedHeapArena* temp = array[i];
			while((j >= 0) && (wbi__arenaSize(array[j]) > minSize)) {
				array[j + 1] = array[j];
				j--;
			}
			array[j + 1] = temp;
		}
	}
#undef wbi__arenaSize
}


WB_ALLOC_API
void* wTaggedAlloc(wTaggedHeap* heap, isize tag, usize size)
{
	wTaggedHeapArena *arena, *newArena;
	void* oldHead;
	wTaggedHeapArena* canFit[wbi__wTaggedHeapSearchSize];
	isize canFitCount = 0;

	if(size > heap->arenaSize) {
		WB_ALLOC_ERROR_HANDLER("cannot allocate an object larger than the "
				"size of a tagged heap arena.",
				heap, heap->name);
		return NULL;
	}

	if(!heap->arenas[tag]) {
		heap->arenas[tag] = (wTaggedHeapArena*)wPoolRetrieve(&heap->pool);
		if(!heap->arenas[tag]) {
			WB_ALLOC_ERROR_HANDLER("tagged heap arena retrieve returned null "
					"when creating a new tag",
				heap, heap->name);
			return NULL;
		}
		wbi__wTaggedArenaInit(heap, heap->arenas[tag], tag);
	}

	arena = heap->arenas[tag];

	if((char*)arena->head + size > (char*)arena->end) {
		/* TODO(will) add a find-better-fit option rather than
		 * allocating new arenas whenever */

		if(heap->flags & FlagwTaggedHeapSearchForBestFit) {
			while((arena = arena->next)) {
				if((char*)arena->head + size < (char*)arena->end) {
					canFit[canFitCount++] = arena;
					if(canFitCount > (wbi__wTaggedHeapSearchSize - 1)) {
						break;
					}
				}
			}

			if(canFitCount > 0) {
				wbi__wTaggedArenaSortBySize(canFit, canFitCount);
				arena = canFit[0];
			}
		}

		if(canFitCount == 0) {
			newArena = (wTaggedHeapArena*)wPoolRetrieve(&heap->pool);
			if(!newArena) {
				WB_ALLOC_ERROR_HANDLER(
						"tagged heap arena retrieve returned null",
						heap, heap->name);
				return NULL;
			}
			wbi__wTaggedArenaInit(heap, newArena, tag);
			newArena->next = arena;
			arena = newArena;
		}
	}

	oldHead = arena->head;
	arena->head = (void*)alignTo((isize)arena->head + size, heap->align);
	return oldHead;
}

WB_ALLOC_API
void wTaggedFree(wTaggedHeap* heap, isize tag)
{
	wTaggedHeapArena *head;
	if((head = heap->arenas[tag])) do {
		wPoolRelease(&heap->pool, head);
	} while((head = head->next));
	heap->arenas[tag] = NULL;
}
#endif

#ifndef WB_ALLOC_NO_DISABLE_STUPID_MSVC_WARNINGS
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif

/*
Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/
