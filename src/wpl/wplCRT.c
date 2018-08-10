/* This is the list of everything reimplemented in this file.
 * wb_tm does the math stuff, and everything else is windows/sdl dependent
 
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
extern void* stderr;
extern void* stdout;

*/
int _fltused = 1;

#pragma function(memset)
void* memset(void* s, i32 ivalue, usize size)
{
	/*
	 * This isn't a very good memset; it's consistently slower than msvc's
	 * however, it's entirely possible they will just insert theirs as part
	 * of the optimizer.
	 */
	u64 byteVal = ivalue & 0xFF;
	u64 value = 
		byteVal << 32l | byteVal << 40l | byteVal << 48l | byteVal << 56l | 
		byteVal << 8l  | byteVal << 16l | byteVal << 24l | byteVal;
	usize align = size & 7;
	if(align != 0) {
		char* ss = s;
		for(usize i = 0; i < align; ++i) {
			ss[i] = (char)byteVal;
		}
	}

	u64* dst = (void*)((char*)s + align);
	size = size - align;
	size >>= 3;
	for(usize i = 0; i < size; ++i) {
		dst[i] = value;
	}
	return s;
}

#pragma function(memcmp)
i32 memcmp(const void* s1, const void* s2, usize size)
{
	const u8* a = s1;
	const u8* b = s2;

	while(size--) {
		int d = *a++ - *b++;
		if(d > 0) return 1;
		else if(d < 0) return -1;
	}

	return 0;
}


#define wbssecpy_mv(a, b, d) \
		head = _mm_lddqu_si128((vi128*)(src + (a))); \
		head2 = _mm_lddqu_si128((vi128*)(src + (b))); \
		tail = _mm_lddqu_si128((vi128*)(src + size - (b))); \
		tail2 = _mm_lddqu_si128((vi128*)(src + size - (d))); \
		_mm_storeu_si128((vi128*)(dst + (a)), head); \
		_mm_storeu_si128((vi128*)(dst + size - (b)), tail); \
		_mm_storeu_si128((vi128*)(dst + (b)), head2); \
		_mm_storeu_si128((vi128*)(dst + size - (d)), tail2);

