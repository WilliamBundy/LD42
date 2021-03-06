ctime=usr\bin\ctime.exe
lineify=usr\bin\lineify.exe
wirmpht=usr\bin\wirmpht.exe
disabled=/wd4477\
		 /wd4244\
		 /wd4334\
		 /wd4305\
		 /wd4101\
		 /D_CRT_SECURE_NO_WARNINGS
name=LD42
.SILENT:
all: windows.mak start usr/lib/wpl.lib bin/LD42.exe end

usr/lib/wpl.lib: src/wpl/*
	cl /nologo /c /TC /Zi /Gd /EHsc \
		/W3 /fp:fast $(disabled) \
		/I"usr/include" /DWPL_WINDOWS /DWPL_SDL_BACKEND \
		src/wpl/wpl.c /Fd"bin/wpl.pdb" 
	lib /NOLOGO /SUBSYSTEM:WINDOWS /LIBPATH:"usr/lib" \
		/NODEFAULTLIB wpl.obj  /OUT:"usr/lib/wpl.lib"

libwin32: 
	echo WPL_WIN32_BACKEND
	cl /nologo /c /TC /Zi /Gd /EHsc /Gs16777216 /F16777216 \
		/Gm- /GS- /W3 /fp:fast $(disabled) \
		/I"usr/include" /DWPL_REPLACE_CRT /DWPL_WIN32_BACKEND \
		src/wpl/wpl.c /Fd"bin/wpl.pdb" 
	lib /NOLOGO /SUBSYSTEM:WINDOWS /LIBPATH:"usr/lib" \
		/NODEFAULTLIB wpl.obj  /OUT:"usr/lib/wpl.lib"

bin/LD42.exe:  src/*
	echo bin/$(name).exe (SDL)
	$(wirmpht) -p -s -t src/main.c > src/generated.h
	cl /nologo /TC /Zi /Gd /EHsc /W3 \
		/fp:fast $(disabled) \
		/I"usr/include" \
		/DWPL_WINDOWS /DWPL_SDL_BACKEND \
		src/main.c \
		/Fe"bin/$(name).exe" /Fd"bin/$(name).pdb" \
	/link /NOLOGO /INCREMENTAL:NO /SUBSYSTEM:CONSOLE \
		/LIBPATH:"usr/lib"\
		wpl.lib \
		kernel32.lib user32.lib \
		SDL2.lib SDL2main.lib \
		opengl32.lib gdi32.lib

gameNoCRT: 
	echo bin/$(name).exe (Win32/NoCRT)
	$(wirmpht) -p -s -t src/main.c > src/generated.h
	cl /nologo /TC /Zi /Gd /EHsc /W3 /F16777216 \
		/Gs16777216 /Gm- /GS- /fp:fast $(disabled) \
		/DWPL_REPLACE_CRT /DWPL_USE_WIN32 \
		src/main.c \
		/Fe"bin/$(name).exe" /Fd"bin/$(name).pdb" \
	/link /NOLOGO /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /NODEFAULTLIB \
	/STACK:16777216,16777216 /entry:GameMain /LIBPATH:"usr/lib"\
		wpl.lib \
		kernel32.lib user32.lib \
		opengl32.lib gdi32.lib \
		ole32.lib winmm.lib

start:
	$(ctime) -begin usr/bin/ld42.ctm

end:
	del *.obj >nul 2>&1
	$(ctime) -end usr/bin/ld42.ctm