#pragma function(memcpy)
void* memcpy(void *dest, const void *source, usize size)
{
	u8* dst = dest;
	const u8* src = source;

	if(size <= 512) {
		if(size <= 16) {
			u32 src4;
			u16 src2;
			u8  src1;

			u64 head, tail;
			switch(size) {
				case 0: break;
				case 1: *dst = *src; break;
				case 2: *(u16*)dst = *(u16*)src; break;
				case 3: src2 = *(u16*)src;
						src1 = *(src + 2);
						*(u16*)dst = *(u16*)src;
						*(dst + 2) = src1; 
						break;
				case 4: *(u32*)dst = *(u32*)src; break;
				case 5: src4 = *(u32*)src;
						src1 = *(src + 4); 
						*(u32*)dst = src4;
						*(dst + 4) = src1;
						break;
				case 6: src4 = *(u32*)src;
						src2 = *(u16*)(src + 4); 
						*(u32*)dst = src4;
						*(u16*)(dst + 4) = src2;
						break;
				case 7: src4 = *(u32*)src;
						src2 = *(u16*)(src + 4); 
						src1 = *(src + 6);
						*(u32*)dst = src4;
						*(u16*)(dst + 4) = src2;
						*(dst + 6) = src1;
						break;
				case 8: *(u64*)dst = *(u64*)src; break;
				default: head = *(u64*)src;
						 tail = *(u64*)(src + size - 8);
						 *(u64*)dst = head;
						 *(u64*)(dst + size - 8) = tail;
						 break;
			}
		} else if(size <= 32) {
			vi128 head = _mm_lddqu_si128((vi128*)src);
			vi128 tail = _mm_lddqu_si128((vi128*)(src + size - 16));
			_mm_storeu_si128((vi128*)dst, head);
			_mm_storeu_si128((vi128*)(dst + size - 16), tail);
		} else if(size <= 64) {
			vi128 head, head2, tail, tail2;
			wbssecpy_mv( 0, 16, 32);
		} else if(size <= 128) {
			vi128 head, head2, tail, tail2;
			wbssecpy_mv( 0, 16, 32);
			wbssecpy_mv(32, 48, 64);
		} else if(size <= 256) {
			vi128 head, head2, tail, tail2;
			wbssecpy_mv( 0, 16, 32);
			wbssecpy_mv(32, 48, 64);
			wbssecpy_mv(64, 80, 96);
			wbssecpy_mv(96, 112, 128);
		} else if(size <= 512) {
			vi128 head, head2, tail, tail2;
			wbssecpy_mv(0, 16, 32);
			wbssecpy_mv(32, 48, 64);
			wbssecpy_mv(64, 80, 96);
			wbssecpy_mv(96, 112, 128);
			wbssecpy_mv(128, 144, 160);
			wbssecpy_mv(160, 176, 192);
			wbssecpy_mv(192, 208, 224);
			wbssecpy_mv(224, 240, 256);
		}
	} else if(size <= (1 << 21)) {
		__movsb(dst, src, size);
	} else {
		i32 dstalign = (((usize)dst + 15) & -16) == (usize)dst;
		i32 srcalign = (((usize)src + 15) & -16) == (usize)src;
		const vi128* srcm = (vi128*)src;
		vi128* dstm = (vi128*)dst;
		i64 itercount = size >> 4;
		usize end = (usize)src + size - 32;
		if(dstalign && srcalign && size > 32768) {
			switch(itercount % 8) {
				case 0:		_mm_stream_si128(dstm++, _mm_load_si128(srcm++));
				case 7:		_mm_stream_si128(dstm++, _mm_load_si128(srcm++));
				case 6:		_mm_stream_si128(dstm++, _mm_load_si128(srcm++));
				case 5:		_mm_stream_si128(dstm++, _mm_load_si128(srcm++));
				case 4:		_mm_stream_si128(dstm++, _mm_load_si128(srcm++));
				case 3:		_mm_stream_si128(dstm++, _mm_load_si128(srcm++));
				case 2:		_mm_stream_si128(dstm++, _mm_load_si128(srcm++));
				case 1:		_mm_stream_si128(dstm++, _mm_load_si128(srcm++));
			}

			while((usize)srcm + 128 < end) {
				_mm_stream_si128(dstm,     _mm_load_si128(srcm));
				_mm_stream_si128(dstm + 1, _mm_load_si128(srcm + 1));
				_mm_stream_si128(dstm + 2, _mm_load_si128(srcm + 2));
				_mm_stream_si128(dstm + 3, _mm_load_si128(srcm + 3));
				_mm_stream_si128(dstm + 4, _mm_load_si128(srcm + 4));
				_mm_stream_si128(dstm + 5, _mm_load_si128(srcm + 5));
				_mm_stream_si128(dstm + 6, _mm_load_si128(srcm + 6));
				_mm_stream_si128(dstm + 7, _mm_load_si128(srcm + 7));
				dstm += 8;
				srcm += 8;
			}
		} else {
			switch(itercount % 8) {
				case 0:		_mm_storeu_si128(dstm++, _mm_lddqu_si128(srcm++));
				case 7:		_mm_storeu_si128(dstm++, _mm_lddqu_si128(srcm++));
				case 6:		_mm_storeu_si128(dstm++, _mm_lddqu_si128(srcm++));
				case 5:		_mm_storeu_si128(dstm++, _mm_lddqu_si128(srcm++));
				case 4:		_mm_storeu_si128(dstm++, _mm_lddqu_si128(srcm++));
				case 3:		_mm_storeu_si128(dstm++, _mm_lddqu_si128(srcm++));
				case 2:		_mm_storeu_si128(dstm++, _mm_lddqu_si128(srcm++));
				case 1:		_mm_storeu_si128(dstm++, _mm_lddqu_si128(srcm++));
			}
			while((usize)srcm + 128 < end) {
				_mm_storeu_si128(dstm,     _mm_lddqu_si128(srcm));
				_mm_storeu_si128(dstm + 1, _mm_lddqu_si128(srcm + 1));
				_mm_storeu_si128(dstm + 2, _mm_lddqu_si128(srcm + 2));
				_mm_storeu_si128(dstm + 3, _mm_lddqu_si128(srcm + 3));
				_mm_storeu_si128(dstm + 4, _mm_lddqu_si128(srcm + 4));
				_mm_storeu_si128(dstm + 5, _mm_lddqu_si128(srcm + 5));
				_mm_storeu_si128(dstm + 6, _mm_lddqu_si128(srcm + 6));
				_mm_storeu_si128(dstm + 7, _mm_lddqu_si128(srcm + 7));
				dstm += 8;
				srcm += 8;
			}
		}

		while((usize)srcm < end) {
			_mm_storeu_si128(dstm++, _mm_lddqu_si128(srcm++));
		}	

		vi128 tail = _mm_lddqu_si128((vi128*)(src + size - 16));
		_mm_storeu_si128((vi128*)(dst + size - 16), tail);

		tail = _mm_lddqu_si128((vi128*)(src + size - 32));
		_mm_storeu_si128((vi128*)(dst + size - 32), tail);
	} 

	return dst;
}

#pragma function(strlen)
usize strlen(const char* c)
{
	int i = 0;
	while(*c++) i++;
	return i;
}

void* malloc(usize size)
{
	return HeapAlloc(GetProcessHeap(), 0, size);
}

void free(void* mem)
{
	HeapFree(GetProcessHeap(), 0, mem);
}

void* realloc(void* mem, usize size)
{
	if(mem == NULL) {
		return malloc(size);
	} else if(size == 0) {
		free(mem);
	}
	return HeapReAlloc(GetProcessHeap(), 0, mem, size);
}


#pragma function(abs)
i32 abs(i32 x)
{
	return x < 0 ? -x : x;
}

/* stdio.h */

#ifndef wpl_PrintfTempBufSize
#define wpl_PrintfTempBufSize 4096
#endif

// These need to be set with GetStdHandle(...);
void* stdout;
void* stderr;

int vfprintf(void* file, string fmt, va_list args)
{
	char buf[wpl_PrintfTempBufSize];
	size_t len = vsnprintf(buf, wpl_PrintfTempBufSize, fmt, args);
	DWORD out = 0;
	WriteFile(file, buf, (DWORD)len, &out, NULL);
	return (size_t)out;
}

i32 fprintf(void* file, string fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	return vfprintf(file, fmt, args);
}

i32 eprintf(string fmt, ...)
{
	if(!stdout) {
		stdout = GetStdHandle(STD_OUTPUT_HANDLE);
		stderr = GetStdHandle(STD_ERROR_HANDLE);
	}
	va_list args;
	va_start(args, fmt);
	return vfprintf(stdout, fmt, args);
}


i32 printf(string fmt, ...)
{
	if(!stdout) {
		stdout = GetStdHandle(STD_OUTPUT_HANDLE);
		stderr = GetStdHandle(STD_ERROR_HANDLE);
	}

	va_list args;
	va_start(args, fmt);
	return vfprintf(stdout, fmt, args);
}



